#ifndef TIME_DRIVEN_IZHIKEVIC_EXPONENTIAL_DECAY_GPU_INTERFACE_CUH
#define TIME_DRIVEN_IZHIKEVIC_EXPONENTIAL_DECAY_GPU_INTERFACE_CUH

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenNeuronModelGPU_Interface.cuh"

class CurrentSynapse;
class TimeDrivenIzhikevic_Exponential_Decay_GPU;

#include <array>

class TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface : public TimeDrivenNeuronModelGPU_Interface {
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
	float V_reset = -65.0f;
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

	CurrentSynapse* CurrentSynapeModel;
	TimeDrivenIzhikevic_Exponential_Decay_GPU** NeuronModelOnGPU;
	float timestepdouble;
	std::array<float, 6> init = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	std::array<float, 6> sigma = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

	TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface(int timestep);
	~TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface();

	virtual void DestroyGPUModel();
	virtual void CheckType(Interconnections* inter);
	virtual Neuron_State_Vector* InitState();
	virtual InternalSpike* ProcessSpike(Interconnections* inter, int time);
	virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current);
	virtual void UpdateState(int index, int time, Simulation* simulation);
	virtual void InitStateVector(int NumberOfNeurons, int GPUindex);
	virtual void InitializeClassGPU2(int N_neurons);
	virtual void InitializeVectorNeuronState_GPU2();
	virtual void InitializeInputCurrentSynapseStructure();

	virtual bool compare(NeuronModel* neuronmodel) {
		if (!TimeDrivenNeuronModelGPU_Interface::compare(neuronmodel)) {
			return false;
		}
		TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface* e = dynamic_cast<TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface*>(neuronmodel);
		if (e == NULL) {
			return false;
		}
		bool whether = this->a == e->a && this->b == e->b && this->c == e->c && this->d == e->d &&
			this->V_rest == e->V_rest && this->V_reset == e->V_reset && this->V_th == e->V_th &&
			this->R == e->R && this->E_ampa == e->E_ampa && this->ampa_tau == e->ampa_tau &&
			this->E_gaba == e->E_gaba && this->gaba_tau == e->gaba_tau && this->nmda_tau == e->nmda_tau &&
			this->init == e->init && this->sigma == e->sigma && this->integrationMethodGPUInterface == e->integrationMethodGPUInterface;
		return whether;
	}

	void SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep);
	virtual std::map<std::string, boost::any> getParameters();

	virtual int getV_index() { return this->index_V; }
	virtual int get_NumberOfState() { return this->N_NeuronStateVariables; }
	virtual enum NeuronModelType getNeuronModelType() { return NEURAL_LAYER; }
};

#endif
