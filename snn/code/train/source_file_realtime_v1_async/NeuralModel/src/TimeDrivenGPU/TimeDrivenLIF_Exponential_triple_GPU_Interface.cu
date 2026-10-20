#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenLIF_Exponential_triple_GPU_Interface.cuh"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector_Interface.cuh"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenLIF_Exponential_triple_GPU.cuh"
#include <iostream>
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Current/inc/CurrentSynapse.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/error/cudaerror.h"
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/IntegrationMethodGPU_Interface.cuh"
#include "../source_file_realtime_v1_async/ModelFactory/Integration_method_GPU_Interface_Facory.cuh"
#include "../source_file_realtime_v1_async/mainprogram/inc/BenchProfiling.h"


TimeDrivenLIF_Exponential_triple_GPU_Interface::TimeDrivenLIF_Exponential_triple_GPU_Interface(int timestep) :TimeDrivenNeuronModelGPU_Interface(timestep), CurrentSynapeModel(0), NeuronModelOnGPU(0) {
	this->StateVector = new Neuron_State_Vector_Interface(N_NeuronStateVariables);
	std::string name = std::string("TimeDrivenLIF_Exponential_triple_GPU");
	this->setModelName(name);
}

TimeDrivenLIF_Exponential_triple_GPU_Interface::~TimeDrivenLIF_Exponential_triple_GPU_Interface() {
	if (this->CurrentSynapeModel != 0) {
		delete this->CurrentSynapeModel;
		this->CurrentSynapeModel = 0;
	}
	//閺嬫劖鐎疓PU娑撳﹦娈戠粊鐐电病閸忓啯膩閸?	this->DestroyGPUModel();
}

__global__ void DeleteClassGPU(TimeDrivenLIF_Exponential_triple_GPU** GPU_NeuralModel) {
	//鐠佲晝顑囨稉鈧稉顏嗗殠缁嬪鏀㈠В涓烶U娑撳﹦娈戠粊鐐电病閸忓啯膩閸?
	if (blockIdx.x == 0 && threadIdx.x == 0) {
		delete (*GPU_NeuralModel);
	}
}

void TimeDrivenLIF_Exponential_triple_GPU_Interface::DestroyGPUModel() {
	//鐠嬪啰鏁ら崗銊ョ湰閸戣姤鏆熼柨鈧В涓烶U娑撳﹦娈戠粊鐐电病閸忓啯膩閸?
	if (this->NeuronModelOnGPU != 0) {
		DeleteClassGPU << <1, 1 >> > (this->NeuronModelOnGPU);
		cudaFree(this->NeuronModelOnGPU);
	}

}

Neuron_State_Vector* TimeDrivenLIF_Exponential_triple_GPU_Interface::InitState() {
	return this->StateVector;
}


InternalSpike* TimeDrivenLIF_Exponential_triple_GPU_Interface::ProcessSpike(Interconnections* inter, int time) {
	// 根据突触类型把权重累加到对应的辅助状态数组
	if (inter->type == 0 && this->AMPA) {
		this->State_GPU->AuxStateCPU[(this->index_ampa - 1) * this->State_GPU->NumberofNeuron + inter->TargetNeuronModelIndex] += inter->weight;
	}
	else if (inter->type == 1 && this->GABA) {
		this->State_GPU->AuxStateCPU[(this->index_gaba - 1) * this->State_GPU->NumberofNeuron + inter->TargetNeuronModelIndex] += inter->weight;
	}
	else if (inter->type == 2 && this->NMDA) {
		this->State_GPU->AuxStateCPU[(this->index_nmda - 1) * this->State_GPU->NumberofNeuron + inter->TargetNeuronModelIndex] += inter->weight;
	}
	return 0;
}


void TimeDrivenLIF_Exponential_triple_GPU_Interface::ProcessCurrent(Interconnections* inter, Neuron* Target, float current) {
	//鐠佸墽鐤員arget娴肩姴鍙嗙粣浣叫曢惃鍕暩濞翠礁銇囩亸?
	this->CurrentSynapeModel->SetInputCurrentPerSynapse(Target->index_in_NeuronModel, inter->subindex_type, current);
	//鐠侊紕鐣婚幀鑽ゆ暩濞?
	float total_current = this->CurrentSynapeModel->GetTotalInputCurrentPerNeuron(Target->index_in_NeuronModel);
	//鐠佸墽鐤咥uxStateCPU娑擃厾娈戦悽鍨ウ婢堆冪毈
	this->State_GPU->AuxStateCPU[3 * this->State_GPU->NumberofNeuron + Target->index_in_NeuronModel] = total_current;
}

