#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface.cuh"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector_Interface.cuh"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenIzhikevic_Exponential_Decay_GPU.cuh"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Current/inc/CurrentSynapse.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/error/cudaerror.h"
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/IntegrationMethodGPU_Interface.cuh"
#include "../source_file_realtime_v1_async/ModelFactory/Integration_method_GPU_Interface_Facory.cuh"
#include "../source_file_realtime_v1_async/mainprogram/inc/BenchProfiling.h"

TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface(int timestep) : TimeDrivenNeuronModelGPU_Interface(timestep), CurrentSynapeModel(0), NeuronModelOnGPU(0) {
	this->StateVector = new Neuron_State_Vector_Interface(N_NeuronStateVariables);
	std::string name = std::string("TimeDrivenIzhikevic_Exponential_Decay_GPU");
	this->setModelName(name);
}

TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::~TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface() {
	if (this->CurrentSynapeModel != 0) {
		delete this->CurrentSynapeModel;
		this->CurrentSynapeModel = 0;
	}
	this->DestroyGPUModel();
}

__global__ void DeleteIzhClassGPU(TimeDrivenIzhikevic_Exponential_Decay_GPU** GPU_NeuralModel) {
	if (blockIdx.x == 0 && threadIdx.x == 0) {
		delete (*GPU_NeuralModel);
	}
}

void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::DestroyGPUModel() {
	if (this->NeuronModelOnGPU != 0) {
		DeleteIzhClassGPU<<<1, 1>>>(this->NeuronModelOnGPU);
		cudaFree(this->NeuronModelOnGPU);
		this->NeuronModelOnGPU = 0;
	}
}

Neuron_State_Vector* TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::InitState() {
	return this->StateVector;
}

InternalSpike* TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::ProcessSpike(Interconnections* inter, int time) {
	(void)time;
	if (inter->type == 0 && this->AMPA) {
		this->State_GPU->AuxStateCPU[(this->index_ampa - this->N_DifferentialStates) * this->State_GPU->NumberofNeuron + inter->TargetNeuronModelIndex] += inter->weight;
	}
	else if (inter->type == 1 && this->GABA) {
		this->State_GPU->AuxStateCPU[(this->index_gaba - this->N_DifferentialStates) * this->State_GPU->NumberofNeuron + inter->TargetNeuronModelIndex] += inter->weight;
	}
	else if (inter->type == 2 && this->NMDA) {
		this->State_GPU->AuxStateCPU[(this->index_nmda - this->N_DifferentialStates) * this->State_GPU->NumberofNeuron + inter->TargetNeuronModelIndex] += inter->weight;
	}
	return 0;
}

void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::ProcessCurrent(Interconnections* inter, Neuron* Target, float current) {
	this->CurrentSynapeModel->SetInputCurrentPerSynapse(Target->index_in_NeuronModel, inter->subindex_type, current);
	float total_current = this->CurrentSynapeModel->GetTotalInputCurrentPerNeuron(Target->index_in_NeuronModel);
	this->State_GPU->AuxStateCPU[3 * this->State_GPU->NumberofNeuron + Target->index_in_NeuronModel] = total_current;
}

__global__ void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface_Update(TimeDrivenIzhikevic_Exponential_Decay_GPU** NeuralModelGPU, int currenttime, float timestepsize) {
	(*NeuralModelGPU)->UpdateState(currenttime, timestepsize);
}

