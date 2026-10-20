#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/DASTDP.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/Network.h"
#include <boost/any.hpp>
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <set>

DASTDP::DASTDP(std::map<std::string, boost::any> parametermap) : WithPostSynaptic() {
    this->SetParameters(parametermap);
}

DASTDP::~DASTDP() {}

void DASTDP::InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep) {
    (void)NumberOfNeuron;
    (void)basetimestep;
    eligibility.assign(NumberOfConnections, 0.0f);
    lastPostStep.assign(NumberOfConnections, -1000000);
    collected = false;
}

float DASTDP::StdpWindow(float tau) const {
    float z = (tau - this->c_peak) / this->beta;
    return (1.0f - this->b) * std::exp(-z * z) + this->b;
}

void DASTDP::ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation) {
    if (emissionClock) return;
    ApplyEligibility(connection, SpikeTime, simulation);
}

void DASTDP::ApplySourceSpike(Neuron* neuron, int time, Simulation* sim) {
    if (!emissionClock) return;
    for (auto* conn : plastic) {
        if (conn->SourceNeuron == neuron) ApplyEligibility(conn, time, sim);
    }
}

void DASTDP::ApplyEligibility(Interconnections* connection, int SpikeTime, Simulation* simulation) {
    if (this->blockNewEligibility) {
        return;
    }
    int idx = connection->LearningRuleIndex_withPost;
    if (idx < 0 || idx >= (int)eligibility.size()) {
        throw std::runtime_error("Invalid eligibility index");
    }
    float dt = simulation->basetimesteps;
    int lp = lastPostStep[idx];
    float contrib = 0.0f;
    float tau_ms = 0.0f;
    if (lp > -1000000) {
        int referenceStep = SpikeTime + (sourceDelayReference ? connection->delay : 0);
        tau_ms = (float)(lp - referenceStep) * dt; // Delay appears once in the local-pre hypothesis.
        if (referenceSign) tau_ms = -tau_ms;
        contrib = StdpWindow(tau_ms);
    }
    float before = eligibility[idx];
    eligibility[idx] += contrib;
    if (diagnostics && diagnosticTrial >= 0) {
        Accum& a = accum[idx];
        a.events++;
        if (lp > -1000000) { a.pairs++; a.tauSum += tau_ms; }
        if (contrib >= 0) a.positive += contrib; else a.negative += contrib;
        const int delay = connection->delay;
        if ((delay == 10 || delay == 500 || delay == 1000) &&
            (diagnosticTrial < 3 || diagnosticTrial == 10 || diagnosticTrial == 50 || diagnosticTrial == 99)) {
            pairCsv << diagnosticTrial << ',' << diagnosticPhase << ',' << connection->Index << ','
                << idx << ',' << connection->SourceNeuron->Neuron_index << ','
                << connection->TargetNeuron->Neuron_index << ',' << delay << ','
                << (emissionClock ? SpikeTime : SpikeTime-delay) << ','
                << (emissionClock ? SpikeTime+delay : SpikeTime) << ',' << SpikeTime << ','
                << lp << ',' << tau_ms << ',' << contrib << ',' << before << ',' << eligibility[idx] << '\n';
        }
    }
    if (this->tracing && (int)trace.size() < this->traceLimit) {
        TraceRec r;
        r.connIndex = connection->Index;
        r.delayStep = connection->delay;
        r.spikeTime = SpikeTime;
        r.lastPostStep = lp;
        r.tau_ms = tau_ms;
        r.stdp = contrib;
        r.eligBefore = before;
        r.eligAfter = eligibility[idx];
        trace.push_back(r);
    }
}

void DASTDP::ApplyPostSynaticSpike(Neuron* neuron, int SpikeTime, Simulation* simulation) {
    (void)simulation;
    int rid = this->LearningRuleID;
    if (rid < 0) {
        return;
    }
    for (int i = 0; i < neuron->PostSynapticLearning_Number[rid]; i++) {
        int idx = neuron->IndexOfInputLearningIndex[0][rid][i];
        if (idx < 0 || idx >= (int)lastPostStep.size()) {
            continue;
        }
        lastPostStep[idx] = SpikeTime;
    }
}

