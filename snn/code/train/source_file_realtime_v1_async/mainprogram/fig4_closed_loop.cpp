// Continuous virtual coordinate task; paper-defined rules and explicit assumptions.
// No correct-action currents, action-specific plasticity, or seed selection.
#include "inc/Fig4ClosedLoopNetwork.h"
#include "inc/Fig4VirtualEnvironment.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include <filesystem>
#include <iostream>
#include <limits>
#include <algorithm>

using namespace fig4;
struct Options : NetworkSettings {
    std::string mode="control",center="previous",out="closed_loop_output";
    int cycles=300,decisionSteps=500,feedbackSteps=20000,motionSteps=0;
    double initial=std::numeric_limits<double>::quiet_NaN(),moveStep=1,target=0,tolerance=.25,limit=12;
    bool feedbackNoise=false,centerHold=false,diagnostics=false,evaluation=false;
    bool phaseNoise=false; // Alternative temporal correlation; same marginal sigma.
    bool shadowControl=true; // Matched-schedule diagnostic; paper control also tested.
    std::string weights;
    unsigned evalSeed=4242;
    int evalTrials=100,evalCycles=40;
};
struct PhaseCounts {
    std::array<int,16> count{};
    std::array<int,16> first{};
    PhaseCounts() {first.fill(-1);}
    int decode(int cue) const {
        int a=4+2*cue,b=a+1;
        if(count[a]>count[b])return TURN_LEFT;
        if(count[b]>count[a])return TURN_RIGHT;
        if(first[a]>=0&&(first[b]<0||first[a]<first[b]))return TURN_LEFT;
        if(first[b]>=0&&(first[a]<0||first[b]<first[a]))return TURN_RIGHT;
        return NO_DECISION;
    }
};
struct Outcome {
    int cycles=0,arrivals=0,learningBlocks=0,none=0,boundaries=0,moves=0;
    std::array<int,2> exposure{},toward{};
    double initial=0,final=0,sumAbsDistance=0,maxWeightChange=0;
};

static double starting_position(const Options& cfg) {
    if(std::isfinite(cfg.initial))return cfg.initial;
    std::mt19937 gen(cfg.seed^0x63a9e51bu);
    std::uniform_int_distribution<int> side(0,1),distance(2,8);
    int sign=side(gen)?1:-1;
    return sign*distance(gen)*cfg.moveStep;
}

