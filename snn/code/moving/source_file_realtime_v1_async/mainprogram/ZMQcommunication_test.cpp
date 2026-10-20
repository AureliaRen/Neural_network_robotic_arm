/*
* 测试ZMQ通信的主程序
*/

#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/communication/inc/DriverType.h"
#include "../source_file_realtime_v1_async/communication/inc/ZmqTopicProtocol.h"
#include <iostream>
#include <thread>
#include <cassert>
#include <zmq.hpp>
#include <csignal>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>
#include <array>
#include <list>
#include <windows.h>

const unsigned short TEST_PORT = 5565;
//鏈哄櫒浜烘暟鎹殑鍙戝竷绔彛
const unsigned short ASYNC_ROBOT_PUB_PORT = 5566;
	//创建突触前输入神经元
const unsigned short ASYNC_SNN_PUB_PORT = 5567;

namespace {

class TeeBuf : public std::streambuf {
public:
	TeeBuf(std::streambuf* first, std::streambuf* second) : first_(first), second_(second) {}

protected:
	int overflow(int c) override {
		if (c == EOF) {
			return !EOF;
		}
		const int first_result = first_ ? first_->sputc(static_cast<char>(c)) : c;
		const int second_result = second_ ? second_->sputc(static_cast<char>(c)) : c;
		return (first_result == EOF || second_result == EOF) ? EOF : c;
	}

