/*
* 文件名: OutputWeightDriver.h
* 定义了突触权重输出驱动器接口
*/
#ifndef FILE_OUTPUT_WEIGHT_DRIVER_H
#define FILE_OUTPUT_WEIGHT_DRIVER_H
class Network;

class OutputWeightDriver {
	public:
		/*
		* 默认构造函数
		*/
		OutputWeightDriver();

		/*
		* 默认析构函数
		*/
		~OutputWeightDriver();


		/*
		* 写入权重函数
		* @pNetwork: 网络对象
		* @time: 时间
		*/
		virtual void WriteWeight(Network* pNetwork, int time) = 0;
		
};


#endif // FILE_OUTPUT_WEIGHT_DRIVER_H
