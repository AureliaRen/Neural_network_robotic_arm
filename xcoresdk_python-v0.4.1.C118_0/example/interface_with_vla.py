import time
import math
import sys

from scipy.spatial.transform import Rotation as R
import threading
import setup_path
import platform
import numpy as np
from my_first_control_file import world_to_robot_coordinates_with_euler
from move_example import wait_robot as wait_single_robot
from datetime import timedelta
import socket
import cv2
import pickle
from force_control_example import calibrate_force_sensor
from water_manipulation import Arm_offset, receive_image_from_server

if platform.system() == 'Windows':
    from Release.windows import xCoreSDK_python
    from Release.windows.xCoreSDK_python.EventInfoKey import MoveExecution
    from Release.windows.xCoreSDK_python import RtSupportedFields
elif platform.system() == 'Linux':
    from Release.linux import xCoreSDK_python
    from Release.linux.xCoreSDK_python.EventInfoKey import MoveExecution
    from Release.linux.xCoreSDK_python import RtSupportedFields
else:
    raise ImportError("Unsupported operating system")
import argparse
import sys
import threading
import time
import yaml
from collections import deque

import numpy as np
import torch
from PIL import Image as PImage
import cv2

from scripts.agilex_model import create_model
import pyrealsense2 as rs
import pdb
from log import print_separator, print_log

# sys.path.append("./")
# 三个相机的名称
CAMERA_NAMES = ['cam_high', 'cam_right_wrist', 'cam_left_wrist']

observation_window = None

lang_embeddings = None

# debug
preload_images = None

def wait_robot(robot_r,ec_r):
    '''等待运动结束 - 通过查询机械臂是否处于运动中的方式'''
    print_separator("wait_robot",length=80)
    running = True
    while (running):
        time.sleep(0.1)
        st_r = robot_r.operationState(ec_r)
        if(xCoreSDK_python.OperationState.idle or st_r == xCoreSDK_python.OperationState.unknown):
            running = False
    print("move finished")

class RM_controller:
    # 初始化机械臂控制器
    def __init__(self):
        ip_r = '192.168.0.160'
        local_ip = '192.168.2.100'
        self.right_arm = xCoreSDK_python.xMateRobot(ip_r, local_ip)
        self.ec_r = {}
        print_separator("初始化右臂", length=110)
        self.right_arm.setOperateMode(xCoreSDK_python.OperateMode.manual, self.ec_r)
        print_log("setOperateMode", self.ec_r)
        self.right_arm.setPowerState(True, self.ec_r)
        print_log("setPowerState", self.ec_r)
        self.right_arm.setMotionControlMode(xCoreSDK_python.MotionControlMode.NrtCommandMode, self.ec_r)
        print_log("setMotionControlMode", self.ec_r)
        self.right_arm.setDefaultZone(0, self.ec_r)
        self.right_arm.setDefaultSpeed(50, self.ec_r)

    # 用来获得关节角度+夹爪
    def get_qpos(self):
        joint_pos_r = self.right_arm.jointPos(self.ec_r)
        switch_r1 = self.right_arm.getDO(0, 0, self.ec_r)
        switch_r2 = self.right_arm.getDO(0, 1, self.ec_r)
        if switch_r1 == True and switch_r2 == True:
            gripper = 1 #闭合
        elif switch_r1 == False and switch_r2 == False:
            gripper = 0 #张开
        else:
            gripper = 0
        qpos = np.zeros(7)
        qpos[0:6] = joint_pos_r[0:6]
        qpos[6] = gripper
        print(qpos)
        return qpos

    # 控制机械臂运动
    def move(self, d_action):
        action1 = [d_action[0], d_action[1], d_action[2], d_action[3], d_action[4], d_action[5]] #六个关节角
        print("delta_move:", action1) #六个关节角的变化量
        # 获取位置
        pos = self.get_qpos()
        position = pos[0:6]
        # 模型输出的是变化，我们机械臂只支持运动绝对位置，不支持相对距离
        position[0] = position[0] + action1[0]
        position[1] = position[1] + action1[1]
        position[2] = position[2] + action1[2]
        position[3] = position[3] + action1[3]
        position[4] = position[4] + action1[4]
        position[5] = position[5] + action1[5]
        # 我们夹爪只能张/合，所以按照阈值来控制张合
        if d_action[6] > 0.9:
            self.right_arm.setDO(0, 0, True, self.ec_r) # 张开
            self.right_arm.setDO(0, 1, True, self.ec_r)
        else:
            self.right_arm.setDO(0, 0, False, self.ec_r) # 合拢
            self.right_arm.setDO(0, 1, False, self.ec_r)
        # 控制机械臂运动到制定位置
        joint_pos = xCoreSDK_python.JointPosition(action1)
        absjcmd = xCoreSDK_python.MoveAbsJCommand(joint_pos, 50, 10)
        cmdID = xCoreSDK_python.PyString()
        self.right_arm.moveAppend([absjcmd], cmdID, self.ec_r)
        self.right_arm.moveStart(self.ec_r)
        wait_robot()