//閻劌鍙忕仦鈧崙鑺ユ殶鐏忎浇顥奊PU娑撳﹦娈戦弴瀛樻煀閸戣姤鏆?
__global__ void TimeDrivenLIF_Exponential_triple_GPU_Interface_Update(TimeDrivenLIF_Exponential_triple_GPU** NeuralModelGPU, int currenttime, float timestepsize) {
	(*NeuralModelGPU)->UpdateState(currenttime, timestepsize);
}


void TimeDrivenLIF_Exponential_triple_GPU_Interface::UpdateState(int index, int time, Simulation* simulation) {
    const long long total_start = bench_profile::now_ns();
	auto* stateGPU = this->State_GPU;
	int neuronNum = stateGPU->NumberofNeuron;
	int timedepInput = this->N_TimedependentInput;
	size_t copySize = sizeof(float) * neuronNum * timedepInput;
	size_t zeroSize = sizeof(float) * neuronNum * (timedepInput - 1);
	if (this->deviceProp.canMapHostMemory) {
		memset(this->State_GPU->InternalSpikeCPU, 0, sizeof(bool) * this->State_GPU->NumberofNeuron);
        const long long kernel_start = bench_profile::now_ns();
		TimeDrivenLIF_Exponential_triple_GPU_Interface_Update << <this->gridsize, this->blocksize, 0, this->computeStream >> > (this->NeuronModelOnGPU, time, simulation->basetimesteps);
        bench_profile::gpu_update_kernel_ns.fetch_add(bench_profile::now_ns() - kernel_start, std::memory_order_relaxed);
	}
	else {
        const long long h2d_start = bench_profile::now_ns();
		HANDLE_ERROR(cudaMemcpyAsync(stateGPU->AuxStateGPU, stateGPU->AuxStateCPU, copySize, cudaMemcpyHostToDevice, this->copyStream));
        bench_profile::gpu_update_memcpy_h2d_ns.fetch_add(bench_profile::now_ns() - h2d_start, std::memory_order_relaxed);
		HANDLE_ERROR(cudaMemsetAsync(this->State_GPU->InternalSpikeGPU, 0, sizeof(bool) * this->State_GPU->NumberofNeuron, this->computeStream));
        const long long kernel_start = bench_profile::now_ns();
		TimeDrivenLIF_Exponential_triple_GPU_Interface_Update << <this->gridsize, this->blocksize, 0, this->computeStream >> > (this->NeuronModelOnGPU, time, simulation->basetimesteps);
        bench_profile::gpu_update_kernel_ns.fetch_add(bench_profile::now_ns() - kernel_start, std::memory_order_relaxed);
        const long long d2h_start = bench_profile::now_ns();
		HANDLE_ERROR(cudaMemcpyAsync(this->State_GPU->InternalSpikeCPU, this->State_GPU->InternalSpikeGPU, sizeof(bool) * this->State_GPU->NumberofNeuron, cudaMemcpyDeviceToHost, this->copyStream));
        bench_profile::gpu_update_d2h_ns.fetch_add(bench_profile::now_ns() - d2h_start, std::memory_order_relaxed);
	}
	if (this->StateVector->IsMonitored) {
        const long long sync_start = bench_profile::now_ns();
		HANDLE_ERROR(cudaStreamSynchronize(this->copyStream));
		HANDLE_ERROR(cudaStreamSynchronize(this->computeStream));
        bench_profile::gpu_update_sync_ns.fetch_add(bench_profile::now_ns() - sync_start, std::memory_order_relaxed);
        const long long monitor_d2h_start = bench_profile::now_ns();
		HANDLE_ERROR(cudaMemcpyAsync(this->State_GPU->Vector_of_StateVariable, this->State_GPU->Vector_of_StateVariableGPU, this->State_GPU->NumberofNeuron * this->State_GPU->NumberofStateVariable * sizeof(float), cudaMemcpyDeviceToHost, this->copyStream));
		HANDLE_ERROR(cudaMemcpyAsync(this->State_GPU->LastUpdate, this->State_GPU->LastUpdateGPU, this->State_GPU->NumberofNeuron * sizeof(int), cudaMemcpyDeviceToHost, this->copyStream));
		HANDLE_ERROR(cudaMemcpyAsync(this->State_GPU->LastSpike, this->State_GPU->LastSpikingGPU, this->State_GPU->NumberofNeuron * sizeof(int), cudaMemcpyDeviceToHost, this->copyStream));
		HANDLE_ERROR(cudaStreamSynchronize(this->copyStream));
        bench_profile::gpu_update_d2h_ns.fetch_add(bench_profile::now_ns() - monitor_d2h_start, std::memory_order_relaxed);
	}
    const long long memset_start = bench_profile::now_ns();
	HANDLE_ERROR(cudaMemsetAsync(stateGPU->AuxStateGPU, 0, zeroSize, this->computeStream));
    bench_profile::gpu_update_memset_ns.fetch_add(bench_profile::now_ns() - memset_start, std::memory_order_relaxed);
    const long long record_start = bench_profile::now_ns();
	HANDLE_ERROR(cudaEventRecord(this->sync_event, this->computeStream));
    bench_profile::gpu_update_event_record_ns.fetch_add(bench_profile::now_ns() - record_start, std::memory_order_relaxed);
    bench_profile::gpu_update_total_ns.fetch_add(bench_profile::now_ns() - total_start, std::memory_order_relaxed);
    bench_profile::gpu_update_count.fetch_add(1, std::memory_order_relaxed);
}


