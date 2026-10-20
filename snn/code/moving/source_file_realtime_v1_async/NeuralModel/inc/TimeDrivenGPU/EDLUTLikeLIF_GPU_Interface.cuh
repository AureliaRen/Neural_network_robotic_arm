#ifndef EDLUT_LIKE_LIF_GPU_INTERFACE_CUH
#define EDLUT_LIKE_LIF_GPU_INTERFACE_CUH

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenNeuronModelGPU_Interface.cuh"
#include <array>

class CurrentSynapse;
class EDLUTLikeLIF_GPU;

class EDLUTLikeLIF_GPU_Interface : public TimeDrivenNeuronModelGPU_Interface {
public:
    bool I_EXT = false;
    bool Excited = false;
    bool Inhibitory = false;

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
    const int N_TimedependentInput = 3;
    const int N_DifferentialStates = 1;
    const int index_V = 0;

    CurrentSynapse* CurrentSynapeModel;
    EDLUTLikeLIF_GPU** NeuronModelOnGPU;
    float timestepdouble = 0.0f;

    std::array<float, 4> init = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> sigma = { 0.0f, 0.0f, 0.0f, 0.0f };

    EDLUTLikeLIF_GPU_Interface(int timestep);
    ~EDLUTLikeLIF_GPU_Interface();

    void DestroyGPUModel() override;
    void CheckType(Interconnections* inter) override;
    Neuron_State_Vector* InitState() override;
    InternalSpike* ProcessSpike(Interconnections* inter, int time) override;
    void ProcessCurrent(Interconnections* inter, Neuron* Target, float current) override;
    void UpdateState(int index, int time, Simulation* simulation) override;
    void InitStateVector(int NumberOfNeurons, int GPUindex) override;
    void InitializeClassGPU2(int N_neurons) override;
    void InitializeVectorNeuronState_GPU2() override;
    void InitializeInputCurrentSynapseStructure() override;

    bool compare(NeuronModel* neuronmodel) override {
        if (!TimeDrivenNeuronModelGPU_Interface::compare(neuronmodel)) {
            return false;
        }
        auto* e = dynamic_cast<EDLUTLikeLIF_GPU_Interface*>(neuronmodel);
        if (e == nullptr) {
            return false;
        }
        return this->I_EXT == e->I_EXT &&
            this->Excited == e->Excited &&
            this->Inhibitory == e->Inhibitory &&
            this->V_rest == e->V_rest &&
            this->tau == e->tau &&
            this->V_th == e->V_th &&
            this->R == e->R &&
            this->V_reset == e->V_reset &&
            this->t_ref == e->t_ref &&
            this->gexc_tau == e->gexc_tau &&
            this->Eexc == e->Eexc &&
            this->ginh_tau == e->ginh_tau &&
            this->Einhibitory == e->Einhibitory &&
            this->init == e->init &&
            this->sigma == e->sigma &&
            this->integrationMethodGPUInterface == e->integrationMethodGPUInterface;
    }

    void SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep);
    std::map<std::string, boost::any> getParameters() override;

    int getV_index() override { return this->index_V; }
    int get_NumberOfState() override { return this->N_NeuronStateVariables; }
    enum NeuronModelType getNeuronModelType() override { return NEURAL_LAYER; }
};

#endif
