// Open-loop fixed-weight timing diagnostic. Not an arm-task replacement.
#include "inc/Fig4ClosedLoopNetwork.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include <filesystem>
#include <iostream>

using namespace fig4;
struct Counts {
    std::array<int,16> n{},first{};
    Counts(){first.fill(-1);}
    int winner(int cue) const {
        int l=4+2*cue,r=l+1;
        if(n[l]>n[r])return 0;if(n[r]>n[l])return 1;
        if(first[l]>=0&&(first[r]<0||first[l]<first[r]))return 0;
        if(first[r]>=0&&(first[l]<0||first[r]<first[l]))return 1;
        return 2;
    }
};
int main(int argc,char** argv) {
    try {
        if(argc<3||argc>6)throw std::runtime_error("Usage: fig4_window_probe current out [history [feedback_topology [feedback_scale]]]");
        float current=std::stof(argv[1]);std::string out=argv[2];std::filesystem::create_directories(out);
        std::ofstream summary(out+"/summary.csv"),raw(out+"/spikes.csv"),ledger(out+"/credit.csv");
        summary<<"case_id,seed,cue,decision_ms,feedback_noise,decision_winner,feedback_winner,decision_left,decision_right,feedback_left,feedback_right,fixed_weights\n";
        raw<<"case_id,phase,step,neuron\n";
        ledger<<"trial,phase,cue,action,d,conn_id,state_index,source,target,delay_steps,w_before,w_after,c_begin,c_end,integral_c_dt,integral_carried_c_dt,predicted_dw,actual_dw,clip_correction,round_correction,stdp_positive,stdp_negative,pre_events,paired_events,sum_tau_ms,clip_low_steps,clip_high_steps\n"<<std::setprecision(17);
        int caseId=0;
        for(unsigned seed=1;seed<=10;++seed)for(int cue=0;cue<2;++cue)
        for(int duration : {50,200,2000})for(int continuing=0;continuing<2;++continuing) {
            if(argc>=4&&(duration!=50||continuing!=0))continue;
            NetworkSettings cfg;cfg.seed=seed;cfg.i_rs=current;
            if(argc>=4)cfg.pspHistory=argv[3];
            if(argc>=5)cfg.feedbackTopology=argv[4];
            if(argc>=6)cfg.feedbackScale=std::stof(argv[5]);
            auto sim=build_network(cfg);sim->InitSimulation();auto* rule=get_rule(*sim);validate_feedback(*sim,cfg);
            rule->emissionClock=true;rule->diagnostics=true;auto initial=snapshot(*rule);
            std::array<std::mt19937,4> rng;std::array<std::normal_distribution<float>,4> noise;
            for(int ch=0;ch<4;++ch){rng[ch].seed(seed*131u+(unsigned)ch+1u);noise[ch]=std::normal_distribution<float>(0,20);}
            auto phase=[&](const std::string& name,int steps,bool driven,bool noisy) {
                int start=sim->currenttime[0];Counts c;
                // IDs > 99 omit the rule's optional representative-pair stream.
                rule->BeginPhase(200+caseId,name);
                sim->AddExternalCurrentActivity({start,start},{0,1},
                    {driven&&cue==0?current:0.0f,driven&&cue==1?current:0.0f});
                for(int ch=0;ch<4;++ch) {
                    bool active=noisy&&ch/2==cue;
                    std::vector<int> ts,ns;std::vector<float> vals;
                    for(int s=0;s<(active?steps:1);++s) {
                        ts.push_back(start+s);ns.push_back(12+ch);vals.push_back(active?noise[ch](rng[ch]):0.0f);
                    }
                    sim->AddExternalCurrentActivity(ts,ns,vals);
                }
                for(int s=0;s<steps;++s) {
                    sim->RunSimulationStep(1);int t,n;
                    while(sim->output_spike_driver->RemoveBufferedSpike(t,n)) {
                        if(n>=0&&n<16){c.n[n]++;if(c.first[n]<0)c.first[n]=t;raw<<caseId<<','<<name<<','<<t<<','<<n<<'\n';}
                    }
                    rule->Step(sim.get(),0.0f);
                    if(s==2)for(int n=2;n<8;++n) {
                        auto& neuron=sim->network->neurons[n];
                        float i=neuron.neuron_state_vector->GetNeuronState(neuron.index_in_NeuronModel)[5];
                        if(n<4&&i!=(driven&&cue==n-2?current:0.0f))throw std::runtime_error("Cue current mismatch");
                        if(n>=4&&(!noisy||(n-4)/2!=cue)&&i!=0)throw std::runtime_error("Noise clearing mismatch");
                    }
                }
                rule->EndPhase(ledger,cue,c.winner(cue),0);
                return c;
            };
            phase("warmup",100,false,false);
            auto decision=phase("decision",duration*10,true,true);
            auto feedback=phase("feedback",20000,true,continuing!=0);
            if(snapshot(*rule)!=initial)throw std::runtime_error("Fixed diagnostic weights changed");
            int l=4+2*cue,r=l+1;
            summary<<caseId<<','<<seed<<','<<cue<<','<<duration<<','<<continuing<<','<<decision.winner(cue)<<','<<feedback.winner(cue)<<','
                <<decision.n[l]<<','<<decision.n[r]<<','<<feedback.n[l]<<','<<feedback.n[r]<<",1\n";
            ++caseId;
        }
        std::cout<<"PROBE PASS "<<caseId<<" fixed-weight cases\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
