#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/EventQueue/inc/TimingWheelEventQueue.h"
#include "../source_file_realtime_v1_async/Event/inc/EndSimulationTime.h"
#include "../source_file_realtime_v1_async/Event/inc/TimeEventUpdateNeuron.h"
#include "../source_file_realtime_v1_async/InputSpikeDriver/inc/ArrayInputSpikeDriver.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Event/inc/Synchronize/SynchronizeActivityEvent.h"
#include "../source_file_realtime_v1_async/communication/inc/OutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"
#include <cuda_runtime.h>
#include "../source_file_realtime_v1_async/error/cudaerror.h"
#include "../source_file_realtime_v1_async/Event/inc/Synchronize/SaveWeightEvent.h"
#include "../source_file_realtime_v1_async/communication/inc/DriverType.h"
#include "../source_file_realtime_v1_async/Event/inc/Synchronize/CommunicationEvent.h"
#include "../source_file_realtime_v1_async/Event/inc/Outer/OuterUpdateEvent.h"
#include "../source_file_realtime_v1_async/OuterDynamic/inc/OuterDynamicSpikeBuffer.h"
#include "../source_file_realtime_v1_async/ModelFactory/OuterDynamicModelFactory.h"
#include "../source_file_realtime_v1_async/mainprogram/inc/BenchProfiling.h"
#if SNN_WITH_ZMQ
#include "../source_file_realtime_v1_async/communication/inc/ZMQInputOutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/communication/inc/ZMQAsyncInputOutputSpikeDriver.h"
#endif


void Simulation::AddExternalSpikeActivity(const std::vector<int>& EventTime, const std::vector<int>& EventNeuronID) {
	if (EventTime.size() > 0) {
		this->inputSpikeDriver->LoadInputSpike(this->EventHeap, this->network, EventTime.size(), &EventTime[0], &EventNeuronID[0]);
	}
}

void Simulation::AddExternalCurrentActivity(const std::vector<int>& EventTime, const std::vector<int>& EventNeuronID, const std::vector<float>& Current) {
	if (EventTime.size() > 0) {
		this->inputCurrentDriver->LoadInputCurrent(this->EventHeap, this->network, EventTime.size(), &EventTime[0], &EventNeuronID[0], &Current[0]);
	}
}



void Simulation::EndSimulation(int QueueID) {
	this->SimulationEnd[QueueID] = true;
}


Simulation::Simulation(const std::list<NeuronLayerDescription>& neuron_layer_list, const std::list<ConnectionDescription>& connection_list, const std::list<LearningRuleDescription>& learning_rule_list, int simulationsteps, float timestep, int NumberOfQueue, EventQueueType eventQueueType, int timingWheelSize)
	: Simulation(neuron_layer_list, connection_list, learning_rule_list, std::list<OuterDynamicDescription>(), simulationsteps, timestep, NumberOfQueue, eventQueueType, timingWheelSize) {}

