/*
* 文件名：STDP.h
* 定义了STDP类，用于模拟神经元的STDP学习规则，继承自WithPostSynaptic类
*/

#ifndef STDP_H
#define STDP_H
#include <map>
#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/WithPostSynaptic.h"
class Interconnections;
class STDP :public WithPostSynaptic {
    public:

        float MaxLTP = 0.92; //LTP最大值

        float LTP_tau = 16.8; //LTP琛板噺鏃堕棿甯告暟

        float MaxLTD = 0.53; //LTD最大值

        float LTD_tau = 33.1; //LTD琛板噺鏃堕棿甯告暟

        /*
        * 构造函数
        */
        STDP(std::map<std::string, boost::any> parametermap);

        /*
        * 鏋愭瀯鍑芥暟
        */
        virtual ~STDP();

        /*
        * 初始化状态
        */
        virtual void InitState(int NumberOfConnections, int NumberOfState, float basetimestep);

        /*
        * 当连接前脉冲到达时，调用该函数,计算LTP和LTD
        */
        virtual void ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation);

        /*
        * 褰撹繛鎺ュ悗鑴夊啿鍒拌揪鏃讹紝璋冪敤璇ュ嚱鏁帮紝璁＄畻LTP鍜孡TD
        */
        virtual void ApplyPostSynaticSpike(Neuron* neuron, int SpikeTime, Simulation* simulation);

        /*
        * 璁剧疆鍙傛暟
        * 参数：parametermap，参数字典
        */
        void SetParameters(std::map<std::string, boost::any> parametermap);

     
        /*
        * 鑾峰彇妯″瀷鍙傛暟
        */
        virtual std::map<std::string, boost::any> GetParameters();

};



#endif // STDP_H