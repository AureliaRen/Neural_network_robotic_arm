#ifndef TIME_DRIVEN_LIF_EXPONENTIAL_DOUBLE_H
#define TIME_DRIVEN_LIF_EXPONENTIAL_DOUBLE_H

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenModel.h"
#include <array>
#include <vector>
class CurrentSynapse;

class TimeDrivenLIF_Exponential_double : public TimeDrivenModel {
public:
    // 是否接受各种外部输入
    bool I_EXT = true;
    bool Excited = false;
    bool Inhibitory = false;
    // 神经元参数
    float V_rest = -60.;
    float tau = 20.;
    float V_th = -50.;
    float R = 1.;
    float V_reset = -60.;
    int t_ref = 50;
    float gexc_tau = 5.;
    float Eexc = 0.;
    float ginh_tau = 10.;
    float Einhibitory = -80.;
    // 索引参数
    const int N_NeuronStateVariables = 4;
    const int N_DifferentialStates = 1;
    const int index_V = 0;
    const int index_gexc = 1;
    const int index_ginh = 2;
    const int TimeDependentInputSize = 3;
    const int I_EXT_index = 3;
    // 电流突触模型
    CurrentSynapse* CurrentSynapeModel;
	
    std::array<float, 4> init = {0.0, 0.0, 0.0, 0.0};
    std::array<float, 4> sigma = {0.0, 0.0, 0.0, 0.0};
    /*
    * 构造函数
    */
    TimeDrivenLIF_Exponential_double();
    /*
    */
    TimeDrivenLIF_Exponential_double(int timesteps);
    ~TimeDrivenLIF_Exponential_double();

    virtual void InitStateVector(int NumberOfNeurons, int GPUIndex);
    virtual Neuron_State_Vector* InitState();
    virtual void UpdateState(int index, int time, Simulation* simulatiuon);
    void CaculateDifferentialEquation(float* NeuronState, float* AuxNeuronState, int index);
    void CaculateTimeDependentEquation(float* NeuronState, int index, float dt);
    void CaculateSpike(float previous_V, float* NeuronState, int index);
    virtual InternalSpike* ProcessSpike(Interconnections* inter, int time);
    virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current);
    virtual void InitializeInputCurrentSynapseStructure();
    virtual void CheckType(Interconnections* inter);
    virtual int getV_index();
    virtual int get_NumberOfState();
    virtual enum NeuronModelType getNeuronModelType();
    virtual bool compare(NeuronModel* neuralmodel) {
        if (!TimeDrivenModel::compare(neuralmodel)) {
            return false;
        }
        TimeDrivenLIF_Exponential_double* e = dynamic_cast<TimeDrivenLIF_Exponential_double*>(neuralmodel);
        if (e == NULL) {
            return false;
        }
        bool whether = this->V_rest == e->V_rest && this->tau == e->tau && this->V_th == e->V_th && this->R == e->R && this->V_reset == e->V_reset && this->t_ref == e->t_ref && this->gexc_tau == e->gexc_tau && this->Eexc == e->Eexc && this->ginh_tau == e->ginh_tau && this->Einhibitory == e->Einhibitory && this->init == e->init && this->sigma == e->sigma;
        return whether;
    }
    void SetParameters(std::map<std::string, boost::any> parametermap, float timestep);
    virtual std::map<std::string, boost::any> getParameters();

private:
    std::vector<float> gexc_decay_lookup_;
    std::vector<float> ginh_decay_lookup_;

    void ResetConductanceDecayLookup(float dt);
    void EnsureConductanceDecayLookupSize(float dt);
};

#endif
