/*
* 文件名：ZMQInputOutputSpikeDriver.h
* 通过ZMQ实现对脉冲的输入输出交互管理
*/
#ifndef ZMQINPUTOUTPUTSPIKEDRIVER_H
#define ZMQINPUTOUTPUTSPIKEDRIVER_H

#include "../source_file_realtime_v1_async/communication/inc/OutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/InputSpikeDriver/inc/InputSpikeDriver.h"
#include <vector>
#include "../source_file_realtime_v1_async/communication/inc/ZmqSocket.h"
#include "../source_file_realtime_v1_async/communication/inc/DriverType.h"

class ZMQInputOutputSpikeDriver : public InputSpikeDriver, public OutputSpikeDriver {
	private:
		//定义一个传输结构体用于传输脉冲信息
		struct OutputSpikeIO {
			int neuron;

			float time;

			float basetimestepl;

			OutputSpikeIO() = default;

			OutputSpikeIO(int neuronID, int spiketime, float basetimestep) :neuron(neuronID), time(spiketime* basetimestep), basetimestepl(basetimestep){};

		};

		//socket_成员变量
		ZmqSocket* socket_;

		//Driver的类型
		enum DriverType connection_type;

		//输出脉冲缓冲区
		std::vector<OutputSpikeIO> OutputBuffer;


	public:
		/*
		* 构造函数
		* @type:驱动类型
		* @server_address:服务器地址
		* @tcp_port:端口号
		*/
		ZMQInputOutputSpikeDriver(enum DriverType Type, std::string server_address, unsigned short tcp_port);

		/*
		* 析构函数
		*/
		~ZMQInputOutputSpikeDriver();

		/*
		* 加载输入脉冲
		*/
		virtual void LoadInputSpike(EventQueue* eventQueue, Network* network, int time);

		virtual void LoadInputSpike(EventQueue* eventQueue, Network* network, int spikenum, const int* Times, const int* neuron_index);

		/*
		* 在缓冲区写入脉冲信息
		*/
		virtual void WriteSpike(Spike* NewSpike, float basetimesteps);

		/*
		* 虚函数，写入状态
		*/
		virtual void WriteState(int Time, Neuron* Source);

		/*
		 * 虚函数，是否存在缓冲区
		 */
		virtual bool IsBuffered();

		/*
		 * 虚函数，是否可以被写入电压
		 */
		virtual bool IsWritePotentialCapable();

		/*
		* 清空缓冲区
		*/
		virtual void FlushBuffers();



};



#endif // ZMQINPUTOUTPUTSPIKEDRIVER_H