Simulation::Simulation(const std::list<NeuronLayerDescription>& neuron_layer_list, const std::list<ConnectionDescription>& connection_list, const std::list<LearningRuleDescription>& learning_rule_list, const std::list<OuterDynamicDescription>& outer_dynamic_list, int simulationsteps, float timestep, int NumberOfQueue, EventQueueType eventQueueType, int timingWheelSize) :
	network(0), EventHeap(0), totaltimesteps(simulationsteps), basetimesteps(timestep), currenttime(0), NumberOfQueue(0), SimulationEnd(0), inputSpikeDriver(0), inputCurrentDriver(0), output_spike_driver(0), file_output_spike_driver(0), file_output_monitor_driver(0), file_outer_dynamic_state_driver(0), SyncThread(0), PauseThread(0), DelayMin(-1), file_output_weight_driver(0), zmq_input_output_spike_driver(0), zmq_async_input_output_spike_driver(0), WeightSaveInterval(0), CommunivationInterval(0), neuronMonitorExist(false), RealTimeRestrictionObject(0), RealtimeEnabled(false), RealtimeSlotSteps(0), PrintSimulationTime(false), outer_dynamic_spike_buffer(0)
{

	// 设置线程数量
	setNumberOfOpenMPQueues(NumberOfQueue);
	NumberOfQueue = NumberOfOpenMPQueues;
	this->NumberOfQueue = NumberOfOpenMPQueues;
	// 为每个线程分配独立的状态数组
	this->currenttime = new int[NumberOfQueue]();
	this->SimulationEnd = new bool[NumberOfQueue]();
	this->PauseThread = new bool[NumberOfQueue]();
	this->SyncThread = new bool[NumberOfQueue]();

	//创建事件队列
	if (eventQueueType == EVENT_QUEUE_TIMING_WHEEL) {
		int wheelSize = timingWheelSize > 0 ? timingWheelSize : simulationsteps + 1;
		this->EventHeap = new TimingWheelEventQueue(NumberOfQueue, wheelSize);
	}
	else {
		this->EventHeap = new EventQueue(NumberOfQueue);
	}
	//创建网络
	this->network = new Network(neuron_layer_list, connection_list, learning_rule_list, this->NumberOfQueue, this->basetimesteps, this);

	this->neuronMonitorExist = this->network->isMonitor;
	this->outer_dynamic_spike_buffer = new OuterDynamicSpikeBuffer();
	this->CreateOuterDynamic(outer_dynamic_list);

	this->inputSpikeDriver = new ArrayInputSpikeDriver();
	this->inputCurrentDriver = new ArrayInputCurrentDriver();
	this->output_spike_driver = new ArrayOutputSpikeDriver();
	this->RealTimeRestrictionObject = new RealTimeRestriction();
	//获取最小跨线程连接
	this->DelayMin = network->GetMinInterpropagationTime();
}

Simulation::~Simulation() {
	if (this->network != NULL) {
		delete this->network;
	}
	if (this->EventHeap != NULL) {
		delete this->EventHeap;
	}
	if (this->inputSpikeDriver != NULL) {
		delete this->inputSpikeDriver;
	}
	if (this->inputCurrentDriver != NULL) {
		delete this->inputCurrentDriver;
	}
	if (this->output_spike_driver != NULL) {
		delete this->output_spike_driver;
	}
	if (this->file_output_spike_driver != NULL) {
		delete this->file_output_spike_driver;
	}
	if (this->file_output_monitor_driver != NULL) {
		delete this->file_output_monitor_driver;
	}
	if (this->file_outer_dynamic_state_driver != NULL) {
		delete this->file_outer_dynamic_state_driver;
	}
	if (this->file_output_weight_driver != NULL) {
		delete this->file_output_weight_driver;
	}
	for (std::list<OuterDynamicModel*>::iterator it = this->OuterDynamicModelList.begin(); it != this->OuterDynamicModelList.end(); it++) {
		delete *it;
	}
	this->OuterDynamicModelList.clear();
	if (this->outer_dynamic_spike_buffer != NULL) {
		delete this->outer_dynamic_spike_buffer;
	}
	#if SNN_WITH_ZMQ
	if (this->zmq_input_output_spike_driver != NULL) {
		delete this->zmq_input_output_spike_driver;
	}
	if (this->zmq_async_input_output_spike_driver != NULL) {
		delete this->zmq_async_input_output_spike_driver;
	}
	#endif
	if (this->RealTimeRestrictionObject != NULL) {
		delete this->RealTimeRestrictionObject;
	}
	if (this->currenttime != NULL) {
		delete[] this->currenttime;
	}
	if (this->SimulationEnd != NULL) {
		delete[] this->SimulationEnd;
	}
	if (this->PauseThread != NULL) {
		delete[] this->PauseThread;
	}
	if (this->SyncThread != NULL) {
		delete[] this->SyncThread;
	}

	this->InputSpikeDriverList.clear();
	this->InputCurrentDriverList.clear();
	this->OutputSpikeDriverList.clear();
	this->MonitorSpikeList.clear();
	this->WeightDriverList.clear();
	this->OuterDynamicStateDriverList.clear();

}