# 用来控制图像获取的
class Img_controller:
    # 两个D45i相机，通过socket通信访问服务器获取数据
    def __init__(self):
        self.top = None
        self.right = None
        self.t = 0

    def get_img(self):
        image_top = receive_image_from_server(8001)
        image_right = receive_image_from_server(8000)
        self.t += 1
        return image_top, image_right


# 主控制器
class Controller:
    # 建立连接
    def __init__(self):
        self.right_arm_controller = RM_controller()
        self.img_controller = Img_controller()


# Initialize the model
def make_policy(args):
    with open(args.config_path, "r") as fp:
        config = yaml.safe_load(fp)
    args.config = config

    pretrained_vision_encoder_name_or_path = "google/siglip-so400m-patch14-384"
    model = create_model(
        args=args.config,
        dtype=torch.bfloat16,
        pretrained=args.pretrained_model_name_or_path,
        pretrained_vision_encoder_name_or_path=pretrained_vision_encoder_name_or_path,
        control_frequency=args.ctrl_freq,
    )

    return model


def set_seed(seed):
    torch.manual_seed(seed)
    np.random.seed(seed)


# Interpolate the actions to make the robot move smoothly
def interpolate_action(args, prev_action, cur_action):
    steps = np.concatenate((np.array(args.arm_steps_length), np.array(args.arm_steps_length)), axis=0)
    diff = np.abs(cur_action - prev_action)
    step = np.ceil(diff / steps).astype(int)
    step = np.max(step)
    if step <= 1:
        return cur_action[np.newaxis, :]
    new_actions = np.linspace(prev_action, cur_action, step + 1)
    return new_actions[1:]


def get_config(args):
    config = {
        'episode_len': args.max_publish_step,
        'state_dim': 7,
        'chunk_size': args.chunk_size,
        'camera_names': CAMERA_NAMES,
    }
    return config


# Get the observation from the ROS topic
def get_RM_observation(args, controller):
    puppet_arm_right = controller.right_arm_controller.get_qpos()
    img_front, img_right = controller.img_controller.get_img()
    return img_front, img_right, puppet_arm_right


# Update the observation window buffer
def update_observation_window(args, config, controller):
    # JPEG transformation
    # Align with training
    def jpeg_mapping(img):
        img = cv2.imencode('.jpg', img)[1].tobytes()
        img = cv2.imdecode(np.frombuffer(img, np.uint8), cv2.IMREAD_COLOR)
        return img

    global observation_window
    if observation_window is None:
        observation_window = deque(maxlen=2)

        # Append the first dummy image
        observation_window.append(
            {
                'qpos': None,
                'images':
                    {
                        config["camera_names"][0]: None,
                        config["camera_names"][1]: None,
                        config["camera_names"][2]: None,
                    },
            }
        )

    img_front, img_right, puppet_arm_right = get_RM_observation(args, controller)
    img_front = jpeg_mapping(img_front)
    img_left = None
    img_right = jpeg_mapping(img_right)

    qpos = np.array(puppet_arm_right)
    qpos = torch.from_numpy(qpos).float().cuda()
    observation_window.append(
        {
            'qpos': qpos,
            'images':
                {
                    config["camera_names"][0]: img_front,
                    config["camera_names"][1]: img_right,
                    config["camera_names"][2]: img_left,
                },
        }
    )


