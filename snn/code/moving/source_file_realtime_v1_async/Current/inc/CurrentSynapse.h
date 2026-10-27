/*
* 文件名: CurrentSynapse.h
* 定义了一个电流突触类，用于处理对神经元的电流输入
*/

#ifndef CURRENTSYNAPSE_H
#define CURRENTSYNAPSE_H
class CurrentSynapse {
    public:
        //电流传入的目标神经元个数
        int N_target_neurons;

        //每个传入神经元的连接个数
        int* N_connections;

        //每个传入连接的传入电流
        float** currents_per_connection;

        /*
        * 默认构造函数
        */
        CurrentSynapse();

        /*
        * 带参构造函数
        * N_target_neurons:目标神经元的个数
        */
        CurrentSynapse(int N_target_neurons);

        /*
        * 析构函数
        */
        ~CurrentSynapse();

        /*
        * N_connections计数器
        * neuron_index:目标神经元索引
        */
        void IncrementNInputCurrentSynapsesPerNeuron(int neuron_index);

        /*
        * 为currents_per_connection分配内存
        */
        void InitializeInputCurrentPerSynapseStructure();

        /*
        * 设置输入电流，即为currents_per_connection赋值
        */
        void SetInputCurrentPerSynapse(int neuron_index, int syn_index, float current);

        /*
        * 获得每个目标神经元的总电流输入
        * neuron_index:目标神经元索引
        */
        float GetTotalInputCurrentPerNeuron(int neuron_index);


};

#endif
