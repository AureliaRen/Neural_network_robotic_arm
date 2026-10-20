#ifndef FIG4_CLOSED_LOOP_NETWORK_H
#define FIG4_CLOSED_LOOP_NETWORK_H
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/DASTDP.h"
#include <array>
#include <random>
#include <list>
#include <memory>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace fig4 {
constexpr float DT=.1f;
constexpr int N_SYN=100;
struct NetworkSettings {
    unsigned seed=1;
    std::string pspHistory="baseline";
    std::string feedbackTopology="bank";
    float feedbackScale=1.0f,feedbackJitter=.10f;
    float w_mean=.5f,jitter=.10f,w_max=2.0f,ch_lts=30.0f,lts_ch=40.0f,i_rs=22.5f,noise=20.0f;
    float tau_c=5.0f;
    bool emission=true;
    bool sourceDelayReference=false;
};
inline NeuronLayerDescription izh(int n,float a,float b,float c,float d) {
    NeuronLayerDescription l;
    l.ModelName="TimeDrivenIzhikevic_Exponential_Decay";
    l.numberofneuron=n;l.isOutput=true;l.isMonitored=false;
    l.NeuronParameter["a"]=a;l.NeuronParameter["b"]=b;l.NeuronParameter["c"]=c;l.NeuronParameter["d"]=d;
    l.NeuronParameter["V_rest"]=-65.0f; // c is the sole reset parameter; V_reset is an overriding alias.
    l.NeuronParameter["V_th"]=30.0f;l.NeuronParameter["R"]=1.0f;
    l.NeuronParameter["ampa_tau"]=5.0f;l.NeuronParameter["gaba_tau"]=10.0f;
    l.NeuronParameter["random_sigma"]=std::array<float,6>{0,0,0,0,0,0};
    return l;
}
inline void single(std::list<ConnectionDescription>& cs,int source,int target,int type,float weight,
                   int delay,int rule,float maxw,const ModelDescription& model) {
    ConnectionDescription c;c.SourceNeuron={source};c.TargetNeuron={target};c.Type={type};
    c.Weight={weight};c.MaxWeight={maxw};c.Delay={delay};c.SynapseRule={rule};c.TriggerRule={-1};c.SynapseModel={model};cs.push_back(c);
}
inline std::unique_ptr<Simulation> build_network(const NetworkSettings& cfg) {
    std::list<NeuronLayerDescription> ls;
    NeuronLayerDescription cue;cue.ModelName="InputCurrentNeuronModel";cue.numberofneuron=2;ls.push_back(cue);
    ls.push_back(izh(2,.02f,.20f,-65.0f,8.0f));ls.push_back(izh(4,.02f,.20f,-50.0f,2.0f));
    ls.push_back(izh(4,.02f,.25f,-65.0f,2.0f));
    NeuronLayerDescription noise;noise.ModelName="InputCurrentNeuronModel";noise.numberofneuron=4;ls.push_back(noise);
    ModelDescription psp;psp.ModelName="PaperPSP";psp.ModelParameter["tau"]=5.0f;
    if(cfg.pspHistory!="baseline") {
        psp.ModelName="PaperPSPHistory";psp.ModelParameter["dt"]=DT;
        psp.ModelParameter["history"]=cfg.pspHistory=="latest-source"?2:cfg.pspHistory=="latest-arrival"?1:0;
        psp.ModelParameter["current_weight"]=cfg.pspHistory!="sum-arrival";
        if(cfg.pspHistory!="sum-arrival" && cfg.pspHistory!="sum-current" &&
           cfg.pspHistory!="latest-arrival" && cfg.pspHistory!="latest-source")
            throw std::runtime_error("Bad PSP history policy");
    }
    std::list<ConnectionDescription> cs;
    single(cs,0,2,3,1.0f,1,-1,1.0f,{});single(cs,1,3,3,1.0f,1,-1,1.0f,{});
    std::mt19937 gen(cfg.seed);
    for(int group=0;group<4;++group) {
        ConnectionDescription c;std::normal_distribution<float> perturb(0.0f,cfg.w_mean*cfg.jitter);
        for(int k=0;k<N_SYN;++k) {
            c.SourceNeuron.push_back(2+group/2);c.TargetNeuron.push_back(4+group);c.Type.push_back(4);
            c.Weight.push_back(cfg.w_mean+perturb(gen));c.MaxWeight.push_back(cfg.w_max);
            c.Delay.push_back(10+990*k/99);c.SynapseRule.push_back(0);c.TriggerRule.push_back(-1);c.SynapseModel.push_back(psp);
        }
        cs.push_back(c);
    }
    if(cfg.feedbackTopology!="single"&&cfg.feedbackTopology!="collapsed"&&cfg.feedbackTopology!="bank")
        throw std::runtime_error("Bad feedback topology");
    if(!std::isfinite(cfg.feedbackScale)||cfg.feedbackScale<0||!std::isfinite(cfg.feedbackJitter)||cfg.feedbackJitter<0)
        throw std::runtime_error("Bad fixed feedback strength");
    std::mt19937 feedbackGen(cfg.seed*15427u+17u);
    auto profile=[&](float total) {
        std::vector<float> w(N_SYN);std::normal_distribution<float> perturb(0,cfg.feedbackJitter);
        double sum=0;for(auto& x:w){x=1.0f+perturb(feedbackGen);if(x<=0)throw std::runtime_error("Nonpositive fixed-bank sample");sum+=x;}
        for(auto& x:w)x=static_cast<float>(x/sum*total*cfg.feedbackScale);return w;
    };
    auto exc=profile(cfg.ch_lts),inh=profile(cfg.lts_ch);
    auto feedback=[&](int source,int target,float total,int legacyDelay,const std::vector<float>& weights,bool inhibitory) {
        if(cfg.feedbackTopology=="single") {
            float w=total*cfg.feedbackScale;single(cs,source,target,4,inhibitory?-w:w,legacyDelay,-1,w,psp);return;
        }
        for(int k=0;k<N_SYN;++k) {
            int delay=cfg.feedbackTopology=="collapsed"?legacyDelay:10+990*k/99;
            single(cs,source,target,4,inhibitory?-weights[k]:weights[k],delay,-1,weights[k],psp);
        }
    };
    for(int k=0;k<4;++k)feedback(4+k,8+k,cfg.ch_lts,10,exc,false);
    for(int k=0;k<4;++k)feedback(8+k,4+(k^1),cfg.lts_ch,20,inh,true);
    for(int k=0;k<4;++k)single(cs,12+k,4+k,3,1.0f,1,-1,1.0f,{});
    LearningRuleDescription da;da.RuleName="DA-STDP";da.RuleParameter["tau_c"]=cfg.tau_c;
    da.RuleParameter["b"]=-.21f;da.RuleParameter["c_peak"]=-12.7f;da.RuleParameter["beta"]=13.61f;
    // No absolute ending event: the harness runs a bounded number of relative steps.
    auto sim=std::make_unique<Simulation>(ls,cs,std::list<LearningRuleDescription>{da},0,DT,1);
    return sim;
}
inline void validate_feedback(Simulation& sim,const NetworkSettings& cfg) {
    int expected=cfg.feedbackTopology=="single"?1:N_SYN;
    for(int kind=0;kind<2;++kind)for(int k=0;k<4;++k) {
        int source=kind?8+k:4+k,target=kind?4+(k^1):8+k;
        std::vector<int> delays;double sum=0;
        for(int i=0;i<sim.network->intersNum;++i) {
            auto& c=sim.network->inters[i];
            if(c.SourceNeuron->Neuron_index!=source||c.TargetNeuron->Neuron_index!=target)continue;
            if(c.type!=4||c.LearningRule_withPost||c.LearningRule_withTrigger||c.LearningRule_withPostAndTrigger||
               (kind?c.weight>0:c.weight<0))throw std::runtime_error("Fixed feedback sign/rule mismatch");
            delays.push_back(c.delay);sum+=c.weight;
        }
        if(delays.size()!=static_cast<size_t>(expected))throw std::runtime_error("Fixed feedback multiplicity mismatch");
        std::sort(delays.begin(),delays.end());
        for(int i=0;i<expected;++i) {
            int d=cfg.feedbackTopology=="bank"?10+990*i/99:kind?20:10;
            if(delays[i]!=d)throw std::runtime_error("Fixed feedback delay mismatch");
        }
        double wanted=(kind?-cfg.lts_ch:cfg.ch_lts)*cfg.feedbackScale;
        if(std::fabs(sum-wanted)>1e-4*std::max(1.0,std::fabs(wanted)))throw std::runtime_error("Fixed feedback total-strength mismatch");
    }
    if(sim.network->intersNum!=406+8*expected)throw std::runtime_error("Network connection count mismatch");
}
inline DASTDP* get_rule(Simulation& sim) {
    // Validate effective factory values, not just requested descriptor keys.
    for(int n=2;n<12;++n) {
        auto p=sim.network->neurons[n].neuron_model->getParameters();
        float expectedB=n<8?.20f:.25f,expectedC=n>=4&&n<8?-50.0f:-65.0f;
        float expectedD=n<4?8.0f:2.0f;
        if(boost::any_cast<float>(p.at("a"))!=.02f || boost::any_cast<float>(p.at("b"))!=expectedB ||
           boost::any_cast<float>(p.at("c"))!=expectedC || boost::any_cast<float>(p.at("d"))!=expectedD ||
           boost::any_cast<float>(p.at("V_reset"))!=expectedC)
            throw std::runtime_error("Effective Izhikevich parameters differ from paper; check reset alias");
    }
    auto* rule=dynamic_cast<DASTDP*>(sim.network->LearningRules[0]);
    if(!rule)throw std::runtime_error("DA-STDP factory mismatch");
    rule->Collect(sim.network);
    if(rule->plastic.size()!=400)throw std::runtime_error("Expected 400 plastic connections");
    return rule;
}
inline std::vector<float> snapshot(DASTDP& rule) {
    std::vector<float> v;for(auto* c:rule.plastic)v.push_back(c->weight);return v;
}
inline void save_weights(DASTDP& rule,const std::string& path) {
    std::ofstream out(path);if(!out)throw std::runtime_error("Cannot save weights");
    out<<"conn_id,source,target,delay_steps,weight\n"<<std::setprecision(9);
    for(auto* c:rule.plastic)out<<c->Index<<','<<c->SourceNeuron->Neuron_index<<','<<c->TargetNeuron->Neuron_index<<','<<c->delay<<','<<c->weight<<'\n';
}
inline void load_weights(DASTDP& rule,const std::string& path) {
    struct W {int id,source,target,delay;float weight;};std::map<int,W> ws;
    std::ifstream in(path);std::string line;
    std::getline(in,line);if(line!="conn_id,source,target,delay_steps,weight")throw std::runtime_error("Bad weight CSV header");
    while(std::getline(in,line)) {
        std::replace(line.begin(),line.end(),',',' ');std::istringstream row(line);W w;std::string extra;
        if(!(row>>w.id>>w.source>>w.target>>w.delay>>w.weight)||(row>>extra)||!std::isfinite(w.weight)||!ws.emplace(w.id,w).second)
            throw std::runtime_error("Invalid weight CSV row");
    }
    if(ws.size()!=rule.plastic.size())throw std::runtime_error("Weight count mismatch");
    for(auto* c:rule.plastic) {
        auto it=ws.find(c->Index);
        if(it==ws.end()||it->second.source!=c->SourceNeuron->Neuron_index||it->second.target!=c->TargetNeuron->Neuron_index||
            it->second.delay!=c->delay||it->second.weight<0||it->second.weight>c->maximum_weight)
            throw std::runtime_error("Weight identity/range mismatch");
        c->weight=it->second.weight;
    }
}
}
#endif
