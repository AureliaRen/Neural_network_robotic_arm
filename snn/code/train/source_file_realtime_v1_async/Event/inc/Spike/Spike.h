/*
* 文件名：Spike.h
* 定义了一个Spike类，用于表示一个脉冲放电事件，该类继承自Event类。
*/

#ifndef SPIKE_H
#define SPIKE_H

#include "../source_file_realtime_v1_async/Event/inc/Event.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
class Simulation;

class Spike : public Event {
    public:
        Neuron* SourceNeuron; // 发出脉冲的神经元

        /*
        * 默认构造函数
        */
        Spike();

        /*
        * 榛樿鏋愭瀯鍑芥暟
        */
        ~Spike();

        /*
        * 带参数的构造函数
        * SourceNeuron: 鍙戝嚭鑴夊啿鐨勭缁忓厓
        * time: 脉冲发生的时间
        * QueueIndex: 脉冲的事件队列索引
        */
        Spike(Neuron* SourceNeuron, int time, int QueueIndex);

        /*
        * 处理脉冲事件的纯虚函数
        */
        virtual void ProcessEvent(Simulation* simulation) = 0;


        virtual enum EventPriority getPriority() = 0;
};


#endif
