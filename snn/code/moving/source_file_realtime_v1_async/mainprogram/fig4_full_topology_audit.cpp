#include "inc/Fig4ClosedLoopNetwork.h"
#include <filesystem>
#include <iostream>
#include <set>
int main(int argc,char** argv) {try {
    std::string out=argc>1?argv[1]:"topology_audit";std::filesystem::create_directories(out);
    for(std::string topology:{"single","collapsed","bank"}) {
        fig4::NetworkSettings cfg;cfg.feedbackTopology=topology;cfg.pspHistory="sum-arrival";
        auto sim=fig4::build_network(cfg);sim->InitSimulation();auto* rule=fig4::get_rule(*sim);fig4::validate_feedback(*sim,cfg);
        std::set<int> plastic;for(auto* c:rule->plastic)plastic.insert(c->Index);
        std::ofstream file(out+"/"+topology+"_connections.csv");
        file<<"conn_id,source,target,type,delay_steps,weight,max_weight,plastic\n"<<std::setprecision(17);
        for(int i=0;i<sim->network->intersNum;++i) {
            auto& c=sim->network->inters[i];
            file<<c.Index<<','<<c.SourceNeuron->Neuron_index<<','<<c.TargetNeuron->Neuron_index<<','<<c.type<<','
                <<c.delay<<','<<c.weight<<','<<c.maximum_weight<<','<<plastic.count(c.Index)<<'\n';
        }
        fig4::save_weights(*rule,out+"/"+topology+"_plastic_initial.csv");
        std::cout<<"TOPOLOGY PASS "<<topology<<" connections="<<sim->network->intersNum<<" plastic="<<rule->plastic.size()<<'\n';
    }
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
