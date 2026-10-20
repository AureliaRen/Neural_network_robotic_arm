// Fit input frequency to an independently digitized paper raster, not outcomes.
#include "inc/Fig4ClosedLoopNetwork.h"
#include <iostream>
#include <filesystem>

using namespace fig4;
struct Rate {double medianIsi=0,rate=0;int spikes=0;};
static Rate measure(float current,std::ostream* out=nullptr,int iteration=-1) {
    NetworkSettings cfg;cfg.i_rs=current;cfg.noise=0;
    auto sim=build_network(cfg);sim->InitSimulation();
    auto* rule=get_rule(*sim);auto before=snapshot(*rule);
    sim->AddExternalCurrentActivity({0,0},{0,1},{current,0.0f});
    std::vector<int> times;
    for(int s=0;s<60000;++s) {
        sim->RunSimulationStep(1);int t,n;
        while(sim->output_spike_driver->RemoveBufferedSpike(t,n))if(n==2&&t>=5000) {
            times.push_back(t);if(out)*out<<iteration<<','<<current<<','<<t<<'\n';
        }
    }
    if(snapshot(*rule)!=before)throw std::runtime_error("Rate calibration changed weights");
    Rate result;result.spikes=(int)times.size();
    if(times.size()>1) {
        std::vector<int> intervals;
        for(size_t i=1;i<times.size();++i)intervals.push_back(times[i]-times[i-1]);
        std::sort(intervals.begin(),intervals.end());
        result.medianIsi=intervals[intervals.size()/2]*DT;
        result.rate=1000/result.medianIsi;
    }
    return result;
}
int main(int argc,char** argv) {
    try {
        if(argc!=3)throw std::runtime_error("Usage: fig4_rate_probe target_hz output_directory");
        double target=std::stod(argv[1]);std::filesystem::create_directories(argv[2]);
        std::string out=argv[2];std::ofstream csv(out+"/calibration.csv");
        std::ofstream spikes(out+"/calibration_spikes.csv");spikes<<"iteration,current,spike_step\n"<<std::setprecision(17);
        csv<<"iteration,current,median_isi_ms,rate_hz,steady_spikes\n"<<std::setprecision(17);
        float best=22.5f,low=4.0f,high=22.5f;double error=1e9;
        for(int i=-1;i<18;++i) {
            float c=i==-1?22.5f:(low+high)*.5f;auto r=measure(c,&spikes,i);
            csv<<i<<','<<c<<','<<r.medianIsi<<','<<r.rate<<','<<r.spikes<<'\n';
            if(std::fabs(r.rate-target)<error){error=std::fabs(r.rate-target);best=c;}
            if(i>=0){if(r.rate<target)low=c;else high=c;}
        }
        auto result=measure(best);
        std::ofstream json(out+"/fitted_current.json");json<<std::setprecision(17);
        json<<"{\"target_hz\":"<<target<<",\"fitted_current\":"<<best
            <<",\"achieved_hz\":"<<result.rate<<",\"median_isi_ms\":"<<result.medianIsi
            <<",\"fit_assumption\":\"constant current to match raster-derived input rate; not a paper-specified current\"}\n";
        std::cout<<"FIT current="<<best<<" achieved_hz="<<result.rate<<" target_hz="<<target<<'\n';
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
