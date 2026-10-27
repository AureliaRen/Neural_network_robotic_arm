#include "../source_file_realtime_v1_async/LearningRule/inc/LearningRule.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"

LearningRule::LearningRule():State(0), LearningRuleID(-1), StateCounter(0){}

LearningRule::~LearningRule(){
	if (this->State != 0) {
		delete this->State;
	}
}