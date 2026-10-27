#ifndef TIME_DRIVEN_IZHIKEVIC_EXPONENTIAL_DECAY_GPU_CUH
#define TIME_DRIVEN_IZHIKEVIC_EXPONENTIAL_DECAY_GPU_CUH

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenNeuralModelGPU.cuh"
#include "../source_file_realtime_v1_async/ModelFactory/Integration_method_GPU_Factory.cuh"

class TimeDrivenIzhikevic_Exponential_Decay_GPU : public TimeDrivenNeuralModelGPU {
public:
	bool I_EXT = true;
	bool AMPA = false;
	bool NMDA = false;
	bool GABA = false;

	float a = 0.02f;
	float b = 0.2f;
	float c = -65.0f;
	float d = 8.0f;
	float V_rest = -65.0f;
	float V_th = 30.0f;
	float R = 1.0f;
	float E_ampa = 0.0f;
	float ampa_tau = 5.0f;
	float E_gaba = -80.0f;
	float gaba_tau = 10.0f;
	float nmda_tau = 80.0f;

	const int N_NeuronStateVariables = 6;
	const int N_TimedependentInput = 4;
	const int N_DifferentialStates = 2;
	const int index_V = 0;
	const int index_U = 1;
	const int index_ampa = 2;
	const int index_gaba = 3;
	const int index_nmda = 4;
	const int I_EXT_index = 5;
	const int TimeDependentInputSize = 4;

	__device__ virtual void SetEnabledSynapsis(bool new_ampa, bool new_gaba, bool new_nmda, bool new_I_EXT) {
		AMPA = new_ampa;
		GABA = new_gaba;
		NMDA = new_nmda;
		I_EXT = new_I_EXT;
	}

	__device__ TimeDrivenIzhikevic_Exponential_Decay_GPU(
		char* int_Name,
		float new_a,
		float new_b,
		float new_c,
		float new_d,
		float newV_rest,
		float newV_th,
		float newR,
		float new_E_ampa,
		float new_ampa_tau,
		float new_E_gaba,
		float new_gaba_tau,
		float new_nmda_tau,
		int Timestep,
		float timesteps,
		void** new_d_parm,
		int new_N_neurons) : TimeDrivenNeuralModelGPU(Timestep, timesteps),
		a(new_a), b(new_b), c(new_c), d(new_d), V_rest(newV_rest), V_th(newV_th), R(newR),
		E_ampa(new_E_ampa), ampa_tau(new_ampa_tau), E_gaba(new_E_gaba), gaba_tau(new_gaba_tau), nmda_tau(new_nmda_tau) {
		(void)new_N_neurons;
		this->integration_method_on_GPU = Integration_method_GPU_Factory<TimeDrivenIzhikevic_Exponential_Decay_GPU>::create_Integration_method_GPU(int_Name, new_d_parm, this);
		this->integration_method_on_GPU->Calculate_conductance_exp_values();
	}

	__device__ ~TimeDrivenIzhikevic_Exponential_Decay_GPU() {
		delete this->integration_method_on_GPU;
	}

	__device__ virtual void UpdateState(int time, float timestep) {
		int index = blockIdx.x * blockDim.x + threadIdx.x;
		while (index < this->Neuron_State_Vector->NumberofNeurons) {
			if (this->AMPA) {
				this->Neuron_State_Vector->Vector_of_State_VariableGPU[(this->N_DifferentialStates * this->Neuron_State_Vector->NumberofNeurons) + index] += this->Neuron_State_Vector->AuxStateGPU[index];
			}
			if (this->GABA) {
				this->Neuron_State_Vector->Vector_of_State_VariableGPU[((this->N_DifferentialStates + 1) * this->Neuron_State_Vector->NumberofNeurons) + index] += this->Neuron_State_Vector->AuxStateGPU[index + this->Neuron_State_Vector->NumberofNeurons];
			}
			if (this->NMDA) {
				this->Neuron_State_Vector->Vector_of_State_VariableGPU[((this->N_DifferentialStates + 2) * this->Neuron_State_Vector->NumberofNeurons) + index] += this->Neuron_State_Vector->AuxStateGPU[index + this->Neuron_State_Vector->NumberofNeurons * 2];
			}
			if (this->I_EXT) {
				this->Neuron_State_Vector->Vector_of_State_VariableGPU[((this->N_DifferentialStates + 3) * this->Neuron_State_Vector->NumberofNeurons) + index] = this->Neuron_State_Vector->AuxStateGPU[index + (this->Neuron_State_Vector->NumberofNeurons * 3)];
			}
			this->Neuron_State_Vector->LastUpdateGPU[index] = time;
			index += blockDim.x * gridDim.x;
		}
		this->integration_method_on_GPU->CaculateIncreament(this->Neuron_State_Vector->NumberofNeurons, this->Neuron_State_Vector->Vector_of_State_VariableGPU);
	}