void Simulation::EnableRealtime(int slotSteps, double maxAdvanceSeconds, float firstSection, float secondSection, float thirdSection) {
	// 开启实时限制
	this->RealtimeEnabled = true;
	//插入第一个Communication事件
	this->RealtimeSlotSteps = (slotSteps > 0) ? slotSteps : ((this->CommunivationInterval > 0) ? this->CommunivationInterval : 1);
	//转化为实时处理的秒数
	const double slotSeconds = static_cast<double>(this->RealtimeSlotSteps) * static_cast<double>(this->basetimesteps)* 0.001;
	// 设置看门狗参数
	this->RealTimeRestrictionObject->SetParameterWatchDog(slotSeconds, maxAdvanceSeconds, firstSection, secondSection, thirdSection);
	this->RealTimeRestrictionObject->SetSleepPeriod(0.01);
}

void Simulation::DisableRealtime() {
	this->RealtimeEnabled = false;
	this->RealtimeSlotSteps = 0;
	if (this->RealTimeRestrictionObject != NULL) {
		this->RealTimeRestrictionObject->StopWatchDog();
	}
}

void Simulation::SetPrintSimulationTime(bool enable) {
	this->PrintSimulationTime = enable;
}




void Simulation::InitSimulation() {

	//this->output_spike_driver = new ArrayOutputSpikeDriver();
	//std::string Filename = "D:/computing_ref/Demo/Python_plot/output_monitor.txt";

	//this->file_output_monitor_driver = new FileOutputSpikeDriver(Filename.c_str(), true);

	AddInputSpikeDriver(this->inputSpikeDriver);
	AddInputCurrentDriver(this->inputCurrentDriver);
	//AddOutputSpikeDriver(this->output_spike_driver);
	if (this->file_output_spike_driver != 0) {
		AddOutputSpikeDriver(this->file_output_spike_driver);
	}
	if (this->file_output_monitor_driver != 0) {
		AddOutputMonitorDriver(this->file_output_monitor_driver);
	}
	if (this->file_output_weight_driver != 0) {
		AddOutputWeightDriver(this->file_output_weight_driver);
	}
	if (this->file_outer_dynamic_state_driver != 0) {
		AddOuterDynamicStateDriver(this->file_outer_dynamic_state_driver);
	}

	for (int i = 0; i < this->NumberOfQueue; i++) {
		//初始化连接时间
		this->currenttime[i] = 0;
		//插入结束事件
		if (this->totaltimesteps > 0) {
			this->EventHeap->Insert_a_Event(new EndSimulationTime(this->totaltimesteps, i), i);
		}
		//插入首个CPU时间驱动神经元的更新事件
		for (int z = 0; z < this->network->neurontypesNum; z++) {
			if (this->network->TimeDrivenNeuronsNum[z][i] > 0) {
				this->EventHeap->Insert_a_Event(new TimeEventUpdateNeuron(0, i, this->network->neurontypes[z][i], this->network->TimeDrivenNeurons[z][i]), i);

			}
		}
		//插入首个GPU时间驱动神经元的更新事件
		for (int z = 0; z < this->network->neurontypesNum; z++) {
			if (this->network->TimeDrivenNeuronsNumGPU[z][i] > 0) {
				this->EventHeap->Insert_a_Event(new TimeEventUpdateNeuron(0, i, this->network->neurontypes[z][i], this->network->TimeDrivenNeuronsGPU[z][i]), i);
			}
		}
		for (std::list<OuterDynamicModel*>::iterator it = this->OuterDynamicModelList.begin(); it != this->OuterDynamicModelList.end(); it++) {
			if ((*it)->getQueueIndex() == i) {
				this->EventHeap->Insert_a_Event(new OuterUpdateEvent(0, i, *it), i);
			}
		}

	}
	//插入第一个线程同步事件
	if (this->DelayMin >= 0) {
		//在同步队列中插入同步事件
		this->EventHeap->Insert_a_syn_Event(new SynchronizeActivityEvent(0, this));
	}

	//插入第一个权重保存事件
	if (this->WeightSaveInterval > 0) {
		this->EventHeap->Insert_a_syn_Event(new SaveWeightEvent(this->WeightSaveInterval, this));
	}

	//插入第一个Communication事件
	if (this->CommunivationInterval > 0) {
		this->EventHeap->Insert_a_syn_Event(new CommunicationEvent(this->CommunivationInterval, this->CommunivationInterval,this));
	}

}

