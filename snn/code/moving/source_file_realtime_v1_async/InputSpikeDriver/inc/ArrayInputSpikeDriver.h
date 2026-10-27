/*
* 文件名：ArrayInputSpikeDriver.h
* 定义了一个类，用于处理外界输入的脉冲驱动(数列形式)
*/

#ifndef ARRAY_INPUT_SPIKE_DRIVER_H
#define ARRAY_INPUT_SPIKE_DRIVER_H

#include "../source_file_realtime_v1_async/EventQueue/inc/EventQueue.h"
#include "../source_file_realtime_v1_async/Network/inc/Network.h"
#include "../source_file_realtime_v1_async/InputSpikeDriver/inc/InputSpikeDriver.h"

class ArrayInputSpikeDriver:public InputSpikeDriver {
	public:

		/*
		* 构造函数
		*/
		ArrayInputSpikeDriver();

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~ArrayInputSpikeDriver();
		
		/*
		* 鍔犺浇杈撳叆鑴夊啿
		* eventQueue:浜嬩欢闃熷垪
		* network:缃戠粶
		* spikenum:鑴夊啿鏁伴噺
		* Times:鑴夊啿鏃堕棿
		* neuron_index:鑴夊啿瀵瑰簲鐨勭缁忓厓绱㈠紩
		*/
		virtual void LoadInputSpike(EventQueue* eventQueue, Network* network, int spikenum, const int* Times, const int* neuron_index);

		/*
		* 加载输入脉冲函数的重载
		*/
		virtual void LoadInputSpike(EventQueue* eventQueue, Network* network, int time) {
			return;
		};

};

#endif
