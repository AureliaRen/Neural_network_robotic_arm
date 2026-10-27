#ifndef LEARNINGRULE_R_STDP_H
#define LEARNINGRULE_R_STDP_H

#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/WithTriggerAndPostSynaptic.h"

class R_STDP : public WithTriggerAndPostSynaptic {
public:
    float MaxLTP = 0.92f;

    float LTP_tau = 16.8f;

    float MaxLTD = 0.53f;

    float LTD_tau = 33.1f;

    float RewardFactor = 1.0f;

    float PunishmentFactor = -1.0f;

    bool ClearEligibilityAfterTrigger = true;

    R_STDP(std::map<std::string, boost::any> parametermap);

    virtual ~R_STDP();

    virtual void InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep);

    virtual void ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation);

    virtual void ApplyPostSynaticSpike(Neuron* neuron, int SpikeTime, Simulation* simulation);

    void SetParameters(std::map<std::string, boost::any> parametermap);

    virtual std::map<std::string, boost::any> GetParameters();
};

#endif //LEARNINGRULE_R_STDP_H