void Simulation::RunSimulationStep(int runsteps) {
	Event* newEvent;
	int time;
	int OpenmpIndex; //线程索引

    #pragma omp parallel num_threads(this->NumberOfQueue) if(this->NumberOfQueue > 1) \
    private(newEvent, OpenmpIndex, time)
	{
		OpenmpIndex = omp_get_thread_num();
        #pragma omp critical
		{
			std::cout << "[OpenMP Thread " << OpenmpIndex
				<< "] of total " << omp_get_num_threads()
				<< " started." << std::endl;
		}
		time = 0.0;
		if (NumberOfGPU > 0) {
			this->SetOpenMPThreadToGPU(OpenmpIndex);
		}
		//初始化EventHeap信号灯变量
		this->SimulationEnd[OpenmpIndex] = false;
		// 插入相对当前仿真时刻的结束事件，而不是重复使用绝对时间 runsteps。
		this->EventHeap->Insert_a_Event(
			new EndSimulationTime(this->currenttime[OpenmpIndex] + runsteps, OpenmpIndex),
			OpenmpIndex);
		//循环执行事件
		while (!this->SimulationEnd[OpenmpIndex]) {
			//判断是否需要同步
			if (this->SyncThread[OpenmpIndex]) {
				//重置线程同步索引
				this->SyncThread[OpenmpIndex] = false;
				//执行线程同步操作
				this->SynchronizeThread();
			}



			//从堆顶获取一个事件
			newEvent = this->EventHeap->Remove_a_Event(OpenmpIndex);
			if (newEvent == NULL) {
				std::cout << "Internal error: empty event queue on thread " << OpenmpIndex << std::endl;
				this->SimulationEnd[OpenmpIndex] = true;
				continue;
			}

			bool skipEvent = false;

			//判断事件是否需要跳过
			if (newEvent->getTime() < this->currenttime[OpenmpIndex]) {
				skipEvent = true;
				std::cout << "Internal error: Bad spike time. Spike: " << newEvent->getTime()*this->basetimesteps << " Current: " << this->currenttime[OpenmpIndex] << std::endl;
			}
			else {
				this->currenttime[OpenmpIndex] = newEvent->getTime();
				if (OpenmpIndex == 0 && this->currenttime[OpenmpIndex] > time) {
					time = this->currenttime[OpenmpIndex];
					if (this->PrintSimulationTime) {
						std::cout << "[SimulationStep] time = "
							<< static_cast<double>(time) * static_cast<double>(this->basetimesteps)
							<< " ms" << std::endl;
					}
				}
			}

			if (!skipEvent) {
				newEvent->ProcessEvent(this);

			}

			delete newEvent;
			newEvent = NULL;
		}

	}

}

