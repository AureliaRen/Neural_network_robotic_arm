/*
* 文件名：InputSpikeDriver.h
* 定义了外界脉冲输入设备的基类
*/

#ifndef INPUTSPIKEDRIVER_H
#define INPUTSPIKEDRIVER_H

#include "../source_file_realtime_v1_async/EventQueue/inc/EventQueue.h"
#include "../source_file_realtime_v1_async/Network/inc/Network.h"

class InputSpikeDriver {
	public:
		bool isFinished; //标记输入是否完成

		InputSpikeDriver() {
            isFinished = false;
		}

		~InputSpikeDriver(){}//默认析构函数

		virtual void LoadInputSpike(EventQueue* eventQueue, Network* network, int time) = 0;
		
		virtual void LoadInputSpike(EventQueue* eventQueue, Network* network, int spikenum, const int* Times, const int* neuron_index) = 0; //加载输入脉冲(纯虚函数)
};

#endif