void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::UpdateState(int index, int time, Simulation* simulation) {
	(void)index;
	const long long total_start = bench_profile::now_ns();
	auto* stateGPU = this->State_GPU;
	int neuronNum = stateGPU->NumberofNeuron;
	int timedepInput = this->N_TimedependentInput;
	size_t copySize = sizeof(float) * neuronNum * timedepInput;
	size_t zeroSize = sizeof(float) * neuronNum * (timedepInput - 1);
	if (this->deviceProp.canMapHostMemory) {
		const long long kernel_start = bench_profile::now_ns();
		TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface_Update<<<this->gridsize, this->blocksize, 0, this->computeStream>>>(this->NeuronModelOnGPU, time, simulation->basetimesteps);
		bench_profile::gpu_update_kernel_ns.fetch_add(bench_profile::now_ns() - kernel_start, std::memory_order_relaxed);
	}
	else {
		const long long h2d_start = bench_profile::now_ns();
		HANDLE_ERROR(cudaMemcpyAsync(stateGPU->AuxStateGPU, stateGPU->AuxStateCPU, copySize, cudaMemcpyHostToDevice, this->copyStream));
		bench_profile::gpu_update_memcpy_h2d_ns.fetch_add(bench_profile::now_ns() - h2d_start, std::memory_order_relaxed);
		const long long kernel_start = bench_profile::now_ns();
		TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface_Update<<<this->gridsize, this->blocksize, 0, this->computeStream>>>(this->NeuronModelOnGPU, time, simulation->basetimesteps);
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

void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::InitStateVector(int NumberofNeuron, int GPUindex) {
	this->NumberOfNeuron = NumberofNeuron;
	this->GPU_ID = GPUindex % NumberOfGPU;
	HANDLE_ERROR(cudaSetDevice(GPUIndex[GPU_ID]));
	HANDLE_ERROR(cudaGetDeviceProperties(&this->deviceProp, GPUIndex[GPUindex % NumberOfGPU]));
	HANDLE_ERROR(cudaEventCreate(&this->sync_event));
	HANDLE_ERROR(cudaStreamCreate(&copyStream));
	HANDLE_ERROR(cudaStreamCreate(&computeStream));
	this->State_GPU = (Neuron_State_Vector_Interface*)this->StateVector;
	float new_init[] = { this->V_rest + this->init[0], this->b * this->V_rest + this->init[1], this->init[2], this->init[3], this->init[4], this->init[5] };
	float new_sigma[] = { this->sigma[0], this->sigma[1], this->sigma[2], this->sigma[3], this->sigma[4], this->sigma[5] };
	this->State_GPU->InitNeuronStateGPU(NumberofNeuron, new_init, new_sigma, this->N_TimedependentInput, this->deviceProp);
	this->InitializeClassGPU2(NumberofNeuron);
	this->InitializeVectorNeuronState_GPU2();
	this->CurrentSynapeModel = new CurrentSynapse(NumberofNeuron);
	InitializeInputCurrentSynapseStructure();
}

__global__ void IzhTimeDrivenNeuralModel_GPU_C_Interface_InitializeClassGPU2(
	TimeDrivenIzhikevic_Exponential_Decay_GPU** NeuralModelOnGPU,
	char* int_Name,
	float a,
	float b,
	float c,
	float d,
	float V_rest,
	float V_th,
	float R,
	float E_ampa,
	float ampa_tau,
	float E_gaba,
	float gaba_tau,
	float nmda_tau,
	int Timestep,
	float timesteps,
	void** d_parm,
	int N_neurons) {
	if (blockIdx.x == 0 && threadIdx.x == 0) {
		(*NeuralModelOnGPU) = new TimeDrivenIzhikevic_Exponential_Decay_GPU(int_Name, a, b, c, d, V_rest, V_th, R, E_ampa, ampa_tau, E_gaba, gaba_tau, nmda_tau, Timestep, timesteps, d_parm, N_neurons);
	}
}

void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::InitializeClassGPU2(int NumberofNeuron) {
	HANDLE_ERROR(cudaMalloc(&(this->NeuronModelOnGPU), sizeof(TimeDrivenIzhikevic_Exponential_Decay_GPU**)));
	char* integrationNameGPU = nullptr;
	cudaMalloc((void**)&integrationNameGPU, 32 * sizeof(char));
	cudaMemcpy(integrationNameGPU, &integrationMethodGPUInterface->name[0], 32 * sizeof(char), cudaMemcpyHostToDevice);
	this->blocksize = 128;
	this->gridsize = this->deviceProp.multiProcessorCount * 16;
	if ((NumberofNeuron + this->blocksize - 1) / this->blocksize < this->gridsize) {
		this->gridsize = (NumberofNeuron + this->blocksize - 1) / this->blocksize;
	}
	int TotalThread = this->gridsize * this->blocksize;
	this->integrationMethodGPUInterface->InitIntegrationMethodOnGPU(this->NumberOfNeuron, TotalThread);
	IzhTimeDrivenNeuralModel_GPU_C_Interface_InitializeClassGPU2<<<1, 1>>>(
		this->NeuronModelOnGPU,
		integrationNameGPU,
		this->a,
		this->b,
		this->c,
		this->d,
		this->V_rest,
		this->V_th,
		this->R,
		this->E_ampa,
		this->ampa_tau,
		this->E_gaba,
		this->gaba_tau,
		this->nmda_tau,
		this->getTimestepSize(),
		this->timestepdouble,
		this->integrationMethodGPUInterface->d_param,
		this->NumberOfNeuron);
	cudaFree(integrationNameGPU);
}

__global__ void initializeVectorNeuronState_Izh_GPU2(TimeDrivenIzhikevic_Exponential_Decay_GPU** NeuralModelGPU, int NumberofVariable, float* InitState, float* AuxStateGPU, float* StateGPU, int* LastUpdateGPU, int* LastSpikeGPU, bool* InternalSpikeGPU, int NumberofNeurons) {
	if (blockIdx.x == 0 && threadIdx.x == 0) {
		(*NeuralModelGPU)->InitializeNeuronStateVector(NumberofVariable, InitState, AuxStateGPU, StateGPU, LastUpdateGPU, LastSpikeGPU, InternalSpikeGPU, NumberofNeurons);
	}
}

void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::InitializeVectorNeuronState_GPU2() {
	Neuron_State_Vector_Interface* State = (Neuron_State_Vector_Interface*)this->StateVector;
	initializeVectorNeuronState_Izh_GPU2<<<1, 1>>>(this->NeuronModelOnGPU, State->NumberofStateVariable, State->InitialStateGPU, State->AuxStateGPU, State->Vector_of_StateVariableGPU, State->LastUpdateGPU, State->LastSpikingGPU, State->InternalSpikeGPU, State->NumberofNeuron);
}

__global__ void SetIzhSynapseOnGPU(TimeDrivenIzhikevic_Exponential_Decay_GPU** NeuralModelGPU, bool AMPA, bool GABA, bool NMDA, bool I_EXT) {
	if (blockIdx.x == 0 && threadIdx.x == 0) {
		(*NeuralModelGPU)->SetEnabledSynapsis(AMPA, GABA, NMDA, I_EXT);
	}
}

void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::CheckType(Interconnections* inter) {
	int Type = inter->type;
	if (Type == 0 && this->AMPA == false) {
		this->AMPA = true;
		SetIzhSynapseOnGPU<<<1, 1>>>(this->NeuronModelOnGPU, this->AMPA, this->GABA, this->NMDA, this->I_EXT);
	}
	else if (Type == 1 && this->GABA == false) {
		this->GABA = true;
		SetIzhSynapseOnGPU<<<1, 1>>>(this->NeuronModelOnGPU, this->AMPA, this->GABA, this->NMDA, this->I_EXT);
	}
	else if (Type == 2 && this->NMDA == false) {
		this->NMDA = true;
		SetIzhSynapseOnGPU<<<1, 1>>>(this->NeuronModelOnGPU, this->AMPA, this->GABA, this->NMDA, this->I_EXT);
	}
	else if (Type == 3) {
		this->I_EXT = true;
		inter->subindex_type = this->CurrentSynapeModel->N_connections[inter->TargetNeuron->index_in_NeuronModel];
		this->CurrentSynapeModel->IncrementNInputCurrentSynapsesPerNeuron(inter->TargetNeuron->index_in_NeuronModel);
		SetIzhSynapseOnGPU<<<1, 1>>>(this->NeuronModelOnGPU, this->AMPA, this->GABA, this->NMDA, this->I_EXT);
	}
	else if (!(Type == 0 || Type == 1 || Type == 2)) {
		std::cout << "Error: Type of synapse is not supported in this model" << std::endl;
	}
}

void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::InitializeInputCurrentSynapseStructure() {
	if (this->CurrentSynapeModel != 0) {
		this->CurrentSynapeModel->InitializeInputCurrentPerSynapseStructure();
	}
}

void TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep) {
	std::map<std::string, boost::any>::iterator iter = parametermap.find("a");
	if (iter != parametermap.end()) { this->a = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("b");
	if (iter != parametermap.end()) { this->b = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("c");
	if (iter != parametermap.end()) { this->c = boost::any_cast<float>(iter->second); this->V_reset = this->c; parametermap.erase(iter); }
	iter = parametermap.find("d");
	if (iter != parametermap.end()) { this->d = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("V_rest");
	if (iter != parametermap.end()) { this->V_rest = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("V_reset");
	if (iter != parametermap.end()) { this->V_reset = boost::any_cast<float>(iter->second); this->c = this->V_reset; parametermap.erase(iter); }
	iter = parametermap.find("V_th");
	if (iter != parametermap.end()) { this->V_th = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("R");
	if (iter != parametermap.end()) { this->R = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("E_ampa");
	if (iter != parametermap.end()) { this->E_ampa = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("ampa_tau");
	if (iter != parametermap.end()) { this->ampa_tau = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("E_gaba");
	if (iter != parametermap.end()) { this->E_gaba = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("gaba_tau");
	if (iter != parametermap.end()) { this->gaba_tau = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("nmda_tau");
	if (iter != parametermap.end()) { this->nmda_tau = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("basetimestep");
	if (iter != parametermap.end()) { this->timestepdouble = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("random_mu");
	if (iter != parametermap.end()) { this->init = boost::any_cast<std::array<float, 6>>(iter->second); parametermap.erase(iter); }
	iter = parametermap.find("random_sigma");
	if (iter != parametermap.end()) { this->sigma = boost::any_cast<std::array<float, 6>>(iter->second); parametermap.erase(iter); }

	iter = parametermap.find("int_method");
	if (iter != parametermap.end()) {
		if (this->integrationMethodGPUInterface != 0) {
			delete this->integrationMethodGPUInterface;
		}
		ModelDescription temp = boost::any_cast<ModelDescription>(iter->second);
		temp.ModelParameter["step"] = basetimestep;
		this->integrationMethodGPUInterface = Integration_method_GPU_Interface_Factory<TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface>::create_Integration_method_GPU_Interface(temp, this);
		parametermap.erase(iter);
	}
	else {
		ModelDescription temp_modelDescription;
		temp_modelDescription.ModelName = "ForwardEulerMethod";
		temp_modelDescription.ModelParameter["step"] = basetimestep;
		this->integrationMethodGPUInterface = Integration_method_GPU_Interface_Factory<TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface>::create_Integration_method_GPU_Interface(temp_modelDescription, this);
	}
}

std::map<std::string, boost::any> TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface::getParameters() {
	std::map<std::string, boost::any> parametermap;
	parametermap["a"] = this->a;
	parametermap["b"] = this->b;
	parametermap["c"] = this->c;
	parametermap["d"] = this->d;
	parametermap["V_rest"] = this->V_rest;
	parametermap["V_reset"] = this->V_reset;
	parametermap["V_th"] = this->V_th;
	parametermap["R"] = this->R;
	parametermap["E_ampa"] = this->E_ampa;
	parametermap["ampa_tau"] = this->ampa_tau;
	parametermap["E_gaba"] = this->E_gaba;
	parametermap["gaba_tau"] = this->gaba_tau;
	parametermap["nmda_tau"] = this->nmda_tau;
	parametermap["random_mu"] = this->init;
	parametermap["random_sigma"] = this->sigma;
	ModelDescription temp;
	temp.ModelParameter = this->integrationMethodGPUInterface->getParameters();
	temp.ModelName = boost::any_cast<std::string>(temp.ModelParameter["name"]);
	parametermap["int_method"] = temp;
	return parametermap;
}