class TrajectoryRunner {
    Options cfg;
    std::unique_ptr<Simulation> sim;
    DASTDP* rule=nullptr;
    std::array<std::mt19937,4> rng;
    std::array<std::normal_distribution<float>,4> noise;
    std::ofstream phases,counts,mechanism,trajectory,inputBoundary;
    std::ostream* externalTrajectory=nullptr;
    int trialId=0;
    std::vector<float> initialWeights;
public:
    TrajectoryRunner(const Options& opt,unsigned noiseSeed,std::ostream* trace=nullptr,int trial=0)
        : cfg(opt),sim(build_network(opt)),externalTrajectory(trace),trialId(trial) {
        if(!cfg.evaluation) {
            std::filesystem::create_directories(cfg.out);
            if(cfg.diagnostics)sim->AddFileOutputSpikeDriver(cfg.out+"/spikes.txt");
        }
        sim->InitSimulation();rule=get_rule(*sim);validate_feedback(*sim,cfg);rule->emissionClock=cfg.emission;
        rule->sourceDelayReference=cfg.sourceDelayReference;
        if(!cfg.weights.empty())load_weights(*rule,cfg.weights);
        initialWeights=snapshot(*rule);
        for(int ch=0;ch<4;++ch) {
            rng[ch].seed(noiseSeed*131u+(unsigned)ch+1u);
            noise[ch]=std::normal_distribution<float>(0.0f,cfg.noise);
        }
        if(!cfg.evaluation) {
            save_weights(*rule,cfg.out+"/weights_init.csv");
            trajectory.open(cfg.out+"/trajectory.csv");trajectory<<std::setprecision(17);writeTrajectoryHeader(trajectory);
            phases.open(cfg.out+"/phase_schedule.csv");
            phases<<"cycle,phase,observed_cue,driven_cue,readout_cue,position,action,d,start_step,end_step,steps\n"<<std::setprecision(17);
            counts.open(cfg.out+"/phase_spikes.csv");counts<<"cycle,phase,neuron,count,first_step\n";
            if(cfg.diagnostics) {
                inputBoundary.open(cfg.out+"/input_boundary.csv");
                inputBoundary<<"cycle,phase,step_in_phase,neural_step,driven_cue,noise_enabled,readout_cue,neuron,I_ext\n"<<std::setprecision(9);
                rule->diagnostics=true;
                mechanism.open(cfg.out+"/mechanism_synapses.csv");mechanism<<std::setprecision(17);
                mechanism<<"trial,phase,cue,action,d,conn_id,state_index,source,target,delay_steps,w_before,w_after,c_begin,c_end,integral_c_dt,integral_carried_c_dt,predicted_dw,actual_dw,clip_correction,round_correction,stdp_positive,stdp_negative,pre_events,paired_events,sum_tau_ms,clip_low_steps,clip_high_steps\n";
                rule->pairCsv.open(cfg.out+"/mechanism_pairs.csv");rule->pairCsv<<std::setprecision(10);
                rule->pairCsv<<"trial,phase,conn_id,state_index,source,target,delay_steps,raw_pre_step,arrival_step,learning_step,last_post_step,tau_ms,stdp,c_before,c_after\n";
            }
        }
    }
    static void writeTrajectoryHeader(std::ostream& out) {
        out<<"trial,cycle,position_before,observed_cue,driven_cue,readout_cue,feedback_pending,feedback_start,feedback_end,decision_start,decision_end,motion_end,n_left,n_right,first_left,first_right,raw_action,action,requested_position,position_after,toward,entered_target,boundary\n";
    }
    int now() const {return sim->currenttime[0];}
    void cue_input(int driven) {
        sim->AddExternalCurrentActivity({now(),now()},{0,1},
            {driven==LEFT?cfg.i_rs:0.0f,driven==RIGHT?cfg.i_rs:0.0f});
    }
    void noise_input(int steps,int readout,bool enable) {
        // Exactly one event per channel at phase start; no zero/sample collisions.
        for(int ch=0;ch<4;++ch) {
            bool active=enable && ch/2==readout;
            float held=active&&cfg.phaseNoise?noise[ch](rng[ch]):0.0f;
            std::vector<int> times,ids;std::vector<float> values;
            int n=active?steps:1;
            for(int s=0;s<n;++s) {
                times.push_back(now()+s);ids.push_back(12+ch);values.push_back(active?(cfg.phaseNoise?held:noise[ch](rng[ch])):0.0f);
            }
            sim->AddExternalCurrentActivity(times,ids,values);
        }
    }
    PhaseCounts phase(int cycle,const std::string& name,int observed,int driven,int readout,
                      double position,int steps,float d,int precedingAction) {
        if(steps<=0)throw std::runtime_error("Empty neural phase");
        int begin=now();PhaseCounts spikes;
        if(cfg.diagnostics && !cfg.evaluation)rule->BeginPhase(cycle,name);
        cue_input(driven);
        bool noiseEnabled=name=="decision" || (name=="learning"&&cfg.feedbackNoise);
        noise_input(steps,readout,noiseEnabled);
        for(int s=0;s<steps;++s) {
            sim->RunSimulationStep(1);
            if(now()!=begin+s+1)throw std::runtime_error("Neural clock mismatch");
            if(cfg.diagnostics && !cfg.evaluation && (s<3 || s==steps-1)) {
                for(int n=2;n<8;++n) {
                    auto& neuron=sim->network->neurons[n];
                    float current=neuron.neuron_state_vector->GetNeuronState(neuron.index_in_NeuronModel)[5];
                    inputBoundary<<cycle<<','<<name<<','<<s<<','<<now()<<','<<driven<<','<<noiseEnabled<<','<<readout<<','<<n<<','<<current<<'\n';
                    if(s>=2 && n<4 && current!=(driven==n-2?cfg.i_rs:0.0f))throw std::runtime_error("RS cue current did not settle");
                    if(s>=2 && n>=4 && (!noiseEnabled || (n-4)/2!=readout) && current!=0.0f)throw std::runtime_error("Noise current did not clear");
                }
            }
            int t,n;
            while(sim->output_spike_driver->RemoveBufferedSpike(t,n)) {
                if(n>=0&&n<16) {spikes.count[n]++;if(spikes.first[n]<0)spikes.first[n]=t;}
            }
            // Evaluation never invokes the weight integrator.
            if(!cfg.evaluation)rule->Step(sim.get(),d);
        }
        int action=name=="decision"?spikes.decode(readout):precedingAction;
        if(cfg.diagnostics && !cfg.evaluation)rule->EndPhase(mechanism,observed,action,d);
        if(!cfg.evaluation) {
            phases<<cycle<<','<<name<<','<<observed<<','<<driven<<','<<readout<<','<<position<<','<<action<<','<<d<<','<<begin<<','<<now()<<','<<steps<<'\n';
            for(int n=2;n<12;++n)counts<<cycle<<','<<name<<','<<n<<','<<spikes.count[n]<<','<<spikes.first[n]<<'\n';
        }
        return spikes;
    }
    Outcome run(bool terminalArrival=false) {
        VirtualEnvironment env(starting_position(cfg),cfg.target,cfg.moveStep,cfg.tolerance,cfg.limit);
        Outcome result;result.initial=env.position;
        int lastCue=env.cue()==CENTER?LEFT:env.cue(),previousAction=NO_DECISION;
        phase(-1,"warmup",env.cue(),-1,lastCue,env.position,100,0.0f,NO_DECISION);
        bool pending=false;
        auto learn=[&](int cycle) {
            int observed=env.cue();
            if(observed!=CENTER)throw std::runtime_error("Feedback without observed arrival");
            int driven=cfg.center=="previous"?lastCue:-1;
            float d=cfg.mode=="reward"?.00025f*cfg.w_max:cfg.mode=="punishment"?-.00025f*cfg.w_max:0.0f;
            // Control replays the same arrival-dependent phase schedule with d=0.
            phase(cycle,"learning",observed,driven,lastCue,env.position,cfg.feedbackSteps,d,previousAction);
            result.learningBlocks++;pending=false;
        };
        for(int cycle=0;cycle<cfg.cycles;++cycle) {
            int observed=env.cue();if(observed!=CENTER)lastCue=observed;
            int fs=-1,fe=-1;bool hadPending=pending;
            if(pending && !cfg.evaluation) {
                if(cfg.mode!="control" || cfg.shadowControl) {fs=now();learn(cycle);fe=now();}
                else pending=false;
            }
            int driven=observed==CENTER?(cfg.center=="previous"?lastCue:-1):observed;
            int ds=now();auto sp=phase(cycle,"decision",observed,driven,lastCue,env.position,cfg.decisionSteps,0.0f,NO_DECISION);int de=now();
            int raw=sp.decode(lastCue),action=(observed==CENTER&&cfg.centerHold)?NO_DECISION:raw;
            if(cfg.motionSteps>0)phase(cycle,"motion",observed,driven,lastCue,env.position,cfg.motionSteps,0.0f,action);
            auto movement=env.move(action);pending=movement.entered;previousAction=action;
            result.cycles++;result.arrivals+=movement.entered;result.none+=action==NO_DECISION;
            result.boundaries+=movement.boundary;result.moves+=movement.after!=movement.before;
            result.sumAbsDistance+=std::fabs(env.position-cfg.target);
            if(observed!=CENTER) {result.exposure[observed]++;result.toward[observed]+=movement.toward;}
            auto row=[&](std::ostream& out) {
                out<<trialId<<','<<cycle<<','<<movement.before<<','<<observed<<','<<driven<<','<<lastCue<<','<<hadPending<<','<<fs<<','<<fe<<','
                    <<ds<<','<<de<<','<<now()<<','<<sp.count[4+2*lastCue]<<','<<sp.count[5+2*lastCue]<<','<<sp.first[4+2*lastCue]<<','<<sp.first[5+2*lastCue]<<','
                    <<raw<<','<<action<<','<<movement.requested<<','<<movement.after<<','<<movement.toward<<','<<movement.entered<<','<<movement.boundary<<'\n';
            };
            if(!cfg.evaluation)row(trajectory);
            if(externalTrajectory)row(*externalTrajectory);
            if(terminalArrival&&movement.entered)break;
        }
        // Deliver a final arrival's feedback, even if it occurs on the last action.
        if(pending && !cfg.evaluation && (cfg.mode!="control" || cfg.shadowControl))learn(cfg.cycles);
        result.final=env.position;
        auto final=snapshot(*rule);
        for(size_t i=0;i<final.size();++i)result.maxWeightChange=std::max(result.maxWeightChange,(double)std::fabs(final[i]-initialWeights[i]));
        if((cfg.evaluation || cfg.mode=="control") && final!=initialWeights)throw std::runtime_error("Fixed weights changed");
        if(!cfg.evaluation) {
            save_weights(*rule,cfg.out+"/weights_final.csv");
            std::ofstream means(cfg.out+"/weight_groups.csv");means<<"group,mean_init,mean_final\n"<<std::setprecision(17);
            for(int g=0;g<4;++g) {
                double a=0,b=0;int n=0;
                for(size_t k=0;k<rule->plastic.size();++k)if(rule->plastic[k]->TargetNeuron->Neuron_index==4+g){a+=initialWeights[k];b+=final[k];n++;}
                means<<g<<','<<a/n<<','<<b/n<<'\n';
            }
        }
        return result;
    }
};