void DASTDP::Collect(Network* net) {
    plastic.clear();
    plasticState.clear();
    for (int i = 0; i < net->intersNum; i++) {
        Interconnections& ic = net->inters[i];
        if (ic.LearningRule_withPost != 0 &&
            ic.LearningRule_withPost->LearningRuleID == this->LearningRuleID) {
            plastic.push_back(&ic);
            plasticState.push_back(ic.LearningRuleIndex_withPost);
        }
    }
    std::set<int> indices, ids;
    for (size_t k=0; k<plastic.size(); ++k) {
        int idx=plasticState[k];
        if (idx<0 || idx>=(int)eligibility.size() || !indices.insert(idx).second ||
            !ids.insert(plastic[k]->Index).second) throw std::runtime_error("Invalid plastic identity mapping");
    }
    if (indices.size()!=eligibility.size()) throw std::runtime_error("Unbound eligibility state");
    accum.resize(eligibility.size());
    collected = true;
}

void DASTDP::Step(Simulation* simulation, float d) {
    if (!collected) {
        throw std::runtime_error("Invalid eligibility index");
    }
    float dt = simulation->basetimesteps;
    float decay = std::exp(-dt / this->tau_c);
    for (size_t k = 0; k < plastic.size(); k++) {
        int idx = plasticState[k];
        if (idx < 0 || idx >= (int)eligibility.size()) {
            continue;
        }
        eligibility[idx] *= decay;
        Interconnections* conn = plastic[k];
        float w = conn->weight + d * eligibility[idx] * dt;
        if (!std::isfinite(w) || !std::isfinite(eligibility[idx])) {
            throw std::runtime_error("Non-finite DA-STDP state");
        }
        float raw = w;
        float before = conn->weight;
        if (w < 0.0f) {
            w = 0.0f;
        }
        if (w > conn->maximum_weight) {
            w = conn->maximum_weight;
        }
        conn->weight = w;
        if (diagnostics && diagnosticTrial >= 0) {
            Accum& a=accum[idx];
            a.carry *= decay;
            a.integral += (double)eligibility[idx] * dt;
            a.carryIntegral += a.carry * dt;
            double predicted=(double)d * eligibility[idx] * dt;
            a.predicted += predicted;
            a.applied += (double)w-before;
            a.clipping += (double)w-raw;
            a.rounding += ((double)raw-before)-predicted;
            if (raw<0) a.low++;
            if (raw>conn->maximum_weight) a.high++;
        }
    }
}

void DASTDP::BeginPhase(int trial, const std::string& phase) {
    diagnosticTrial=trial; diagnosticPhase=phase;
    for (size_t k=0; k<plastic.size(); ++k) {
        int idx=plasticState[k];
        accum[idx]=Accum{};
        accum[idx].c0=eligibility[idx]; accum[idx].carry=eligibility[idx];
        accum[idx].w0=plastic[k]->weight;
    }
}

void DASTDP::EndPhase(std::ostream& out, int cue, int action, float d) {
    for (size_t k=0; k<plastic.size(); ++k) {
        auto* conn=plastic[k]; int idx=plasticState[k]; const auto& a=accum[idx];
        out << diagnosticTrial << ',' << diagnosticPhase << ',' << cue << ',' << action << ',' << d << ','
            << conn->Index << ',' << idx << ',' << conn->SourceNeuron->Neuron_index << ','
            << conn->TargetNeuron->Neuron_index << ',' << conn->delay << ',' << a.w0 << ',' << conn->weight << ','
            << a.c0 << ',' << eligibility[idx] << ',' << a.integral << ',' << a.carryIntegral << ','
            << a.predicted << ',' << a.applied << ',' << a.clipping << ',' << a.rounding << ','
            << a.positive << ',' << a.negative << ',' << a.events << ',' << a.pairs << ','
            << a.tauSum << ',' << a.low << ',' << a.high << '\n';
    }
}

void DASTDP::SetParameters(std::map<std::string, boost::any> parametermap) {
    std::map<std::string, boost::any>::iterator iter = parametermap.find("tau_c");
    if (iter != parametermap.end()) { this->tau_c = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
    iter = parametermap.find("b");
    if (iter != parametermap.end()) { this->b = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
    iter = parametermap.find("c_peak");
    if (iter != parametermap.end()) { this->c_peak = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
    iter = parametermap.find("beta");
    if (iter != parametermap.end()) { this->beta = boost::any_cast<float>(iter->second); parametermap.erase(iter); }
}

std::map<std::string, boost::any> DASTDP::GetParameters() {
    std::map<std::string, boost::any> parametermap;
    parametermap["tau_c"] = this->tau_c;
    parametermap["b"] = this->b;
    parametermap["c_peak"] = this->c_peak;
    parametermap["beta"] = this->beta;
    return parametermap;
}
