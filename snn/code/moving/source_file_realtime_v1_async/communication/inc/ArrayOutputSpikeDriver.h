/*
* 文件名: ArrayOutputSpikeDriver.h
* 定义了脉冲记录器类，用于将脉冲计入内存向量
*/

#ifndef ARRAY_OUTPUT_SPIKE_DRIVER_H
#define ARRAY_OUTPUT_SPIKE_DRIVER_H

#include "../source_file_realtime_v1_async/communication/inc/OutputSpikeDriver.h"
#include <vector>
class Spike;
class Neuron;

class ArrayOutputSpikeDriver :public OutputSpikeDriver{
	private:
		//定义了一个结构体，用于记录脉冲信息
		struct OutputSpike {
			int neuron;

			int time;

			float basetimestepl;

			OutputSpike() = default;

			OutputSpike(int neuronID, int spiketime, float basetimestep):neuron(neuronID), time(spiketime), basetimestepl(basetimestep) {};

		};


     public:
		 //瀹氫箟浜嗕竴涓悜閲忥紝鐢ㄤ簬瀛樺偍鑴夊啿淇℃伅
		 std::vector<OutputSpike> OutputBuffer;

		 //构造函数
		 ArrayOutputSpikeDriver();

		 //鏋愭瀯鍑芥暟
		 ~ArrayOutputSpikeDriver();

		 /*
		 * 向Buffer中写入脉冲信息
		 * NewSpike: 新脉冲信息
		 */
		 virtual void WriteSpike(Spike* NewSpike, float basetimestep);

		 /*
		 * 写入状态信息
		 */
		 virtual void WriteState(int Time, Neuron* Source);


		 /*
		 * 鏄惁鍙鍐欏叆鑴夊啿
		 */
		 virtual bool IsBuffered();

		 /*
		 * 是否可以被写入电压
		 */
		 virtual bool IsWritePotentialCapable();

		 /*
		 * 鐢ㄤ簬鑾峰彇缂撳啿鍖轰腑鑴夊啿锛坰pikes锛夌殑鍑芥暟
		 */
		 int GetBufferedSpikes(int*& Times, int*& Cells);

		 /*
		 * 鐢ㄤ簬绉婚櫎缂撳啿鍖虹殑鑴夊啿鍑芥暟
		 */
		 bool RemoveBufferedSpike(int& Time, int& Cell);

		 /*
		 * 虚函数，发布缓冲区内的脉冲信息
		 */
		 virtual void FlushBuffers();

		 /*
		 * 清空内存中的脉冲缓冲
		 */
		 void ClearBuffer();



};



#endif // ARRAY_OUTPUT_SPIKE_DRIVER_H
