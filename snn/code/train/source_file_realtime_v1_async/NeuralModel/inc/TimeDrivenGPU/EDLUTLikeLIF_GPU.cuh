#ifndef EDLUT_LIKE_LIF_GPU_CUH
#define EDLUT_LIKE_LIF_GPU_CUH

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenNeuralModelGPU.cuh"
#include "../source_file_realtime_v1_async/ModelFactory/Integration_method_GPU_Factory.cuh"

class EDLUTLikeLIF_GPU : public TimeDrivenNeuralModelGPU {
public:
    float V_rest = -60.0f;
    float tau = 20.0f;
    float V_th = -50.0f;
    float R = 1.0f;
    float V_reset = -60.0f;
    int t_ref = 50;
    float gexc_tau = 5.0f;
    float Eexc = 0.0f;
    float ginh_tau = 10.0f;
    float Einhibitory = -80.0f;

    const int N_NeuronStateVariables = 4;
    const int N_DifferentialStates = 1;
    const int TimeDependentInputSize = 3;
    const int index_V = 0;
    const int index_gexc = 1;
    const int index_ginh = 2;
    const int I_EXT_index = 3;

    bool I_EXT = false;
    bool Excited = false;
    bool Inhibitory = false;

    __device__ EDLUTLikeLIF_GPU(
        char* int_Name,
        float newV_rest,
        float newtau,
        float newV_th,
        float newR,
        float newV_reset,
        int newt_ref,
        float newgexc_tau,
        float newEexc,
        float newginh_tau,
        float newEinhibitory,
        int Timestep,
        float timesteps,
        void** parm,
        int N_neurons)
        : TimeDrivenNeuralModelGPU(Timestep, timesteps),
          V_rest(newV_rest),
          tau(newtau),
          V_th(newV_th),
          R(newR),
          V_reset(newV_reset),
          t_ref(newt_ref),
          gexc_tau(newgexc_tau),
          Eexc(newEexc),
          ginh_tau(newginh_tau),
          Einhibitory(newEinhibitory) {
        this->integration_method_on_GPU =
            Integration_method_GPU_Factory<EDLUTLikeLIF_GPU>::create_Integration_method_GPU(int_Name, parm, this);
        this->integration_method_on_GPU->Calculate_conductance_exp_values();
    }

    __device__ ~EDLUTLikeLIF_GPU() {
        delete this->integration_method_on_GPU;
    }

    __device__ void SetEnabledSynapsis(bool new_Excited, bool new_Inhibited, bool new_I_EXT) {
        Excited = new_Excited;
        Inhibitory = new_Inhibited;
        I_EXT = new_I_EXT;
    }

    __device__ void UpdateState(int time, float timestep) override {
        int index = blockIdx.x * blockDim.x + threadIdx.x;
        while (index < this->Neuron_State_Vector->NumberofNeurons) {
            if (this->Excited) {
                this->Neuron_State_Vector->Vector_of_State_VariableGPU[
                    (this->N_DifferentialStates * this->Neuron_State_Vector->NumberofNeurons) + index] +=
                    this->Neuron_State_Vector->AuxStateGPU[index];
            }
            if (this->Inhibitory) {
                this->Neuron_State_Vector->Vector_of_State_VariableGPU[
                    ((this->N_DifferentialStates + 1) * this->Neuron_State_Vector->NumberofNeurons) + index] +=
                    this->Neuron_State_Vector->AuxStateGPU[index + this->Neuron_State_Vector->NumberofNeurons];
            }
            if (this->I_EXT) {
                this->Neuron_State_Vector->Vector_of_State_VariableGPU[
                    ((this->N_DifferentialStates + 2) * this->Neuron_State_Vector->NumberofNeurons) + index] =
                    this->Neuron_State_Vector->AuxStateGPU[index + (this->Neuron_State_Vector->NumberofNeurons * 2)];
            }
            this->Neuron_State_Vector->LastUpdateGPU[index] = time;
            index += blockDim.x * gridDim.x;
        }

        this->integration_method_on_GPU->CaculateIncreament(
            this->Neuron_State_Vector->NumberofNeurons,
            this->Neuron_State_Vector->Vector_of_State_VariableGPU);
    }

    __device__ void InitializeNeuronStateVector(
        int NumberofState,
        float* initState,
        float* AuxStateGPU,
        float* VectorNeuronStateGPU,
        int* LastUpdate,
        int* LastSpike,
        bool* InternalSpikeGPU,
        int NumberofNeuron) {
        this->Neuron_State_Vector = new Neuron_State_Vector_on_GPU(
            NumberofState,
            initState,
            AuxStateGPU,
            VectorNeuronStateGPU,
            LastUpdate,
            LastSpike,
            InternalSpikeGPU,
            NumberofNeuron);
    }

    __device__ void Caculate_Conductance(int index, float elapsed_time) {
        this->Set_conductance_exp_values(index, 0, exp(-elapsed_time / this->gexc_tau));
        this->Set_conductance_exp_values(index, 1, exp(-elapsed_time / this->ginh_tau));
    }

    __device__ void CaculateDifferentialEquation(
        int index,
        int NumberofNeuron,
        float* NeuronState,
        float* AuxNeuronState,
        float elapsetime) {
        float V = NeuronState[index];
        float gexc = NeuronState[index + NumberofNeuron];
        float ginh = NeuronState[index + (NumberofNeuron * 2)];
        float I_ext = NeuronState[index + (NumberofNeuron * 3)];
        if (this->Neuron_State_Vector->LastSpikeGPU[index] > this->t_ref) {
            float current = 0.0f;
            if (this->Excited) {
                current += gexc * (this->Eexc - V);
            }
            if (this->Inhibitory) {
                current += ginh * (this->Einhibitory - V);
            }
            current += I_ext;
            AuxNeuronState[blockIdx.x * blockDim.x + threadIdx.x] = ((current * this->R) + (this->V_rest - V)) / this->tau;
        } else {
            AuxNeuronState[blockIdx.x * blockDim.x + threadIdx.x] = 0.0f;
        }
    }

    __device__ void CaculateTimeDependentEquation(
        int index,
        int NumberofNeuron,
        float* NeuronState,
        float elapsetime,
        int elapsetimeindex) {
        const float zero_threshold = 1e-9f;
        float* conductance_exp_values = this->Get_conductance_exponential_values(elapsetimeindex);
        if (NeuronState[NumberofNeuron * this->N_DifferentialStates + index] < zero_threshold) {
            NeuronState[NumberofNeuron * this->N_DifferentialStates + index] = 0.0f;
        } else {
            NeuronState[NumberofNeuron * this->N_DifferentialStates + index] *= conductance_exp_values[0];
        }
        if (NeuronState[NumberofNeuron * (this->N_DifferentialStates + 1) + index] < zero_threshold) {
            NeuronState[NumberofNeuron * (this->N_DifferentialStates + 1) + index] = 0.0f;
        } else {
            NeuronState[NumberofNeuron * (this->N_DifferentialStates + 1) + index] *= conductance_exp_values[1];
        }
    }

    __device__ void CaculateSpike(float previous_V, float* NeuronState, int index, float timestep) {
        if (NeuronState[index] > this->V_th) {
            NeuronState[index] = this->V_reset;
            this->Neuron_State_Vector->LastSpikeGPU[index] = 0;
            this->integration_method_on_GPU->ResetState(index);
            this->Neuron_State_Vector->InternalSpikeGPU[index] = true;
        }
    }
};

#endif
