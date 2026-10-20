/*
* 文件名：FileOutputWeightDriver.h
* 输出权重类,继承自OutputWeightDriver
*/
#ifndef FILEOUTPUTWEIGHTDRIVER_H	
#define FILEOUTPUTWEIGHTDRIVER_H

#include "../source_file_realtime_v1_async/communication/inc/OutputWeightDriver.h"
#include <string>

class FileOutputWeightDriver : public OutputWeightDriver {
	public:
		std::string filename;


		/*
		* 构造函数
		* @newfilename: 文件名
		*/
		FileOutputWeightDriver(const char* newfilename);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~FileOutputWeightDriver();

		/*
		* 杈撳嚭鏉冮噸
		* @pNetwork: 缃戠粶鎸囬拡
		* @time: 鏃堕棿
		*/
		virtual void WriteWeight(Network* pNetwork, int time);



};

#endif
