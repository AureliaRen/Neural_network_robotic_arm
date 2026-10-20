/*
* 文件名：PropogatedSpikeGroup.h
* 定义了一个PropogatedSpikeGroup类，用于表示一个神经元模型的传播脉冲组，继承自Spike类
* 我们将一个NeuronModel在同一时刻产生的且延迟相同的所有脉冲传播定义为一个传播组，集成为一个事件以节约内存
*/

#ifndef PROPAGATEDSPIKEGROUP_H
#define PROPAGATEDSPIKEGROUP_H

#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"
class Interconnections;

class PropogatedSpikeGroup : public Spike {

    public:
        static const int MaxSize = 1024; //初始定义的最大传播组大小，即同时放电神经元的最大数量

        int N_Elements; //传播组中脉冲的数量,即同时放电神经元的数量

        int N_ConnectionsWithEqualDelay[MaxSize]; //每个脉冲的延迟相等的连接数量，即同时放电神经元的连接数量

        Interconnections* ConnectionsWithEqualDelay[MaxSize]; //每个脉冲的延迟相等的首个连接的指针


        /*
        * 构造函数，创建传播组
        */
        PropogatedSpikeGroup(int time, int QueueIndex);

        /*
        * 鏋愭瀯鍑芥暟锛岄噴鏀句紶鎾粍
        */
        ~PropogatedSpikeGroup();

        /*
        * 添加一个新的放电源神经元
        * NumberOfConnections: 鏂版斁鐢垫簮绁炵粡鍏冪殑鐩哥瓑寤惰繜杩炴帴鏁伴噺
        * ConnectionsWithEqualDelay: 鏂版斁鐢垫簮绁炵粡鍏冪殑鐩哥瓑寤惰繜杩炴帴鐨勯涓繛鎺ョ殑鎸囬拡
        * 杩斿洖鍊硷細N_Elements鏄惁绛変簬MaxSize
        */
        bool IncludeNewSourceNeuron(int NuberOfConnections, Interconnections* ConnectionsWithEqualDelay);

        /*
        * 璇ョ被鐨勬牳蹇冨姛鑳斤紝澶勭悊浜嬩欢
        * 
        */
        virtual void ProcessEvent(Simulation* simulation);

        virtual void ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level);

        /*
        * 查询事件优先级
        */
        virtual enum EventPriority getPriority();


};

#endif