static void write_config(const Options& c) {
    std::filesystem::create_directories(c.out);std::ofstream f(c.out+"/config.json");
    f<<"{\n  \"psp_history\": \""<<c.pspHistory<<"\",\n";
    f<<"  \"feedback_topology\": \""<<c.feedbackTopology<<"\",\n  \"feedback_scale\": "<<c.feedbackScale
     <<",\n  \"feedback_jitter\": "<<c.feedbackJitter<<",\n";
    f<<std::setprecision(17)<<"  \"mode\": \""<<c.mode<<"\",\n  \"seed\": "<<c.seed
     <<",\n  \"cycles\": "<<c.cycles<<",\n  \"pre_clock\": \""<<(c.sourceDelayReference?"source-delay":c.emission?"emission":"arrival")
     <<"\",\n  \"pairing\": \"recent past post; paper unspecified\",\n  \"pair_sign\": \"post-minus-pre (2020)\",\n  \"center_cue\": \""<<c.center
     <<"\",\n  \"center_action\": \""<<(c.centerHold?"hold":"compete")<<"\",\n  \"feedback_noise\": \""<<(c.feedbackNoise?"continue":"off")
     <<"\",\n  \"decision_ms\": "<<c.decisionSteps*DT<<",\n  \"feedback_ms\": "<<c.feedbackSteps*DT
     <<",\n  \"noise_sampling\": \""<<(c.phaseNoise?"per-phase":"per-step")<<"\""
     <<",\n  \"movement_neural_ms\": "<<c.motionSteps*DT<<",\n  \"dt_ms\": "<<DT<<",\n  \"initial_position\": "<<starting_position(c)
     <<",\n  \"target\": "<<c.target<<",\n  \"move_step\": "<<c.moveStep<<",\n  \"target_tolerance\": "<<c.tolerance<<",\n  \"position_limit\": "<<c.limit
     <<",\n  \"noise_sigma\": "<<c.noise<<",\n  \"input_current\": "<<c.i_rs<<",\n  \"w_max\": "<<c.w_max<<",\n  \"tau_c_ms\": "<<c.tau_c
     <<",\n  \"initial_weight_mean\": "<<c.w_mean<<",\n  \"jitter\": "<<c.jitter<<",\n  \"feedback_trigger\": \"outside-to-inside observed target-band entry\",\n"
     <<"  \"clock_assumption\": \"RPC-local neural time; motor movement does not advance neural time by default\",\n"
     <<"  \"control_assumption\": \""<<(c.shadowControl?"arrival-dependent shadow learning phase with d=0":"no reward/punishment and no extra learning simulation")<<"\",\n"
     <<"  \"evaluation\": "<<(c.evaluation?"true":"false")<<",\n  \"eval_seed\": "<<c.evalSeed<<",\n  \"eval_horizon\": "<<c.evalCycles<<"\n}\n";
}

