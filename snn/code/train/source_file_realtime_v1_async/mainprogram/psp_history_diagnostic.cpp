// Mechanistic diagnostic only: no neurons, decisions, reinforcement or fitting.
#include "../source_file_realtime_v1_async/SynModel/inc/int/PaperPSP.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

static double kernel(double x) {return x>0?x/5.0*std::exp(1-x/5.0):0.0;}
int main(int argc,char**argv) {
 try {
  std::string out=argc>1?argv[1]:"psp_history_diagnostic";
  std::filesystem::create_directories(out);
  std::ofstream summary(out+"/summary.csv"),trace(out+"/currents.csv");
  summary<<"case,delay_ms,n_syn,mean_numeric_sum,mean_exact_sum,mean_latest_arrival,mean_latest_source,max_numeric_error,max_history_difference\n"<<std::setprecision(17);
  trace<<"case,delay_ms,step,numeric_sum,exact_sum,latest_arrival,latest_source\n"<<std::setprecision(17);
  int checks=0;
  for(std::string kind:{"single","pair20","period20.7","period50"}) {
   std::vector<int> spikes{1000};
   if(kind=="pair20")spikes.push_back(1200);
   if(kind.rfind("period",0)==0) {
    int p=kind=="period50"?500:207;
    for(int s=1000+p;s<=20000;s+=p)spikes.push_back(s);
   }
   for(int delay:{10,500,1000,-1}) {
    std::vector<int> delays;
    if(delay==-1)for(int n=0;n<100;++n)delays.push_back(10+990*n/99);
    else delays.push_back(delay);
    PaperPSP psp(1);ModelDescription im;im.ModelName="ForwardEulerMethod";im.ModelParameter["step"]=.1f;
    psp.setParameters({{"tau",5.0f},{"int_method",im}});psp.InitSynState(1);
    Interconnections c(nullptr,nullptr,nullptr,1,.5f,4,0);c.TargetNeuronModelIndex=0;
    std::vector<std::vector<int>> arrivals(20001);
    for(int s:spikes)for(int d:delays)if(s+d<=20000)arrivals[s+d].push_back(d);
    int latest=-1;double sums[4]={},err=0,diff=0;
    std::vector<int> lastArrival(delays.size(),-1);std::vector<int> arrived;
    size_t sourceIndex=0;
    for(int t=0;t<=20000;++t) {
     if(sourceIndex<spikes.size()&&spikes[sourceIndex]==t) {latest=t;++sourceIndex;}
     for(int d:arrivals[t]) {
      psp.ProcessInput(&c,nullptr,.5f);arrived.push_back(t);
      size_t index=std::find(delays.begin(),delays.end(),d)-delays.begin();lastArrival[index]=t;
     }
     double values[4]={psp.GetCurrent(nullptr,0),0,0,0};
     for(int a:arrived)if(t-a<1500)values[1]+=.5*kernel((t-a)*.1);
     for(size_t i=0;i<delays.size();++i) {
      if(lastArrival[i]>=0)values[2]+=.5*kernel((t-lastArrival[i])*.1);
      if(latest>=0)values[3]+=.5*kernel((t-latest-delays[i])*.1);
     }
     err=std::max(err,std::fabs(values[0]-values[1]));diff=std::max(diff,std::fabs(values[1]-values[3]));
     if(t>=15000&&t<20000)for(int j=0;j<4;++j)sums[j]+=values[j]/5000;
     if(t%5==0&&delay!=-1)trace<<kind<<','<<delay*.1<<','<<t<<','<<values[0]<<','<<values[1]<<','<<values[2]<<','<<values[3]<<'\n';
     if(kind=="single"&&(std::fabs(values[1]-values[2])>1e-10||std::fabs(values[1]-values[3])>1e-10))throw std::runtime_error("Single-spike histories disagree");
     if(kind.rfind("period",0)==0&&delay>=500&&delay>=(kind=="period50"?500:207)&&t>15000&&values[3]!=0)throw std::runtime_error("Expected suppression of latest-source long-delay kernel");
     psp.UpdateSynState(.1f);
    }
    if(err>.02*delays.size())throw std::runtime_error("Baseline numeric PSP differs from exact summed kernel");
    summary<<kind<<','<<(delay<0?-1:delay*.1)<<','<<delays.size()<<','<<sums[0]<<','<<sums[1]<<','<<sums[2]<<','<<sums[3]<<','<<err<<','<<diff<<'\n';++checks;
   }
  }
  std::cout<<"PSP_HISTORY_DIAGNOSTIC PASS cases="<<checks<<" (all-step causal, single-spike and numeric checks)\n";return 0;
 }catch(const std::exception&e) {std::cerr<<e.what()<<'\n';return 1;}
}
