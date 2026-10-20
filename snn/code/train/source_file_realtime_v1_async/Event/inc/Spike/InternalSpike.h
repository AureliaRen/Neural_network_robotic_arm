/*
* 文件名：InternalSpike.h
* 定义了InternalSpike类，用于处理网络内部脉冲，即神经元发出的放电，该类继承自Spike类
*/

#ifndef INTERNALSPIKE_H
#define INTERNALSPIKE_H

#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"


class InternalSpike : public Spike {
	public:

		/*
		* 无参数构造函数
		*/
		InternalSpike();

		/*
		* 带参数构造函数
		*/
		InternalSpike(Neuron* neuron, int time, int QueueIndex);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~InternalSpike();

		/*
		* 定义一个纯虚函数用于处理事件
		*/
		virtual void ProcessEvent(Simulation* simulation)=0;

		/*
		* 查询事件优先级
		*/
		virtual enum EventPriority getPriority()=0;


};


#endif