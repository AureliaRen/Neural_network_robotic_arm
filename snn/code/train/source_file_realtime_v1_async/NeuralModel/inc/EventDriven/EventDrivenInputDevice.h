/*
* 文件名: EventDrivenInputDevice.h
* 定义了一个事件驱动输入设备类，该类继承自NeuronModel类，用于模拟事件驱动输入设备的行为。
*/

#ifndef EVENTDRIVENINPUTDEVICE_H
#define EVENTDRIVENINPUTDEVICE_H

#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
class EventDrivenInputDevice : public NeuronModel {
    public:
        /*
        * 无参构造函数
        */
        EventDrivenInputDevice();
        /*
        * 有参构造函数
        */
        EventDrivenInputDevice(int timesteps);
        /*
        * 鏋愭瀯鍑芥暟
        */
        ~EventDrivenInputDevice();

        /*
        * 纯虚函数，用于初始化神经元
        */
        virtual Neuron_State_Vector* InitState() = 0;

        /*
        * 纯虚函数，用于初始化神经元状态向量
        */
        virtual void InitStateVector(int NumberOfNeurons, int GPUIndex) = 0;

        /*
        * 虚函数，用于处理输入脉冲(输入设备不接收脉冲输入)
        */
        virtual InternalSpike* ProcessSpike(Interconnections* inter, int time) {
            return NULL;
        }

        /*
        * 虚函数，用于处理输入电流（输入设备不接收电流输入）
        */
        virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current) {

        }

        /*
         * 纯虚函数，具体实现方法在子类中，用于初始化电流传入
        */
        virtual void InitializeInputCurrentSynapseStructure() = 0;

        /*
        * 纯虚函数，具体实现方法在子类中，用于检查传入突触类型
        */
        virtual void CheckType(Interconnections* inter) = 0;

        /*
        * 虚函数，用于返回V的索引
        */
        virtual int getV_index() {
            return -1;
        }

        /*
        * 虚函数，用于返回每个神经元中变量的数目
        */
        virtual int get_NumberOfState() {
            return -1;
        }

        /*
        * 绾櫄鍑芥暟锛岃繑鍥炵缁忓厓妯″瀷绫诲瀷
        */
        virtual enum NeuronModelType getNeuronModelType() = 0;


        /*
        * 瀹氫箟姣旇緝鍑芥暟锛岀敤浜庢瘮杈冧袱涓缁忓厓妯″瀷
        */
        virtual bool compare(NeuronModel* neuronmodel) {
            if (!NeuronModel::compare(neuronmodel)) {
                return false;
            }
            EventDrivenInputDevice* e = dynamic_cast<EventDrivenInputDevice*>(neuronmodel);
            if (e == NULL) {
                return false;
            }
            return true;
        };

        /*
        * 纯虚函数，用于返回神经参数列表
        */
        virtual std::map<std::string, boost::any> getParameters() = 0;


};

#endif // EVENTDRIVENINPUTDEVICE_H
