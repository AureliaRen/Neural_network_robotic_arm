/*
* 文件名：PropogateCurrent.h
* 用于传播电流输入事件
*/

#ifndef PROPAGATECURRENT_H
#define PROPAGATECURRENT_H

#include "../source_file_realtime_v1_async/Event/inc/Current/Current.h"

class PropogatedCurrent : public Current {
	public:
        int PropogationDelayIndex; //当前传播延迟索引

        int UpperBoundDelayIndex; //最后一个传播延迟索引(对应一个神经元的不同延迟数目)

        int NSynapses; //该延迟对应的突触数目

        Interconnections* inter; //首个突触连接

        /*
        * 默认构造函数
        */
        PropogatedCurrent();

        /*
        * 构造函数
        */
        PropogatedCurrent(int time, int QueueIndex, Neuron* neuron, int PropogationDelayIndex, int UpperBoundDelayIndex, float current);

        /*
        * 析构函数
        */
        ~PropogatedCurrent();

        /*
        * 澶勭悊浜嬩欢
        */
        virtual void ProcessEvent(Simulation* simulation);


        /*
        * 查询事件优先级
        */
        virtual enum EventPriority getPriority();


};


#endif // PROPAGATECURRENT_H