void TimeDrivenLIF_Exponential_triple_GPU_Interface::InitStateVector(int NumberofNeuron, int GPUindex) {
	//閸掑棗绔峰蹇氼吀缁犳鍨庨柊宀筆U
	//this->GPU_ID = openmpindex % NumberofGPU;
	this->NumberOfNeuron = NumberofNeuron;
	this->GPU_ID = GPUindex % NumberOfGPU;
	HANDLE_ERROR(cudaSetDevice(GPUIndex[GPU_ID]));
	HANDLE_ERROR(cudaGetDeviceProperties(&this->deviceProp, GPUIndex[GPUindex % NumberOfGPU]));
	HANDLE_ERROR(cudaEventCreate(&this->sync_event));
	HANDLE_ERROR(cudaStreamCreate(&copyStream));
	HANDLE_ERROR(cudaStreamCreate(&computeStream));
	// 将 StateVector 转换为 GPU 状态接口
	this->State_GPU = (Neuron_State_Vector_Interface*)this->StateVector;
	// 初始化状态均值和方差，其中膜电位从 V_reset + init[0] 开始
	float new_init[] = { this->V_reset + this->init[0], 0.0, 0.0, 0.0, 0.0 };
	float new_sigma[] = { this->sigma[0], this->sigma[1], this->sigma[2], this->sigma[3], this->sigma[4] };
	// 在 GPU 上初始化神经元状态
	this->State_GPU->InitNeuronStateGPU(NumberofNeuron, new_init, new_sigma, this->N_TimedependentInput, this->deviceProp);
	//閸掓繂顫愰崠鏈慞U娑撳﹦娈戠粊鐐电病閸忓啯膩閸?
	this->InitializeClassGPU2(NumberofNeuron);
	//閸掓繂顫愰崠鏈慞U娑撳﹦娈戦悩鑸碘偓浣告倻闁?
	InitializeVectorNeuronState_GPU2();
	this->CurrentSynapeModel = new CurrentSynapse(NumberofNeuron);
	//閸掓繂顫愰崠鏍暩濞翠焦膩閸?
	InitializeInputCurrentSynapseStructure();
}


__global__ void LIFTimeDrivenNeuralModel_GPU_C_Interface_InitializeClassGPU2(TimeDrivenLIF_Exponential_triple_GPU** NeuralModelOnGPU, char* int_Name,
	float V_rest, float tau, float V_th, float R, float V_reset, int t_ref, float E_ampa, float ampa_tau, float E_gaba, float gaba_tau, float nmda_tau, int Timestep, float timesteps, void** d_parm, int N_neurons) {
	if (blockIdx.x == 0 && threadIdx.x == 0) {
		(*NeuralModelOnGPU) = new TimeDrivenLIF_Exponential_triple_GPU(int_Name, V_rest, tau, V_th, R, V_reset, t_ref, E_ampa, ampa_tau, E_gaba, gaba_tau, nmda_tau, Timestep, timesteps, d_parm, N_neurons);
	}
}


