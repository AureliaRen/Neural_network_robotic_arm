/*
* 文件名：InputCurrentDriver.h
* 定义了输入电流驱动器基类
*/
#ifndef INPUTCURRENTDRIVER_H
#define INPUTCURRENTDRIVER_H

#include "../source_file_realtime_v1_async//EventQueue/inc/EventQueue.h"
#include "../source_file_realtime_v1_async/Network/inc/Network.h"

class InputCurrentDriver {
	public:
		bool isFinished;

		InputCurrentDriver() {
			isFinished = false;
		}

		~InputCurrentDriver(){}

		/*
		* 加载输入电流
		* eventQueue: 事件队列
		* network: 网络对象
		* currentnum: 输入电流数量
		* Times: 输入电流时间队列
		* neuron_index: 输入电流对应的神经元索引队列
		* current: 输入电流队列
		*/

		virtual void LoadInputCurrent(EventQueue* eventQueue, Network* network, int currentnum, const int* Times, const int* neuron_index, const float* current) = 0;
};

#endif