void Simulation::RunSimulationRealtime(int runsteps) {
	if (!this->RealtimeEnabled || this->RealTimeRestrictionObject == NULL || this->RealtimeSlotSteps <= 0) {
		this->RunSimulationStep(runsteps);
		return;
	}
	Event* newEvent;
	int OpenmpIndex;
	int time;
	const int realtimeSlotSteps = this->RealtimeSlotSteps;
	// 定义看门狗线程	std::thread watchdogThread;
	// 阻塞等待看门狗线程启动	std::atomic<bool> watchdogStarted(false);

#pragma omp parallel num_threads(this->NumberOfQueue) if(this->NumberOfQueue > 1) private(newEvent, OpenmpIndex, time)
	{
		OpenmpIndex = omp_get_thread_num();
		int nextRealtimeBoundary = realtimeSlotSteps;
		time = 0;
		if (NumberOfGPU > 0) {
			this->SetOpenMPThreadToGPU(OpenmpIndex);
		}
		//初始化线程信号灯变量
		this->SimulationEnd[OpenmpIndex] = false;
		// 插入相对当前仿真时刻的结束事件，而不是重复使用绝对时间 runsteps。
		this->EventHeap->Insert_a_Event(
			new EndSimulationTime(this->currenttime[OpenmpIndex] + runsteps, OpenmpIndex),
			OpenmpIndex);
		//循环执行事件
		while (!this->SimulationEnd[OpenmpIndex]) {
			//获取约束等级
			RealTimeRestrictionLevel restrictionLevel = this->RealTimeRestrictionObject->GetRestrictionLevel();
			if (this->SyncThread[OpenmpIndex]) {
				this->SyncThread[OpenmpIndex] = false;
				this->SynchronizeThread(restrictionLevel);
				restrictionLevel = this->RealTimeRestrictionObject->GetRestrictionLevel();
			}
	//逐一在InputSpikeDriverList中加载输入
			newEvent = this->EventHeap->Remove_a_Event(OpenmpIndex);
			if (newEvent == NULL) {
				this->SimulationEnd[OpenmpIndex] = true;
				continue;
			}
			//判断事件是否合法
			bool skipEvent = false;
			if (newEvent->getTime() < this->currenttime[OpenmpIndex]) {
				skipEvent = true;
				std::cout << "Internal error: Bad spike time. Spike: " << newEvent->getTime() << " Current: " << this->currenttime[OpenmpIndex] << std::endl;
			}//打印当前时间
			else {
				this->currenttime[OpenmpIndex] = newEvent->getTime();
				if (OpenmpIndex == 0 && this->currenttime[OpenmpIndex] > time) {
					time = this->currenttime[OpenmpIndex];
					if (this->PrintSimulationTime) {
						std::cout << "[SimulationRealtime] time = "
							<< static_cast<double>(time) * static_cast<double>(this->basetimesteps)
							<< " ms" << std::endl;
					}
				}
			}

			if (OpenmpIndex == 0) {
				while (this->currenttime[0] >= nextRealtimeBoundary) {
					//判断是否需要启动看门狗线程
					if (!watchdogStarted.load()) {
						this->RealTimeRestrictionObject->StartWatchDog();
						watchdogThread = std::thread([this]() {
							this->RealTimeRestrictionObject->Watchdog();
						});
						//标记看门狗已启动
						watchdogStarted.store(true);
					}
					this->RealTimeRestrictionObject->NextStepWatchDog();
					nextRealtimeBoundary += realtimeSlotSteps;
				}
			}

			while (watchdogStarted.load() &&
				this->RealTimeRestrictionObject->GetRestrictionLevel() == SIMULATION_TOO_FAST) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			if (!skipEvent) {
				//获取约束等级
				restrictionLevel = this->RealTimeRestrictionObject->GetRestrictionLevel();
				newEvent->ProcessEvent(this, restrictionLevel);
			}

			delete newEvent;
			newEvent = NULL;
		}
	}

	if (watchdogStarted.load()) {
		this->RealTimeRestrictionObject->StopWatchDog();
	}
	if (watchdogThread.joinable()) {
		watchdogThread.join();
	}
}

void Simulation::SynchronizeThread() {
    const long long start_ns = bench_profile::now_ns();
	//定义一个屏障同步指令
#pragma omp barrier
//定义单线程执行指令
#pragma omp single
	{
		if (!this->PauseThread[0] && !this->SimulationEnd[0]) {
			Event* event = this->EventHeap->Remove_a_syn_Event();
			if (event != NULL) {
				event->ProcessEvent(this);
				delete event;
			}
		}
	}

    bench_profile::sync_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
    bench_profile::sync_count.fetch_add(1, std::memory_order_relaxed);
}

void Simulation::SynchronizeThread(RealTimeRestrictionLevel restrictionLevel) {
    const long long start_ns = bench_profile::now_ns();
#pragma omp barrier
#pragma omp single
	{
		if (!this->PauseThread[0] && !this->SimulationEnd[0]) {
			Event* event = this->EventHeap->Remove_a_syn_Event();
			if (event != NULL) {
				event->ProcessEvent(this, restrictionLevel);
				delete event;
			}
		}
	}
    bench_profile::sync_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
    bench_profile::sync_count.fetch_add(1, std::memory_order_relaxed);
}


void Simulation::WriteSpike(Spike* spike) {
	Neuron* neuron = spike->SourceNeuron;
	//判断是否被监视
	if (neuron->IsMonitor) {
		//写入监视数据
		for (std::list<OutputSpikeDriver*>::iterator it = this->MonitorSpikeList.begin(); it != this->MonitorSpikeList.end(); it++) {
			if (!(*it)->IsWritePotentialCapable()) {//是否可以写入
				(*it)->WriteSpike(spike, this->basetimesteps);
			}
		}
	}

	//判断是否为输出神经元
	if (neuron->IsOutput) {
		if (this->outer_dynamic_spike_buffer != NULL) {
			this->outer_dynamic_spike_buffer->RecordSpike(spike->getTime(), neuron->Neuron_index);
		}
		for (std::list<OutputSpikeDriver*>::iterator it = this->OutputSpikeDriverList.begin(); it != this->OutputSpikeDriverList.end(); it++) {
			
			(*it)->WriteSpike(spike, this->basetimesteps);
			
		}
	}

}