void TimeDrivenLIF_Exponential_triple_GPU_Interface::InitializeClassGPU2(int NumberofNeuron) {
	HANDLE_ERROR(cudaMalloc(&(this->NeuronModelOnGPU), sizeof(TimeDrivenLIF_Exponential_triple_GPU**)));
	//閸掑棝鍘ょ粔顖氬瀻閺傝纭堕惃鍕倳缁?
	char* integrationNameGPU = nullptr;
	cudaMalloc((void**)&integrationNameGPU, 32 * sizeof(char));

	// CPU -> GPU 拷贝积分方法名称
	cudaMemcpy(
		integrationNameGPU,
		&integrationMethodGPUInterface->name[0],
		32 * sizeof(char),
		cudaMemcpyHostToDevice
	);
	//閸掑棝鍘locksize
	this->blocksize = 128;
	//鐠侊紕鐣籫ridsize
	this->gridsize = this->deviceProp.multiProcessorCount * 16;
	if ((NumberofNeuron + this->blocksize - 1) / this->blocksize < this->gridsize) {
		this->gridsize = (NumberofNeuron + this->blocksize - 1) / this->blocksize;
	}
	//缂佺喕顓搁幀鑽ゅ殠缁嬪鏆?
	int TotalThread = this->gridsize * this->blocksize;
	//娑撶瘡PU娑撳﹦娈戠粔顖氬瀻閺傝纭堕崚鍡涘帳閸愬懎鐡?
	this->integrationMethodGPUInterface->InitIntegrationMethodOnGPU(this->NumberOfNeuron, TotalThread);
	//鐠嬪啰鏁ら崗銊ョ湰閸戣姤鏆熼崚娑樼紦GPU娑撳﹦娈戠粊鐐电病閸忓啯膩閸?
	LIFTimeDrivenNeuralModel_GPU_C_Interface_InitializeClassGPU2 << <1, 1 >> > (this->NeuronModelOnGPU, integrationNameGPU, this->V_rest, this->tau, this->V_th, this->R, this->V_reset, this->t_ref, this->E_ampa, this->ampa_tau, this->E_gaba, this->gaba_tau, this->nmda_tau, this->getTimestepSize(), this->timestepdouble, this->integrationMethodGPUInterface->d_param, this->NumberOfNeuron);
	//闁插﹥鏂丟PU娑撳﹦娈戠粔顖氬瀻閺傝纭堕崥宥囆?
	cudaFree(integrationNameGPU);
}

__global__ void initializeVectorNeuronState_GPU2(TimeDrivenLIF_Exponential_triple_GPU** NeuralModelGPU, int NumberofVariable, float* InitState, float* AuxStateGPU, float* StateGPU, int* LastUpdateGPU, int* LastSpikeGPU, bool* InternalSpikeGPU, int NumberofNeurons) {
	if (blockIdx.x == 0 && threadIdx.x == 0) {
		(*NeuralModelGPU)->InitializeNeuronStateVector(NumberofVariable, InitState, AuxStateGPU, StateGPU, LastUpdateGPU, LastSpikeGPU, InternalSpikeGPU, NumberofNeurons);
	}
}

void TimeDrivenLIF_Exponential_triple_GPU_Interface::InitializeVectorNeuronState_GPU2() {
	Neuron_State_Vector_Interface* State = (Neuron_State_Vector_Interface*)this->StateVector;
	//鐠嬪啰鏁ら崗銊ョ湰閸戣姤鏆熼崚婵嗩潗閸栨湋PU娑撳﹦娈戦悩鑸碘偓浣告倻闁?
	initializeVectorNeuronState_GPU2 << <1, 1 >> > (this->NeuronModelOnGPU, State->NumberofStateVariable, State->InitialStateGPU, State->AuxStateGPU, State->Vector_of_StateVariableGPU, State->LastUpdateGPU, State->LastSpikingGPU, State->InternalSpikeGPU, State->NumberofNeuron);

}

