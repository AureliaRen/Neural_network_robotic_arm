/*
* 文件名：InputSpikeNeuronModel.h
* 定义了输入脉冲神经元模型，该模型用于模拟输入脉冲神经元的行为(实际该类没有具体行为，主要用作网络中的占位神经元)。该类继承自EventDrivenInputDevice类。
*/
#ifndef INPUTSPIKENEURONMODEL_H
#define INPUTSPIKENEURONMODEL_H

#include "../source_file_realtime_v1_async/NeuralModel/inc/EventDriven/EventDrivenInputDevice.h"

class InputSpikeNeuronModel : public EventDrivenInputDevice {
    public:

        /*
        * 构造函数
        */
        InputSpikeNeuronModel();

        /*
        * 带参构造函数
        */
        InputSpikeNeuronModel(int timestep);

        /*
        * 鏋愭瀯鍑芥暟
        */
        ~InputSpikeNeuronModel();

        /*
         * 铏氬嚱鏁帮紝鐢ㄤ簬鍒濆鍖栫缁忓厓
         */
        virtual Neuron_State_Vector* InitState() {
            return NULL;
        }

        /*
        * 纯虚函数，用于初始化神经元状态向量
        */
        virtual void InitStateVector(int NumberOfNeurons, int GPUIndex) {

        }

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
        * 虚函数，用于更新神经元模型（没有实际方法）
        */
        virtual void UpdateState(int index, int time, Simulation* simulation) {

        }

        /*
         * 虚函数，用于初始化电流传入（没有实际方法）
        */
        virtual void InitializeInputCurrentSynapseStructure() {

        }

        /*
        * 虚函数，具体实现方法在子类中，用于检查传入突触类型(没有实际方法)
        */
        virtual void CheckType(Interconnections* inter) {

        }

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
        virtual enum NeuronModelType getNeuronModelType() {
            return INPUT_DEVICE;
        }

        virtual bool compare(NeuronModel* neuronModel) {
            if (!EventDrivenInputDevice::compare(neuronModel)) {
                return false;
            }
            InputSpikeNeuronModel* e = dynamic_cast<InputSpikeNeuronModel*>(neuronModel);
            if (e == NULL) {
                return false;
            }
            return true;
        };

        
        virtual std::map<std::string, boost::any> getParameters() {
            return std::map<std::string, boost::any>();
        }

};

#endif
