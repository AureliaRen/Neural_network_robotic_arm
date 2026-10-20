/*
* 文件名：OutputSpikeDriver.h
* 定义了OutputSpikeDriver的基类
*/

#ifndef OUTPUTSPIKEDRIVER_H
#define OUTPUTSPIKEDRIVER_H

class Spike;
class Neuron;

class OutputSpikeDriver {
	public:

		/*
		* 析构函数
		*/
		virtual ~OutputSpikeDriver();

		/*
		* 纯虚函数，向Buffer中写入脉冲信息
		*/
		virtual void WriteSpike(Spike* NewSpike, float basetimesteps) = 0;

		/*
		* 纯虚函数，写入状态
		*/
		virtual void WriteState(int Time, Neuron* Source) = 0;

		/*
		 * 纯虚函数，是否存在缓冲区
		 */
		virtual bool IsBuffered() = 0;

		/*
		 * 纯虚函数，是否可以被写入电压
		 */
		virtual bool IsWritePotentialCapable() = 0;

		/*
		* 虚函数，发布脉冲信息
		*/
		virtual void FlushBuffers() = 0;


};



#endif // !OUTPUTSPIKEDRIVER_H