# RDT inference
def inference_fn(args, config, policy, t):
    global observation_window
    global lang_embeddings

    # print(f"Start inference_thread_fn: t={t}")
    while True:
        time1 = time.time()

        # fetch images in sequence [front, right, left]
        image_arrs = [
            observation_window[-2]['images'][config['camera_names'][0]],
            observation_window[-2]['images'][config['camera_names'][1]],
            observation_window[-2]['images'][config['camera_names'][2]],

            observation_window[-1]['images'][config['camera_names'][0]],
            observation_window[-1]['images'][config['camera_names'][1]],
            observation_window[-1]['images'][config['camera_names'][2]]
        ]

        # fetch debug images in sequence [front, right, left]
        # image_arrs = [
        #     preload_images[config['camera_names'][0]][max(t - 1, 0)],
        #     preload_images[config['camera_names'][2]][max(t - 1, 0)],
        #     preload_images[config['camera_names'][1]][max(t - 1, 0)],
        #     preload_images[config['camera_names'][0]][t],
        #     preload_images[config['camera_names'][2]][t],
        #     preload_images[config['camera_names'][1]][t]
        # ]
        # # encode the images
        # for i in range(len(image_arrs)):
        #     image_arrs[i] = cv2.imdecode(np.frombuffer(image_arrs[i], np.uint8), cv2.IMREAD_COLOR)
        # proprio = torch.from_numpy(preload_images['qpos'][t]).float().cuda()

        images = [PImage.fromarray(arr) if arr is not None else None
                  for arr in image_arrs]

        # for i, pos in enumerate(['f', 'r', 'l'] * 2):
        #     images[i].save(f'{t}-{i}-{pos}.png')

        # get last qpos in shape [7, ]
        proprio = observation_window[-1]['qpos']
        # unsqueeze to [1, 7]
        proprio = proprio.unsqueeze(0)

        # actions shaped as [1, 64, 14] in format [left, right]
        actions = policy.step(
            proprio=proprio,
            images=images,
            text_embeds=lang_embeddings
        ).squeeze(0).cpu().numpy()
        # print(f"inference_actions: {actions.squeeze()}")

        print(f"Model inference time: {time.time() - time1} s")

        # print(f"Finish inference_thread_fn: t={t}")
        return actions


# Main loop for the manipulation task
def model_inference(args, config, controller):
    global lang_embeddings

    # Load rdt model
    policy = make_policy(args)

    lang_dict = torch.load(args.lang_embeddings_path)
    print(f"Running with instruction: \"{lang_dict['instruction']}\" from \"{lang_dict['name']}\"")
    lang_embeddings = lang_dict["embeddings"]

    max_publish_step = config['episode_len']
    chunk_size = config['chunk_size']

    # Initialize the previous action to be the initial robot state
    action = None
    ptr = 0
    chunk_size = 4 #动作分块大小
    # Inference loop
    with torch.inference_mode():
        while True:
            # The current time step
            t = 0

            action_buffer = np.zeros([chunk_size, config['state_dim']])

            while t < max_publish_step:
                # Update observation window
                update_observation_window(args, config, controller)

                # When coming to the end of the action chunk
                if t % chunk_size == 0:
                    # Start inference
                    action_buffer = inference_fn(args, config, policy, t).copy()
                # print("action_buffer:")
                # print(action_buffer)
                raw_action = action_buffer[t % chunk_size]

                # raw_action = np.zeros(7)
                # for i in range(4):
                #     raw_action+=action_buffer[ptr]
                #     ptr+= 1
                action = raw_action
                # print("raw action:")
                # print(action)
                # Interpolate the original action sequence
                if args.use_actions_interpolation:
                    # print(f"Time {t}, pre {pre_action}, act {action}")
                    interp_actions = interpolate_action(args, pre_action, action)
                else:
                    interp_actions = action[np.newaxis, :]
                # Execute the interpolated actions one by one
                for act in interp_actions:
                    right_arm_action = act[:7]
                    # pdb.set_trace()
                    controller.right_arm_controller.move(right_arm_action)
                    # print(f"doing action: {act}")
                    time.sleep(0.1)
                t += 1

                print("Published Step", t)
                pre_action = action.copy()