void Simulation::WriteState(int time, Neuron* neuron) {
	if (neuron->IsMonitor) {
		for (std::list<OutputSpikeDriver*>::iterator it = this->MonitorSpikeList.begin(); it != this->MonitorSpikeList.end(); it++) {
			if ((*it)->IsWritePotentialCapable()) {//是否可以写入
				(*it)->WriteState(time, neuron);
			}
		}
	}
}

void Simulation::SetOpenMPThreadToGPU(int threadNum) {
	if (NumberOfGPU > 0) {
		HANDLE_ERROR(cudaSetDevice(GPUIndex[threadNum % NumberOfGPU]));
	}
}

int Simulation::GetWeightSaveStep() {
	return this->WeightSaveInterval;
}

void Simulation::AddInputSpikeDriver(InputSpikeDriver* newInputSpikeDriver) {
	this->InputSpikeDriverList.push_back(newInputSpikeDriver);
}

void Simulation::AddOutputSpikeDriver(OutputSpikeDriver* newOutputSpikeDriver) {
	this->OutputSpikeDriverList.push_back(newOutputSpikeDriver);
}

void Simulation::AddInputCurrentDriver(InputCurrentDriver* newInputCurrentDriver) {
	this->InputCurrentDriverList.push_back(newInputCurrentDriver);
}

void Simulation::AddOutputMonitorDriver(OutputSpikeDriver* newMonitorSpikeDriver) {
	this->MonitorSpikeList.push_back(newMonitorSpikeDriver);
}

void Simulation::AddOutputWeightDriver(OutputWeightDriver* WeightDriver) {
	this->WeightDriverList.push_back(WeightDriver);
}

void Simulation::AddOuterDynamicStateDriver(OuterDynamicStateDriver* driver) {
	this->OuterDynamicStateDriverList.push_back(driver);
}

void Simulation::AddFileOutputSpikeDriver(std::string filename) {
	this->file_output_spike_driver = new FileOutputSpikeDriver(filename.c_str(), false);
}

void Simulation::AddFileOutputMonitorDriver(std::string filename) {
	this->file_output_monitor_driver = new FileOutputSpikeDriver(filename.c_str(), true);
}

void Simulation::AddFileOuterDynamicStateDriver(std::string filename) {
	this->file_outer_dynamic_state_driver = new FileOuterDynamicStateDriver(filename.c_str());
}

void Simulation::AddFileOutputWeightDriver(std::string FileName, int WeightSaveInterval) {
	this->file_output_weight_driver = new FileOutputWeightDriver(FileName.c_str());
	this->WeightSaveInterval = WeightSaveInterval;
}

void Simulation::AddZMQInputOutputSpikeDriver(enum DriverType driverType, std::string address, unsigned short port, int communicationInterval) {
#if SNN_WITH_ZMQ
	this->zmq_input_output_spike_driver = new ZMQInputOutputSpikeDriver(driverType, address, port);
	this->CommunivationInterval = communicationInterval;
	this->InputSpikeDriverList.push_back(this->zmq_input_output_spike_driver);
	this->OutputSpikeDriverList.push_back(this->zmq_input_output_spike_driver);
#else
	(void) driverType;
	(void) address;
	(void) port;
	(void) communicationInterval;
	std::cout << "ZMQ support is disabled in this build." << std::endl;
#endif

}

void Simulation::AddZMQAsyncInputOutputSpikeDriver(std::string subscribeAddress, unsigned short publishPort, unsigned short subscribePort, std::string publishTopic, std::string subscribeTopic, int communicationInterval) {
#if SNN_WITH_ZMQ
	this->zmq_async_input_output_spike_driver = new ZMQAsyncInputOutputSpikeDriver(
		publishPort,
		subscribeAddress,
		subscribePort,
		publishTopic,
		subscribeTopic);
	this->CommunivationInterval = communicationInterval;
	this->InputSpikeDriverList.push_back(this->zmq_async_input_output_spike_driver);
	this->OutputSpikeDriverList.push_back(this->zmq_async_input_output_spike_driver);
#else
	(void)subscribeAddress;
	(void)publishPort;
	(void)subscribePort;
	(void)publishTopic;
	(void)subscribeTopic;
	(void)communicationInterval;
	std::cout << "ZMQ support is disabled in this build." << std::endl;
#endif
}