	int sync() override {
		const int first_result = first_ ? first_->pubsync() : 0;
		const int second_result = second_ ? second_->pubsync() : 0;
		return (first_result == 0 && second_result == 0) ? 0 : -1;
	}

private:
	std::streambuf* first_;
	std::streambuf* second_;
};

std::ofstream g_log_file;
std::mutex g_log_mutex;
std::streambuf* g_old_cout = nullptr;
std::streambuf* g_old_cerr = nullptr;
std::unique_ptr<TeeBuf> g_cout_tee;
std::unique_ptr<TeeBuf> g_cerr_tee;

void LogLine(const std::string& message) {
	std::lock_guard<std::mutex> lock(g_log_mutex);
	if (g_log_file.is_open()) {
		g_log_file << message << std::endl;
		g_log_file.flush();
	}
}

LONG WINAPI UnhandledExceptionLogger(EXCEPTION_POINTERS* exception_info) {
	if (exception_info != nullptr && exception_info->ExceptionRecord != nullptr) {
		LogLine("Unhandled SEH exception code=" + std::to_string(exception_info->ExceptionRecord->ExceptionCode));
	}
	else {
		LogLine("Unhandled SEH exception with null exception info");
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

void TerminateLogger() {
	try {
		std::rethrow_exception(std::current_exception());
	}
	catch (const std::exception& ex) {
		LogLine(std::string("std::terminate after exception: ") + ex.what());
	}
	catch (...) {
		LogLine("std::terminate after non-std exception");
	}
	std::_Exit(1);
}

void SignalLogger(int signal_value) {
	LogLine("Signal received: " + std::to_string(signal_value));
	std::_Exit(1);
}

void InitializeLogging() {
	const char* log_path = "D:\\LLM_Related\\codex\\repo_copies\\snn_async_runtime.log";
	g_log_file.open(log_path, std::ios::out | std::ios::app);
	g_old_cout = std::cout.rdbuf();
	g_old_cerr = std::cerr.rdbuf();
	g_cout_tee = std::make_unique<TeeBuf>(g_old_cout, g_log_file.rdbuf());
	g_cerr_tee = std::make_unique<TeeBuf>(g_old_cerr, g_log_file.rdbuf());
	std::cout.rdbuf(g_cout_tee.get());
	std::cerr.rdbuf(g_cerr_tee.get());

	SetUnhandledExceptionFilter(UnhandledExceptionLogger);
	std::set_terminate(TerminateLogger);
	std::signal(SIGABRT, SignalLogger);
	std::signal(SIGSEGV, SignalLogger);

	LogLine("==== zmq_communication_smoke start ====");
}

} // namespace

int main() {
	InitializeLogging();
	try {
	const std::filesystem::path output_root = "D:/LLM_Related/codex/repo_copies/runtime_output";
	const std::filesystem::path weight_root = output_root / "Weight";
	const std::filesystem::path output_monitor_file = output_root / "output_monitor.txt";
	const std::filesystem::path output_spike_file = output_root / "output_spike.txt";
	const std::filesystem::path output_weight_file = weight_root / "outputweight.txt";
	LogLine("Ensuring output directories exist");
	std::filesystem::create_directories(weight_root);
	//存放网络神经元层描述
	std::list<NeuronLayerDescription> neuDescription;
	//瀛樻斁缃戠粶杩炴帴鎻忚堪
	std::list<ConnectionDescription> synDescription;
	//存放网络连接突触可塑性描述
	std::list<LearningRuleDescription> learnDescription;

	//鍒涘缓绐佽Е鍓嶈緭鍏ョ缁忓厓
	NeuronLayerDescription PreInputSpikeLayer;
	PreInputSpikeLayer.ModelName = "InputSpikeNeuronModel";
	PreInputSpikeLayer.numberofneuron = 2;
	PreInputSpikeLayer.isMonitored = true;
	PreInputSpikeLayer.isOutput = false;
	neuDescription.push_back(PreInputSpikeLayer);

	//创建突触后LIF神经元
	NeuronLayerDescription PostLIFNeuron;
	PostLIFNeuron.ModelName = "TimeDrivenLIF_Exponential_triple_GPU";
	PostLIFNeuron.numberofneuron = 2;
	PostLIFNeuron.isMonitored = true;
	PostLIFNeuron.isOutput = true;
	neuDescription.push_back(PostLIFNeuron);

	//鍒涘缓绐佽Е鍚庤Е鍙戠缁忓厓
	NeuronLayerDescription PostTriggerNeuron;
	PostTriggerNeuron.ModelName = "InputSpikeNeuronModel";
	PostTriggerNeuron.numberofneuron = 1;
	PostTriggerNeuron.isMonitored = false;
	PostTriggerNeuron.isOutput = false;
	neuDescription.push_back(PostTriggerNeuron);

	//创建输入到LIF神经元的连接
	NeuronLayerDescription CurrentNeuron;
	CurrentNeuron.ModelName = "InputCurrentNeuronModel";
	CurrentNeuron.numberofneuron = 1;
	CurrentNeuron.isMonitored = false;
	CurrentNeuron.isOutput = false;
	neuDescription.push_back(CurrentNeuron);

	//鍒涘缓杈撳叆鍒癓IF绁炵粡鍏冪殑杩炴帴
	ConnectionDescription Input2LIF;
	std::vector<int> source_neuron_list1;
	std::vector<int> target_neuron_list1;
	std::vector<int> Type1;
	std::vector<float> Weight1;
	std::vector<float> MaxWeight1;
	std::vector<int> Delay1;
	std::vector<int> learning_rule1;
	std::vector<int> trigger_rule1;
	std::vector<ModelDescription> modelDescription;

	source_neuron_list1.push_back(0);
	source_neuron_list1.push_back(1);
	target_neuron_list1.push_back(2);
	target_neuron_list1.push_back(2);
	Type1.push_back(0);
	Type1.push_back(0);
	Weight1.push_back(1.0);
	Weight1.push_back(1.0);
	MaxWeight1.push_back(10.0);
	MaxWeight1.push_back(10.0);
	Delay1.push_back(1);
	Delay1.push_back(1);
	learning_rule1.push_back(0);
	learning_rule1.push_back(0);
	trigger_rule1.push_back(-1);
	trigger_rule1.push_back(-1);
	ModelDescription temp{};
	//temp.ModelName = "Alpha";
	modelDescription.push_back(temp);
	modelDescription.push_back(temp);
	Input2LIF.SourceNeuron = source_neuron_list1;
	Input2LIF.TargetNeuron = target_neuron_list1;
	Input2LIF.Type = Type1;
	Input2LIF.Weight = Weight1;
	Input2LIF.MaxWeight = MaxWeight1;
	Input2LIF.Delay = Delay1;
	Input2LIF.SynapseRule = learning_rule1;
	Input2LIF.TriggerRule = trigger_rule1;
	Input2LIF.SynapseModel = modelDescription;
	synDescription.push_back(Input2LIF);

	//创建触发神经元到LIF之间的连接
	ConnectionDescription Trigger2LIF;
	std::vector<int> source_neuron_list2;
	std::vector<int> target_neuron_list2;
	std::vector<int> Type2;
	std::vector<float> Weight2;
	std::vector<float> MaxWeight2;
	std::vector<int> Delay2;
	std::vector<int> learning_rule2;
	std::vector<int> trigger_rule2;
	std::vector<ModelDescription> modelDescription2;

	source_neuron_list2.push_back(4);
	target_neuron_list2.push_back(2);
	Type2.push_back(0);
	Weight2.push_back(0.0);
	MaxWeight2.push_back(10.0);
	Delay2.push_back(1);
	learning_rule2.push_back(-1);
	trigger_rule2.push_back(0);
	modelDescription2.push_back(temp);


	Trigger2LIF.SourceNeuron = source_neuron_list2;
	Trigger2LIF.TargetNeuron = target_neuron_list2;
	Trigger2LIF.Type = Type2;
	Trigger2LIF.Weight = Weight2;
	Trigger2LIF.MaxWeight = MaxWeight2;
	Trigger2LIF.Delay = Delay2;
	Trigger2LIF.SynapseRule = learning_rule2;
	Trigger2LIF.TriggerRule = trigger_rule2;
	Trigger2LIF.SynapseModel = modelDescription2;
	synDescription.push_back(Trigger2LIF);

	//创建突触可塑性描述
	LearningRuleDescription cerebullar_learning;
	cerebullar_learning.RuleName = "CerebullarLearningRule";
	cerebullar_learning.RuleParameter["fixweightchange"] = 0.2f;
	cerebullar_learning.RuleParameter["kernalchange"] = -0.3f;
	learnDescription.push_back(cerebullar_learning);

	//创建Simulation对象
	ConnectionDescription Current2LIF;
	std::vector<int> source_neuron_list3;
	std::vector<int> target_neuron_list3;
	std::vector<int> Type3;
	std::vector<float> Weight3;
	std::vector<float> MaxWeight3;
	std::vector<int> Delay3;
	std::vector<int> learning_rule3;
	std::vector<int> trigger_rule3;
	std::vector<ModelDescription> modelDescription3;

	source_neuron_list3.push_back(5);
	target_neuron_list3.push_back(3);
	Type3.push_back(3);
	Weight3.push_back(1.0);
	MaxWeight3.push_back(10.0);
	Delay3.push_back(1);
	learning_rule3.push_back(-1);
	trigger_rule3.push_back(-1);
	modelDescription3.push_back(temp);

	Current2LIF.SourceNeuron = source_neuron_list3;
	Current2LIF.TargetNeuron = target_neuron_list3;
	Current2LIF.Type = Type3;
	Current2LIF.Weight = Weight3;
	Current2LIF.MaxWeight = MaxWeight3;
	Current2LIF.Delay = Delay3;
	Current2LIF.SynapseRule = learning_rule3;
	Current2LIF.TriggerRule = trigger_rule3;
	Current2LIF.SynapseModel = modelDescription3;
	synDescription.push_back(Current2LIF);



	//鍒涘缓Simulation瀵硅薄
	Simulation* mysim;
	mysim = new Simulation(neuDescription, synDescription, learnDescription, 500, 0.2f, 1);
	LogLine("Simulation object allocated");
	std::cout << "Simulation object created" << std::endl;

	std::vector<int> input_spike_time;
	std::vector<int> input_neuron_index;
	for (int i = 15; i < 500; i += 150) {
		input_spike_time.push_back(i);
		input_neuron_index.push_back(0);
	}
	for (int i = 10; i < 500; i += 400) {
		input_spike_time.push_back(i);
		input_neuron_index.push_back(1);
	}
	for (int i = 0; i < 500; i += 250) {
		input_spike_time.push_back(i);
		input_neuron_index.push_back(4);
	}

	mysim->AddExternalSpikeActivity(input_spike_time, input_neuron_index);
	std::cout << "External spike activity added" << std::endl;

	//定义输出文件
	std::vector<int> input_current_time;
	std::vector<int> current_neuron_index;
	std::vector<float> current_value;
	input_current_time.push_back(0);
	current_neuron_index.push_back(5);
	current_value.push_back(12.8);


	mysim->AddZMQAsyncInputOutputSpikeDriver(
		"127.0.0.1",
		ASYNC_SNN_PUB_PORT,
		ASYNC_ROBOT_PUB_PORT,
		zmq_topic_protocol::kDefaultSnnOutputTopic,
		zmq_topic_protocol::kDefaultRobotInputTopic,
		10);
	LogLine("Async ZMQ driver added");

	//瀹氫箟杈撳嚭鏂囦欢
	mysim->AddFileOutputMonitorDriver(output_monitor_file.string());
	std::cout << "File Output monitor driver added" << std::endl;
	mysim->AddFileOutputSpikeDriver(output_spike_file.string());
	std::cout << "File Output spike driver added" << std::endl;
	mysim->AddFileOutputWeightDriver(output_weight_file.string(), 2);
	//mysim->AddFileOutputWeightDriver("D:/computing_ref/Demo/Python_plot/Weight/output_weight.txt", 100);
	//std::cout << "File Output weight driver added" << std::endl;
	mysim->InitSimulation();
	LogLine("Simulation initialized");
	std::cout << "Simulation initialized" << std::endl;
	//
	mysim->AddExternalCurrentActivity(input_current_time, current_neuron_index, current_value);
	std::cout << "External current activity added" << std::endl;
	//mysim->LoadWeight("D:/Programing/My_Program/SNN_simulator/My_project_build/Pyplot/output_weight1000.txt");
	//开始模拟
	//开始模拟
	auto start_time = std::chrono::high_resolution_clock::now();
	mysim->RunSimulationStep(500);
	LogLine("RunSimulationStep returned");
	std::cout << "Simulation finished" << std::endl;
	auto end_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli> duration_ms = end_time - start_time;

	delete mysim;
	LogLine("Simulation deleted");
	std::cout << "Simulation finished" << std::endl;
	std::cout << "Elapsed: " << duration_ms.count() << " ms" << std::endl;
	std::cout << "Elapsed: " << duration_ms.count() / 1000.0f << " s" << std::endl;
	return 0;
	}
	catch (const std::exception& ex) {
		LogLine(std::string("Caught std::exception: ") + ex.what());
		std::cerr << "[FATAL] " << ex.what() << std::endl;
		return 1;
	}
	catch (...) {
		LogLine("Caught unknown exception");
		std::cerr << "[FATAL] Unknown exception" << std::endl;
		return 1;
	}
}