__global__ void SetSynapseOnGPU(TimeDrivenLIF_Exponential_triple_GPU** NeuralModelGPU, bool AMPA, bool GABA, bool NMDA, bool I_EXT) {
	if (blockIdx.x == 0 && threadIdx.x == 0) {
		(*NeuralModelGPU)->SetEnabledSynapsis(AMPA, GABA, NMDA, I_EXT);
	}
}

void TimeDrivenLIF_Exponential_triple_GPU_Interface::CheckType(Interconnections* inter) {
	int Type = inter->type;
	if (Type == 0 && this->AMPA == false) {
		this->AMPA = true;
		SetSynapseOnGPU << <1, 1 >> > (this->NeuronModelOnGPU, this->AMPA, this->GABA, this->NMDA, this->I_EXT);
	}
	else if (Type == 1 && this->GABA == false) {
		this->GABA = true;
		SetSynapseOnGPU << <1, 1 >> > (this->NeuronModelOnGPU, this->AMPA, this->GABA, this->NMDA, this->I_EXT);
	}
	else if (Type == 3) {
		this->I_EXT = true;
		// 记录当前神经元已接入的输入电流突触序号
		inter->subindex_type = this->CurrentSynapeModel->N_connections[inter->TargetNeuron->index_in_NeuronModel];
		// 增加该神经元的输入电流突触计数
		this->CurrentSynapeModel->IncrementNInputCurrentSynapsesPerNeuron(inter->TargetNeuron->index_in_NeuronModel);
		SetSynapseOnGPU << <1, 1 >> > (this->NeuronModelOnGPU, this->AMPA, this->GABA, this->NMDA, this->I_EXT);
	}
	else if (Type == 2 && this->NMDA == false) {
		this->NMDA = true;
		SetSynapseOnGPU << <1, 1 >> > (this->NeuronModelOnGPU, this->AMPA, this->GABA, this->NMDA, this->I_EXT);
	}
	else if (Type == 0 && this->AMPA == true) {
		return;
	}
	else if (Type == 1 && this->GABA == true) {
		return;
	}
	else if (Type == 2 && this->NMDA == true) {
		return;
	}
	else {
		std::cout << "Error: Type of synapse is not supported in this model" << std::endl;
	}
}

void TimeDrivenLIF_Exponential_triple_GPU_Interface::InitializeInputCurrentSynapseStructure() {
	if (this->CurrentSynapeModel != 0) {
		this->CurrentSynapeModel->InitializeInputCurrentPerSynapseStructure();
	}
}