static int evaluate(Options cfg) {
    std::ofstream csv(cfg.out+"/closed_evaluation.csv"),trace(cfg.out+"/evaluation_trajectories.csv");
    csv<<"trial,start_side,initial_position,final_position,cycles,arrivals,toward_fraction,no_decision_fraction,boundary_fraction,mean_abs_distance,approach_success,avoid_success,max_weight_change\n"<<std::setprecision(17);
    trace<<std::setprecision(17);TrajectoryRunner::writeTrajectoryHeader(trace);
    std::vector<int> sides(cfg.evalTrials);for(int i=0;i<cfg.evalTrials;++i)sides[i]=i%2;
    std::mt19937 order(cfg.evalSeed);std::shuffle(sides.begin(),sides.end(),order);
    int approach=0,avoid=0;double maxChange=0;
    for(int tr=0;tr<cfg.evalTrials;++tr) {
        Options trial=cfg;trial.cycles=cfg.evalCycles;trial.initial=sides[tr]==LEFT?4.0:-4.0;trial.diagnostics=false;
        TrajectoryRunner run(trial,cfg.evalSeed*7u+(unsigned)tr+1u,&trace,tr);auto r=run.run(true);
        int exposures=r.exposure[0]+r.exposure[1],towards=r.toward[0]+r.toward[1];
        bool a=r.arrivals>0;
        bool p=r.arrivals==0 && std::fabs(r.final-cfg.target)>=std::fabs(r.initial-cfg.target)+2*cfg.moveStep && r.moves>0;
        approach+=a;avoid+=p;maxChange=std::max(maxChange,r.maxWeightChange);
        csv<<tr<<','<<sides[tr]<<','<<r.initial<<','<<r.final<<','<<r.cycles<<','<<r.arrivals<<','
           <<(exposures?(double)towards/exposures:0)<<','<<(double)r.none/r.cycles<<','<<(double)r.boundaries/r.cycles<<','
           <<r.sumAbsDistance/r.cycles<<','<<a<<','<<p<<','<<r.maxWeightChange<<'\n';
    }
    std::cout<<"EVAL approach="<<(double)approach/cfg.evalTrials<<" avoid="<<(double)avoid/cfg.evalTrials<<" max|dw|="<<maxChange<<'\n';
    return 0;
}

