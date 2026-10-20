/*
* PaperPSP.h
* Additive-current PSP synapse for Wei, Bu & Zhu (BSPC 2020) Eq.(4):
*
*     PSP(x) = w * (x/tau) * exp(1 - x/tau),   x = t - t_pre - t_delay
*     PSP(x) = 0,                               x < 0
*
* The delay (x<0 -> 0) is handled by the event scheduler: a spike is only
* delivered when it arrives, so this kernel only ever sees x >= 0.
*
* Implementation: a two-stage (alpha) linear filter, both stages with the
* same time constant tau:
*     h' = -h/tau            (input stage; a spike adds w to h)
*     g' = -g/tau + h        (output stage)
* Its impulse response is g(x) = w * x * exp(-x/tau) (the UN-normalised
* alpha function). Dividing by tau and multiplying by e gives the paper's
* normalised PSP with peak exactly w at x=tau:
*     PSP = (e/tau) * g = w * (x/tau) * exp(1 - x/tau).
* >>> The e factor below is applied exactly once, because the filter gives
*     exp(-x/tau) (no e); do NOT interpret it as an extra exp(1) on top of
*     the paper formula. The 1/tau is the standard alpha normalisation. <<<
*
* GetCurrent returns the additive current (NOT a conductance g*(E-V)), so
* the input current is w>0 excitatory / w<0 inhibitory directly by sign.
*/
#ifndef PAPERPSP_H
#define PAPERPSP_H

class NeuronModel;

#include "../source_file_realtime_v1_async/SynModel/inc/int/Int.h"
#include <array>

class PaperPSP : public Int {
public:
    int NumberOfJumpVariable = 1;

    int NumberOfNeuron = 0;

    const int N_DifferentialStates = 1;

    float* g_h = nullptr;   // per neuron: [g, h]

    float tau = 5.0f;                        // ms (paper uses tau = 5.0)

    float psp_scale = 2.718281828459045f;    // e, normalises peak to w

    std::array<float, 2> init = { 0.0f, 0.0f };   // initial state

    std::array<float, 2> sigma = { 0.0f, 0.0f };  // unused (kept for parity)

    PaperPSP(int timestepsize);

    virtual ~PaperPSP();

    virtual void UpdateSynState(float dt);

    virtual void CaculateDifferentialEquation(float* SynState, float* AuxState, int index);

    virtual float* GetState(int index);

    virtual float GetCurrent(float* NeuronState, int index);

    virtual void InitSynState(int NumberOfNeurons);

    virtual void ProcessInput(Interconnections* inter, float* NeuronState, float weight);

    virtual std::map<std::string, boost::any> getParameters();

    virtual void setParameters(std::map<std::string, boost::any> parameters);

    virtual bool compare(Synmodel* synmodel) {
        if (!Int::compare(synmodel)) {
            return false;
        }
        PaperPSP* e = dynamic_cast<PaperPSP*>(synmodel);
        if (e == NULL) {
            return false;
        }
        return this->tau == e->tau && this->init == e->init && this->sigma == e->sigma;
    }
};

#endif