	__device__ void InitializeNeuronStateVector(int NumberofState, float* initState, float* AuxStateGPU, float* VectorNeuronStateGPU, int* LastUpdate, int* LastSpike, bool* InternalSpikeGPU, int NumberofNeuron) {
		this->Neuron_State_Vector = new Neuron_State_Vector_on_GPU(NumberofState, initState, AuxStateGPU, VectorNeuronStateGPU, LastUpdate, LastSpike, InternalSpikeGPU, NumberofNeuron);
	}

	__device__ void Caculate_Conductance(int index, float elapsed_time) {
		this->Set_conductance_exp_values(index, 0, expf(-elapsed_time / this->ampa_tau));
		this->Set_conductance_exp_values(index, 1, expf(-elapsed_time / this->gaba_tau));
		this->Set_conductance_exp_values(index, 2, expf(-elapsed_time / this->nmda_tau));
	}

	__device__ void CaculateDifferentialEquation(int index, int NumberofNeuron, float* NeuronState, float* AuxNeuronState, float elapsetime) {
		(void)elapsetime;
		const int batch_offset = gridDim.x * blockDim.x;
		const int GPU_offset = blockDim.x * blockIdx.x + threadIdx.x;
		float V = NeuronState[index];
		float U = NeuronState[index + NumberofNeuron];
		float gAMPA = NeuronState[index + NumberofNeuron * this->index_ampa];
		float gGABA = NeuronState[index + NumberofNeuron * this->index_gaba];
		float gNMDA = NeuronState[index + NumberofNeuron * this->index_nmda];
		float I_ext = NeuronState[index + NumberofNeuron * this->I_EXT_index];

		if (this->Neuron_State_Vector->LastSpikeGPU[index] > 0) {
			float current = 0.0f;
			if (this->AMPA) {
				current += gAMPA * (this->E_ampa - V);
			}
			if (this->GABA) {
				current += gGABA * (this->E_gaba - V);
			}
			if (this->NMDA) {
				float g_nmda_inf = 1.0f / (1.0f + expf(-0.062f * V) * (1.2f / 3.57f));
				current += gNMDA * g_nmda_inf * (this->E_ampa - V);
			}
			current += I_ext;
			AuxNeuronState[GPU_offset] = 0.04f * V * V + 5.0f * V + 140.0f - U + this->R * current;
			AuxNeuronState[batch_offset + GPU_offset] = this->a * (this->b * V - U);
		}
		else {
			AuxNeuronState[GPU_offset] = 0.0f;
			AuxNeuronState[batch_offset + GPU_offset] = 0.0f;
		}
	}

	__device__ void CaculateTimeDependentEquation(int index, int NumberofNeuron, float* NeuronState, float elapsetime, int elapsetimeindex) {
		(void)elapsetime;
		float zerothrethhold = 1e-9f;
		float* conductance_exp_values = this->Get_conductance_exponential_values(elapsetimeindex);
		float* g_ampa = &NeuronState[NumberofNeuron * this->index_ampa + index];
		float* g_gaba = &NeuronState[NumberofNeuron * this->index_gaba + index];
		float* g_nmda = &NeuronState[NumberofNeuron * this->index_nmda + index];

		if (*g_ampa < zerothrethhold) *g_ampa = 0.0f; else *g_ampa *= conductance_exp_values[0];
		if (*g_gaba < zerothrethhold) *g_gaba = 0.0f; else *g_gaba *= conductance_exp_values[1];
		if (*g_nmda < zerothrethhold) *g_nmda = 0.0f; else *g_nmda *= conductance_exp_values[2];
	}

	__device__ void CaculateSpike(float previous_V, float* NeuronState, int index, float timestep) {
		(void)previous_V;
		(void)timestep;
		if (NeuronState[index] >= this->V_th) {
			NeuronState[index] = this->c;
			NeuronState[index + this->Neuron_State_Vector->NumberofNeurons] += this->d;
			this->Neuron_State_Vector->LastSpikeGPU[index] = 0;
			this->integration_method_on_GPU->ResetState(index);
			this->Neuron_State_Vector->InternalSpikeGPU[index] = true;
		}
	}
};

#endif
