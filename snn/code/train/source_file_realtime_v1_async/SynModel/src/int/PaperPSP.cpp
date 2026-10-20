#include "../source_file_realtime_v1_async/SynModel/inc/int/PaperPSP.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Intergration/inc/IntegerationMethod.h"
#include "../source_file_realtime_v1_async/ModelFactory/IntegrationMethodFactory_Syn.h"
#include <cmath>

PaperPSP::PaperPSP(int timestep) : Int(timestep), g_h(nullptr) {
    std::string name = "PaperPSP";
    this->setModelName(name);
}

PaperPSP::~PaperPSP() {
    if (this->g_h != nullptr) {
        delete[] this->g_h;
        this->g_h = nullptr;
    }
}

void PaperPSP::UpdateSynState(float dt) {
    // decay the input stage h, then integrate the output stage g
    for (int i = 0; i < this->NumberOfNeuron; i++) {
        this->g_h[2 * i + 1] *= std::exp(-dt / this->tau);
    }
    this->integrationMethod->CaculateIncreament();
}

void PaperPSP::CaculateDifferentialEquation(float* SynState, float* AuxState, int index) {
    (void)index;
    AuxState[0] = -SynState[0] / this->tau + SynState[1];
}

float* PaperPSP::GetState(int index) {
    return this->g_h + index * 2;
}

float PaperPSP::GetCurrent(float* NeuronState, int index) {
    (void)NeuronState;
    // additive current. The two-stage filter gives g(t) = w*t*exp(-t/tau)
    // (impulse response, no normalisation), so PSP = (e/tau)*g has peak w
    // at t=tau. The e factor is applied exactly once (see header comment).
    return this->psp_scale * this->g_h[index * 2] / this->tau;
}

void PaperPSP::InitSynState(int NumberOfNeurons) {
    this->g_h = new float[NumberOfNeurons * 2]();
    this->NumberOfNeuron = NumberOfNeurons;
    for (int i = 0; i < NumberOfNeurons; i++) {
        this->g_h[2 * i] = this->init[0];
        this->g_h[2 * i + 1] = this->init[1];
    }
}

void PaperPSP::ProcessInput(Interconnections* inter, float* NeuronState, float weight) {
    (void)NeuronState;
    // a spike adds its (signed) weight to the input stage h of the target neuron
    this->g_h[2 * inter->TargetNeuronModelIndex + 1] += weight;
}

std::map<std::string, boost::any> PaperPSP::getParameters() {
    std::map<std::string, boost::any> param;
    param = Int::getParameters();
    param["tau"] = this->tau;
    param["random_mu"] = this->init;
    param["random_sigma"] = this->sigma;
    return param;
}

void PaperPSP::setParameters(std::map<std::string, boost::any> param) {
    std::map<std::string, boost::any>::iterator iter = param.find("tau");
    if (iter != param.end()) {
        this->tau = boost::any_cast<float>(iter->second);
        param.erase(iter);
    }
    iter = param.find("random_mu");
    if (iter != param.end()) {
        this->init = boost::any_cast<std::array<float, 2>>(iter->second);
        param.erase(iter);
    }
    iter = param.find("random_sigma");
    if (iter != param.end()) {
        this->sigma = boost::any_cast<std::array<float, 2>>(iter->second);
        param.erase(iter);
    }

    iter = param.find("int_method");
    if (iter != param.end()) {
        if (this->integrationMethod != 0) {
            delete this->integrationMethod;
        }
        ModelDescription temp = boost::any_cast<ModelDescription>(iter->second);
        this->integrationMethod = IntegrationMethodFactory_Syn<PaperPSP>::createIntegerationMethod(temp, this);
        param.erase(iter);
    }
    Int::setParameters(param);
}