void TimeDrivenLIF_Exponential_triple_GPU_Interface::SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep) {
	//閸︺劌鐡ч崗闀愯厬閺屻儲澹橀崣鍌涙殶
	//閺屻儲澹橀棃娆愪紖閻㈠吀缍?
	std::map<std::string, boost::any>::iterator iter = parametermap.find("V_rest");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->V_rest = new_parameter;
		parametermap.erase(iter);
	}
	//閺屻儲澹榯au
	iter = parametermap.find("tau");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->tau = new_parameter;
		parametermap.erase(iter);
	}
	//閺屻儲澹橀梼鍫濃偓?
	iter = parametermap.find("V_th");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->V_th = new_parameter;
		parametermap.erase(iter);
	}
	//閺屻儲澹橀悽鐢告▎
	iter = parametermap.find("R");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->R = new_parameter;
		parametermap.erase(iter);
	}
	//閺屻儲澹橀柌宥囩枂閻㈠吀缍?
	iter = parametermap.find("V_reset");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->V_reset = new_parameter;
		parametermap.erase(iter);
	}
	//閺屻儲澹樻稉宥呯安閺?
	iter = parametermap.find("t_ref");
	if (iter != parametermap.end()) {
		int new_parameter = boost::any_cast<int>(iter->second);
		this->t_ref = new_parameter;
		parametermap.erase(iter);
	}
	iter = parametermap.find("E_ampa");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->E_ampa = new_parameter;
		parametermap.erase(iter);
	}
	//?鑼??AMPA?????????鍗??????
	iter = parametermap.find("ampa_tau");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->ampa_tau = new_parameter;
		parametermap.erase(iter);
	}
	iter = parametermap.find("E_gaba");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->E_gaba = new_parameter;
		parametermap.erase(iter);
	}
	iter = parametermap.find("nmda_tau");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->nmda_tau = new_parameter;
		parametermap.erase(iter);
	}
	iter = parametermap.find("gaba_tau");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->gaba_tau = new_parameter;
		parametermap.erase(iter);
	}

	iter = parametermap.find("basetimestep");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->timestepdouble = new_parameter;
		parametermap.erase(iter);
	}

	//閺屻儲澹橀梾蹇旀簚閹冨棘閺?
	iter = parametermap.find("random_mu");
	if (iter != parametermap.end()) {
		std::array<float, 5> new_parameter = boost::any_cast<std::array<float, 5>>(iter->second);
		this->init = new_parameter;
		parametermap.erase(iter);
	}

	iter = parametermap.find("random_sigma");
	if (iter != parametermap.end()) {
		std::array<float, 5> new_parameter = boost::any_cast<std::array<float, 5>>(iter->second);
		this->sigma = new_parameter;
		parametermap.erase(iter);
	}

	//閺屻儲澹樼粔顖氬瀻閺傝纭?

	iter = parametermap.find("int_method");
	if (iter != parametermap.end()) {
		if (this->integrationMethodGPUInterface != 0) {
			delete this->integrationMethodGPUInterface;
		}
		ModelDescription temp = boost::any_cast<ModelDescription>(iter->second);
		temp.ModelParameter["step"] = basetimestep;
		// 根据参数创建对应的 GPU 积分方法接口
		this->integrationMethodGPUInterface = Integration_method_GPU_Interface_Factory<TimeDrivenLIF_Exponential_triple_GPU_Interface>::create_Integration_method_GPU_Interface(temp, this);
		parametermap.erase(iter);

	}
	else {
		//閸掓稑缂撴稉鈧稉顏堢帛鐠併倗娈戦崜宥呮倻濞喲勫缁夘垰鍨庨弬瑙勭《
		ModelDescription temp_modelDescription;
		temp_modelDescription.ModelName = "ForwardEulerMethod";
		temp_modelDescription.ModelParameter["step"] = basetimestep;
		this->integrationMethodGPUInterface = Integration_method_GPU_Interface_Factory<TimeDrivenLIF_Exponential_triple_GPU_Interface>::create_Integration_method_GPU_Interface(temp_modelDescription, this);
	}

	static int debug_print_count = 0;
	if (debug_print_count < 8) {
		++debug_print_count;
		std::cout
			<< "[triple_gpu params] "
			<< "V_rest=" << this->V_rest
			<< " tau=" << this->tau
			<< " V_th=" << this->V_th
			<< " R=" << this->R
			<< " V_reset=" << this->V_reset
			<< " t_ref=" << this->t_ref
			<< " E_ampa=" << this->E_ampa
			<< " ampa_tau=" << this->ampa_tau
			<< " E_gaba=" << this->E_gaba
			<< " gaba_tau=" << this->gaba_tau
			<< " nmda_tau=" << this->nmda_tau
			<< " basetimestep=" << basetimestep
			<< std::endl;
	}



}


std::map<std::string, boost::any> TimeDrivenLIF_Exponential_triple_GPU_Interface::getParameters() {
	//閻㈢喐鍨氭稉鈧稉顏勭摟閸?
	std::map<std::string, boost::any> parametermap;
	parametermap["V_rest"] = this->V_rest;
	parametermap["tau"] = this->tau;
	parametermap["V_th"] = this->V_th;
	parametermap["R"] = this->R;
	parametermap["V_reset"] = this->V_reset;
	parametermap["t_ref"] = this->t_ref;
	parametermap["E_ampa"] = this->E_ampa;
	parametermap["ampa_tau"] = this->ampa_tau;
	parametermap["E_gaba"] = this->E_gaba;
	parametermap["nmda_tau"] = this->nmda_tau;
	parametermap["gaba_tau"] = this->gaba_tau;
	parametermap["random_mu"] = this->init;
	parametermap["random_sigma"] = this->sigma;
	ModelDescription temp;
	temp.ModelParameter = this->integrationMethodGPUInterface->getParameters();
	temp.ModelName = boost::any_cast<std::string>(temp.ModelParameter["name"]);
	parametermap["int_method"] = temp;
	return parametermap;

}

