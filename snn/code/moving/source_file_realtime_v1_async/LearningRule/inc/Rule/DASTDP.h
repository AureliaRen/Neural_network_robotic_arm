/*
* DASTDP.h
* Dopamine-modulated STDP (Wei, Bu & Zhu 2020 Eq.(5)-(8)) as a framework
* LearningRule (WithPostSynaptic).
*   Eq.(5) dc/dt = -c/tau_c + STDP(tau) delta(t-t_pre)
*   Eq.(6) ds/dt = c * d
*   Eq.(7) d = +lr / 0 / -lr          (set by the caller; not a membrane current)
*   Eq.(8) STDP(tau) = (1-b) exp(-(tau-c_peak)^2/beta^2) + b
* Baseline ApplyPreSynaticSpike receives ARRIVAL time (delay already included).
* Optional emissionClock uses the RS source event and skips arrival learning.
* Optional referenceSign uses the opposite convention in Wei et al. 2017 Fig.7;
* this is a separate reference-based alternative, not the 2020 paper definition.
* Note: this implementation pairs the CURRENT presynaptic event with the MOST
* RECENT postsynaptic spike (a paper-unspecified assumption).
*/
#ifndef LEARNINGRULE_DASTDP_H
#define LEARNINGRULE_DASTDP_H

#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/WithPostSynaptic.h"
#include <vector>
#include <fstream>
#include <string>

class Interconnections;
class Neuron;
class Simulation;
class Network;

class DASTDP : public WithPostSynaptic {
public:
    float b = -0.21f;
    float c_peak = -12.7f;
    float beta = 13.61f;
    float tau_c = 5.0f;

    std::vector<float> eligibility;
    std::vector<int> lastPostStep;

    std::vector<Interconnections*> plastic;
    std::vector<int> plasticState;
    bool collected = false;

    struct TraceRec {
        int connIndex;
        int delayStep;
        int spikeTime;
        int lastPostStep;
        float tau_ms;
        float stdp;
        float eligBefore;
        float eligAfter;
    };
    std::vector<TraceRec> trace;
    int traceLimit = 40;
    bool tracing = false;
    bool blockNewEligibility = false;
    bool emissionClock = false;
    bool sourceDelayReference = false; // Explicit hypothesis: source impulse, local pre reference.
    bool referenceSign = false; // alternate convention from Wei et al. 2017 Fig.7
    bool diagnostics = false;
    int diagnosticTrial = -1;
    std::string diagnosticPhase;
    struct Accum {
        double c0=0, carry=0, integral=0, carryIntegral=0;
        double predicted=0, applied=0, clipping=0, rounding=0;
        double positive=0, negative=0, tauSum=0;
        float w0=0;
        long long events=0, pairs=0, low=0, high=0;
    };
    std::vector<Accum> accum;
    std::ofstream pairCsv;
    void BeginPhase(int trial, const std::string& phase);
    void EndPhase(std::ostream& out, int cue, int action, float d);
    void ApplySourceSpike(Neuron* neuron, int time, Simulation* sim);
    void ApplyEligibility(Interconnections* conn, int time, Simulation* sim);

    DASTDP(std::map<std::string, boost::any> parametermap);
    virtual ~DASTDP();

    virtual void InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep);
    virtual void ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation);
    virtual void ApplyPostSynaticSpike(Neuron* neuron, int SpikeTime, Simulation* simulation);

    float StdpWindow(float tau) const;
    void Collect(Network* net);
    void Step(Simulation* simulation, float d);

    void SetParameters(std::map<std::string, boost::any> parametermap);
    virtual std::map<std::string, boost::any> GetParameters();
};

#endif
