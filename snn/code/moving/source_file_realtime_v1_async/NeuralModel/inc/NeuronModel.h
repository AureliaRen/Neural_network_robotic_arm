/*
* 文件名：NeuronModel.h
* 定义了神经元模型的基类，包括神经元模型的基本属性和操作，同一类型的神经元将会公用一个NeuronModel实例
*/
#ifndef NEURONMODEL_H
#define NEURONMODEL_H

#include <string>
#include <string.h>
#include <map>
#include <boost/any.hpp>
class Neuron_State_Vector;
class Neuron;
class Interconnections;
class InternalSpike;
class Simulation;
class InputSpikeDriver;
#include "NeuronModelPropogationStructure.h"

enum NeuronModelType { INPUT_DEVICE, NEURAL_LAYER, COM_INPUT_DEVICE };


class NeuronModel {
    private:
        
        std::string model_name; //模型的名称

        int timestep_size; //基准时间步长的倍数

    public:

        Neuron_State_Vector* StateVector; //神经元状态向量

        NeuronModelPropogationStructure* PropogationStructure; //传播结构

        bool TimeDriven = false; //是否为时间驱动模型

        bool IsGPU = false; //是否为GPU模型

        int NumberOfNeuron; //神经元数量

        int Variable_count; //突触的类型总数

        /*
        * 无参构造函数
        */

        NeuronModel();

        /*
        * 有参构造函数
        */
        NeuronModel(int timestep_size);

        /*
        * 析构函数,用到了多态，所以需要声明为虚函数
        */
        virtual ~NeuronModel();

        /*
        * 关键函数。定义了一个纯虚函数，其具体实现方法在子类中，用于处理传入的脉冲事件
        * inter:传入脉冲事件的连接指针
        * time:传入脉冲事件的时间
        * 
        */
        virtual InternalSpike* ProcessSpike(Interconnections* inter, int time) = 0;

        /*
        * 关键函数。定义了一个纯虚函数，其具体实现方法在子类中，用于处理传入的电流事件
        * inter:传入电流事件的连接指针
        * current:传入电流的大小
        */
        virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current) = 0;

        /*
        * 意义了一个比较函数，用于比较两个神经元模型是否相同
        */
        virtual bool compare(NeuronModel* neuronmodel) {
            return this->model_name == neuronmodel->getModelName();
        }

        /*
        * 获取时间步大小
        */
        int getTimestepSize();

        /*
        * 设置时间步大小
        */
        void setTimestepSize(int timestepsize);


        /*
        * 纯虚函数，具体实现方法在子类中，用于初始化神经元模型
        */
        virtual Neuron_State_Vector* InitState() = 0;

        /*
        * 纯虚函数，具体实现方法在子类中，用于更新神经元模型
        */
        virtual void UpdateState(int index,int time, Simulation* simulation) = 0;

        /*
        * 纯虚函数，具体实现方法在子类中，用于初始化StateVector
        * NumberOfNeurons:神经元数量
        * GPUIndex:GPU索引
        */
        virtual void InitStateVector(int NumberOfNeurons, int GPUIndex) = 0;

        /*
        * 设置模型名称
        * name:模型名称
        */
        void setModelName(std::string name);

        /*
        * 获取模型名称
        */
        std::string getModelName();

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
        virtual int getV_index() = 0;

        /*
        * 虚函数，用于返回每个神经元中变量的数目
        */
        virtual int get_NumberOfState() = 0;

        /*
        * 纯虚函数，返回神经元模型类型
        */
        virtual enum NeuronModelType getNeuronModelType() = 0;

        /*
        * 纯虚函数，获取神经元模型的参数
        */
        virtual std::map<std::string, boost::any> getParameters() = 0;

        /*
        * 虚函数，获取InputSpikeDriver
        */
        virtual InputSpikeDriver* getInputSpikeDriver() {
            return NULL;
        }

        /*
        * 虚函数，设置InputSpikeDriver
        */
        virtual void setInputSpikeDriver(InputSpikeDriver* inputspikedriver) {

        }



};
#endif