void Simulation::SaveWeight() {
	std::cout << "Saving weights..." << std::endl;
	for (std::list<OutputWeightDriver*>::iterator it = this->WeightDriverList.begin(); it != this->WeightDriverList.end(); it++) {
		(*it)->WriteWeight(this->network, this->currenttime[0]);
	}
}


void Simulation::LoadInput(CommunicationEvent* c_event) {
	//逐一在InputSpikeDriverList中加载输入
	for (std::list<InputSpikeDriver*>::iterator it = this->InputSpikeDriverList.begin(); it != this->InputSpikeDriverList.end(); it++) {
		if (!(*it)->isFinished) {
			(*it)->LoadInputSpike(this->EventHeap, this->network, c_event->getTime());
		}
	}
}

void Simulation::PublishOutput(CommunicationEvent* c_event) {
	//逐一在OutputSpikeDriverList中发布输出
	for (std::list<OutputSpikeDriver*>::iterator it = this->OutputSpikeDriverList.begin(); it != this->OutputSpikeDriverList.end(); it++) {
		if ((*it)->IsBuffered()) {
			(*it)->FlushBuffers();
		}
	}

}

void Simulation::LoadWeight(const char* filename) {
	//调用网络导入权重
	this->network->LoadWeightsFromFile(filename);
}

void Simulation::SaveWeightToFile(const char* filename) {
	this->network->SaveWeightsToFile(filename);
}

void Simulation::WriteOuterDynamicState(int time_step, const OuterDynamicJointState& state) {
	for (std::list<OuterDynamicStateDriver*>::iterator it = this->OuterDynamicStateDriverList.begin(); it != this->OuterDynamicStateDriverList.end(); it++) {
		(*it)->WriteJointState(time_step, this->basetimesteps, state);
	}
}

void Simulation::CreateOuterDynamic(const std::list<OuterDynamicDescription>& outer_dynamic_list) {
	for (std::list<OuterDynamicDescription>::const_iterator it = outer_dynamic_list.begin(); it != outer_dynamic_list.end(); it++) {
		OuterDynamicModel* model = OuterDynamicModelFactory::createOuterDynamicModel(*it);
		model->Initialize(*it, this);
		this->OuterDynamicModelList.push_back(model);
	}
}

void Simulation::QueueOuterDynamicInputSpike(int event_time, int neuron_id) {
	std::lock_guard<std::mutex> lock(this->outer_dynamic_input_mutex);
	this->outer_dynamic_input_times.push_back(event_time);
	this->outer_dynamic_input_neurons.push_back(neuron_id);
}

void Simulation::FlushOuterDynamicInputSpikes(int current_time) {
	std::vector<int> event_times;
	std::vector<int> event_neurons;
	{
		std::lock_guard<std::mutex> lock(this->outer_dynamic_input_mutex);
		if (this->outer_dynamic_input_times.empty()) {
			return;
		}

		std::vector<int> remaining_times;
		std::vector<int> remaining_neurons;
		remaining_times.reserve(this->outer_dynamic_input_times.size());
		remaining_neurons.reserve(this->outer_dynamic_input_neurons.size());

		for (std::size_t i = 0; i < this->outer_dynamic_input_times.size(); ++i) {
			if (this->outer_dynamic_input_times[i] > current_time) {
				event_times.push_back(this->outer_dynamic_input_times[i]);
				event_neurons.push_back(this->outer_dynamic_input_neurons[i]);
			}
		}

		this->outer_dynamic_input_times.swap(remaining_times);
		this->outer_dynamic_input_neurons.swap(remaining_neurons);
	}

	if (!event_times.empty()) {
		this->inputSpikeDriver->LoadInputSpike(
			this->EventHeap,
			this->network,
			static_cast<int>(event_times.size()),
			event_times.data(),
			event_neurons.data());
	}
}