int main(int argc,char** argv) {
    try {
        Options cfg;
        for(int i=1;i<argc;++i) {
            std::string a=argv[i];auto value=[&](){if(i+1>=argc)throw std::runtime_error("Missing CLI value");return std::string(argv[++i]);};
            if(a=="--mode")cfg.mode=value();else if(a=="--seed")cfg.seed=(unsigned)std::stoul(value());
            else if(a=="--cycles")cfg.cycles=std::stoi(value());else if(a=="--out")cfg.out=value();
            else if(a=="--pre-clock") {auto v=value();if(v!="arrival"&&v!="emission"&&v!="source-delay")throw std::runtime_error("Bad pre clock");cfg.emission=v!="arrival";cfg.sourceDelayReference=v=="source-delay";}
            else if(a=="--center-cue")cfg.center=value();
            else if(a=="--center-action") {auto v=value();if(v!="compete"&&v!="hold")throw std::runtime_error("Bad center action");cfg.centerHold=v=="hold";}
            else if(a=="--feedback-noise") {auto v=value();if(v!="off"&&v!="continue")throw std::runtime_error("Bad feedback noise");cfg.feedbackNoise=v=="continue";}
            else if(a=="--noise-sampling") {auto v=value();if(v!="per-step"&&v!="per-phase")throw std::runtime_error("Bad noise sampling");cfg.phaseNoise=v=="per-phase";}
            else if(a=="--control-phase") {auto v=value();if(v!="shadow"&&v!="none")throw std::runtime_error("Bad control phase");cfg.shadowControl=v=="shadow";}
            else if(a=="--decision-ms")cfg.decisionSteps=(int)std::lround(std::stod(value())/DT);
            else if(a=="--movement-neural-ms")cfg.motionSteps=(int)std::lround(std::stod(value())/DT);
            else if(a=="--start-position")cfg.initial=std::stod(value());else if(a=="--move-step")cfg.moveStep=std::stod(value());
            else if(a=="--i-rs")cfg.i_rs=std::stof(value());
            else if(a=="--psp-history")cfg.pspHistory=value();
            else if(a=="--feedback-topology")cfg.feedbackTopology=value();
            else if(a=="--feedback-scale")cfg.feedbackScale=std::stof(value());
            else if(a=="--target-tolerance")cfg.tolerance=std::stod(value());else if(a=="--position-limit")cfg.limit=std::stod(value());
            else if(a=="--diagnostics")cfg.diagnostics=true;
            else if(a=="--eval") {cfg.evaluation=true;cfg.weights=value();}
            else if(a=="--eval-seed")cfg.evalSeed=(unsigned)std::stoul(value());
            else if(a=="--eval-trials")cfg.evalTrials=std::stoi(value());else if(a=="--eval-cycles")cfg.evalCycles=std::stoi(value());
            else throw std::runtime_error("Unknown argument: "+a);
        }
        if((cfg.mode!="control"&&cfg.mode!="reward"&&cfg.mode!="punishment")||(cfg.center!="previous"&&cfg.center!="silent")||
            cfg.cycles<=0||cfg.decisionSteps<=0||cfg.motionSteps<0||cfg.evalCycles<=0||cfg.evalTrials<=0||cfg.evalTrials%2||!std::isfinite(cfg.i_rs))
            throw std::runtime_error("Invalid configuration");
        VirtualEnvironment validate(starting_position(cfg),cfg.target,cfg.moveStep,cfg.tolerance,cfg.limit);
        write_config(cfg);
        if(cfg.evaluation)return evaluate(cfg);
        TrajectoryRunner run(cfg,cfg.seed);auto r=run.run();
        std::ofstream summary(cfg.out+"/summary.csv");
        summary<<"seed,mode,cycles,initial_position,final_position,arrivals,learning_blocks,left_exposure,right_exposure,left_toward,right_toward,no_decisions,boundaries,max_weight_change\n";
        summary<<std::setprecision(17)<<cfg.seed<<','<<cfg.mode<<','<<r.cycles<<','<<r.initial<<','<<r.final<<','<<r.arrivals<<','<<r.learningBlocks<<','
            <<r.exposure[0]<<','<<r.exposure[1]<<','<<r.toward[0]<<','<<r.toward[1]<<','<<r.none<<','<<r.boundaries<<','<<r.maxWeightChange<<'\n';
        std::cout<<"TRAIN seed="<<cfg.seed<<" mode="<<cfg.mode<<" arrivals="<<r.arrivals<<" learning_blocks="<<r.learningBlocks
                 <<" final_position="<<r.final<<" exposure_L/R="<<r.exposure[0]<<'/'<<r.exposure[1]<<" max|dw|="<<r.maxWeightChange<<'\n';
        return 0;
    }catch(const std::exception& e) {std::cerr<<"ERROR: "<<e.what()<<'\n';return 2;}
}
