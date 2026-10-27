#include "../source_file_realtime_v1_async/communication/inc/ZMQInputOutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/InputSpike.h"
//设置安全上限
static const uint32_t MAX_SPIKES_PER_BATCH = 10'000'000;

ZMQInputOutputSpikeDriver::ZMQInputOutputSpikeDriver(enum DriverType Type, std::string server_address, unsigned short tcp_port):connection_type(Type) {
	if (Type == SERVER) {//服务器模式
		this->socket_ = new ZmqSocket(ZmqSocket::Mode::REPLY, "", static_cast<int>(tcp_port));
	}
	else if (Type == CLIENT) {//客户端模式
		this->socket_ = new ZmqSocket(ZmqSocket::Mode::REQUEST, server_address, static_cast<int>(tcp_port));
	}
	this->isFinished = false;


}

ZMQInputOutputSpikeDriver::~ZMQInputOutputSpikeDriver() {
	delete this->socket_;
}

void ZMQInputOutputSpikeDriver::LoadInputSpike(EventQueue* eventQueue, Network* network, int time) {
	
		try {
			//将时间转换为uint32_t类型
			uint32_t time_step = static_cast<uint32_t>(time);
			// 计算缓冲区大小
			uint32_t output_count = static_cast<uint32_t>(this->OutputBuffer.size());

			if (this->OutputBuffer.size() > MAX_SPIKES_PER_BATCH) {
				throw std::runtime_error("Spike count exceeds MAX_SPIKES_PER_BATCH");
			}

			// 计算总请求大小
			size_t request_size = sizeof(time_step) + sizeof(output_count);
			if (output_count > 0) {
				request_size += output_count * sizeof(OutputSpikeIO);
			}

			//填充请求缓冲区
			std::vector<char> request_buf(request_size);
			char* ptr = request_buf.data();

			std::memcpy(ptr, &time_step, sizeof(time_step));
			ptr += sizeof(time_step);
			std::memcpy(ptr, &output_count, sizeof(output_count));
			ptr += sizeof(output_count);

			if (output_count > 0) {
				std::memcpy(ptr, this->OutputBuffer.data(), output_count * sizeof(OutputSpikeIO));
			}

			//发送请求
			this->socket_->sendBuffer(request_buf.data(), static_cast<int>(request_size));

			//接收响应
			uint32_t input_count = 0;
			int bytes = this->socket_->receiveBuffer(&input_count, sizeof(input_count));
			//进行格式检查
			if (bytes != static_cast<int>(sizeof(input_count))) {
				throw std::runtime_error("Failed to receive input spike count");
			}
			//进行安全检查
			if (input_count > MAX_SPIKES_PER_BATCH) {
				throw std::runtime_error("Input spike batch too large");
			}
			//解析脉冲信息
			if (input_count > 0) {
				std::vector<OutputSpikeIO> input_spikes(input_count);
				size_t data_size = input_count * sizeof(OutputSpikeIO);
				//接收脉冲信息
				bytes = this->socket_->receiveBuffer(input_spikes.data(), static_cast<int>(data_size));
				if (static_cast<size_t>(bytes) != data_size) {
					throw std::runtime_error("Incomplete input spike data");
				}

				//将脉冲信息存入队列
				for (const auto& spike : input_spikes) {
					if (spike.neuron < 0 || spike.neuron >= network->neuronsNum) {
						continue;
					}
					Neuron* target_neuron = &(network->neurons[spike.neuron]);
					int Time = spike.time / spike.basetimestepl;
					if (target_neuron) {
						InputSpike* Spike = new InputSpike(&(network->neurons[spike.neuron]), Time, network->neurons[spike.neuron].Queue_index);
						eventQueue->Insert_a_Event(Spike, Spike->getIndex());
					}
				}
			}


			
		}
		catch (std::exception& e) {
			throw;
		}
	

}

void ZMQInputOutputSpikeDriver::LoadInputSpike(EventQueue* eventQueue, Network* network, int spikenum, const int* Times, const int* neuron_index) {
	return;
}

void ZMQInputOutputSpikeDriver::WriteSpike(Spike* NewSpike, float basetimesteps) {
	if (!NewSpike) return;
		
	//创建一个OutputSpike结构体
	OutputSpikeIO out_spike(NewSpike->SourceNeuron->Neuron_index, NewSpike->getTime(), basetimesteps);

    // 进行写入线程保护(只让一个线程写入
    #pragma omp critical(ZMQInputOutputSpikeDriver_WriteSpike)
	{
	   this->OutputBuffer.push_back(out_spike);
	}


}

void ZMQInputOutputSpikeDriver::WriteState(int Time, Neuron* Source) {
	return;
}

bool ZMQInputOutputSpikeDriver::IsBuffered() {
	return true;
}

bool ZMQInputOutputSpikeDriver::IsWritePotentialCapable() {
	return false;
}

void ZMQInputOutputSpikeDriver::FlushBuffers() {
	
	if(this->OutputBuffer.empty()) return; //判断是否为空

	try {

		this->OutputBuffer.clear();


	}
	catch (std::exception& e) {
		throw;
	}

	
}