def get_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--max_publish_step', action='store', type=int,
                        help='Maximum number of action publishing steps', default=10000, required=False)
    parser.add_argument('--seed', action='store', type=int,
                        help='Random seed', default=None, required=False)

    parser.add_argument('--img_front_topic', action='store', type=str, help='img_front_topic',
                        default='/camera_f/color/image_raw', required=False)
    parser.add_argument('--img_left_topic', action='store', type=str, help='img_left_topic',
                        default='/camera_l/color/image_raw', required=False)
    parser.add_argument('--img_right_topic', action='store', type=str, help='img_right_topic',
                        default='/camera_r/color/image_raw', required=False)

    parser.add_argument('--img_front_depth_topic', action='store', type=str, help='img_front_depth_topic',
                        default='/camera_f/depth/image_raw', required=False)
    parser.add_argument('--img_left_depth_topic', action='store', type=str, help='img_left_depth_topic',
                        default='/camera_l/depth/image_raw', required=False)
    parser.add_argument('--img_right_depth_topic', action='store', type=str, help='img_right_depth_topic',
                        default='/camera_r/depth/image_raw', required=False)

    parser.add_argument('--puppet_arm_left_cmd_topic', action='store', type=str, help='puppet_arm_left_cmd_topic',
                        default='/master/joint_left', required=False)
    parser.add_argument('--puppet_arm_right_cmd_topic', action='store', type=str, help='puppet_arm_right_cmd_topic',
                        default='/master/joint_right', required=False)
    parser.add_argument('--puppet_arm_left_topic', action='store', type=str, help='puppet_arm_left_topic',
                        default='/puppet/joint_left', required=False)
    parser.add_argument('--puppet_arm_right_topic', action='store', type=str, help='puppet_arm_right_topic',
                        default='/puppet/joint_right', required=False)

    parser.add_argument('--robot_base_topic', action='store', type=str, help='robot_base_topic',
                        default='/odom_raw', required=False)
    parser.add_argument('--robot_base_cmd_topic', action='store', type=str, help='robot_base_topic',
                        default='/cmd_vel', required=False)
    parser.add_argument('--use_robot_base', action='store_true',
                        help='Whether to use the robot base to move around',
                        default=False, required=False)
    parser.add_argument('--publish_rate', action='store', type=int,
                        help='The rate at which to publish the actions',
                        default=30, required=False)
    parser.add_argument('--ctrl_freq', action='store', type=int,
                        help='The control frequency of the robot',
                        default=25, required=False)

    parser.add_argument('--chunk_size', action='store', type=int,
                        help='Action chunk size',
                        default=64, required=False)
    parser.add_argument('--arm_steps_length', action='store', type=float,
                        help='The maximum change allowed for each joint per timestep',
                        default=[0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.2], required=False)

    parser.add_argument('--use_actions_interpolation', action='store_true',
                        help='Whether to interpolate the actions if the difference is too large',
                        default=False, required=False)
    parser.add_argument('--use_depth_image', action='store_true',
                        help='Whether to use depth images',
                        default=False, required=False)

    parser.add_argument('--disable_puppet_arm', action='store_true',
                        help='Whether to disable the puppet arm. This is useful for safely debugging', default=False)

    parser.add_argument('--config_path', type=str, default="configs/base.yaml",
                        help='Path to the config file')
    # parser.add_argument('--cfg_scale', type=float, default=2.0,
    #                     help='the scaling factor used to modify the magnitude of the control features during denoising')
    parser.add_argument('--pretrained_model_name_or_path', type=str, required=True,
                        help='Name or path to the pretrained model')

    parser.add_argument('--lang_embeddings_path', type=str, required=True,
                        help='Path to the pre-encoded language instruction embeddings')

    args = parser.parse_args()
    return args


def main():
    args = get_arguments()
    controller = Controller()
    if args.seed is not None:
        set_seed(args.seed)
    config = get_config(args)
    model_inference(args, config, controller)


if __name__ == '__main__':
    main()
