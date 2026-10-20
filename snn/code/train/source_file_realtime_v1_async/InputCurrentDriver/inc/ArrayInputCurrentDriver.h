/*
* 文件名：ArrayInputCurrentDriver.h
* 定义了电流输入设备
*/

#ifndef ARRAY_INPUT_CURRENT_DRIVER_H
#define ARRAY_INPUT_CURRENT_DRIVER_H

#include "../source_file_realtime_v1_async/InputCurrentDriver/inc/InputCurrentDriver.h"
#include "../source_file_realtime_v1_async/EventQueue/inc/EventQueue.h"

class ArrayInputCurrentDriver : public InputCurrentDriver {
	public:
		/*
		* 构造函数
		*/
		ArrayInputCurrentDriver();

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~ArrayInputCurrentDriver();

		/*
		* 加载输入电流
		* eventQueue: 事件队列
		* network: 网络对象
		* currentnum: 输入电流数量
		* Times: 输入电流时间队列
		* neuron_index: 输入电流对应的神经元索引队列
		* current: 输入电流队列
		*/
		virtual void LoadInputCurrent(EventQueue* eventQueue, Network* network, int currentnum, const int* Times, const int* neuron_index, const float* current);
};



#endif // ARRAY_INPUT_CURRENT_DRIVER_H

