#ifndef PAPER_PSP_HISTORY_H
#define PAPER_PSP_HISTORY_H
#include "../source_file_realtime_v1_async/SynModel/inc/Synmodel.h"
#include <vector>
class Interconnections;
// Interpretation alternatives. Baseline PaperPSP remains byte-identical.
class PaperPSPHistory : public Synmodel {
 struct Entry {Interconnections* c;double g=0,h=0;int step=0;int observedSource=-1;};
 std::vector<std::vector<Entry>> entries_;
 std::vector<float> lastCurrent_;
 float tau_=5.0f,dt_=.1f;
 int history_=0; // 0 sum arrivals, 1 latest arrival, 2 latest source
 bool currentWeight_=false;
 int currentStep_=0;
 void advance(Entry& e,int step);
public:
 explicit PaperPSPHistory(int step):Synmodel(step) {setModelName("PaperPSPHistory");}
 void BeginStep(int step) {currentStep_=step;}
 void OnArrival(Interconnections* c,int step,float weight);
 void ProcessInput(Interconnections*,float*,float) override;
 void UpdateSynState(float) override {}
 float GetCurrent(float*,int index) override;
 void InitSynState(int n) override {entries_.assign(n,{});lastCurrent_.assign(n,0);currentStep_=0;}
 float LastCurrent(int index) const {return lastCurrent_.at(index);}
 std::vector<std::pair<Interconnections*,int>> SourceObservations(int index) const {
  std::vector<std::pair<Interconnections*,int>> v;
  for(const auto& e:entries_.at(index))v.push_back({e.c,e.observedSource});return v;
 }
 void setParameters(std::map<std::string,boost::any>) override;
 std::map<std::string,boost::any> getParameters() override;
 bool compare(Synmodel*) override;
};
#endif
