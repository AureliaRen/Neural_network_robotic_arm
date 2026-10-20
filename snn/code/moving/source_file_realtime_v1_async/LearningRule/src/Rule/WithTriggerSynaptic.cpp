#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/WithTriggerSynaptic.h"

WithTriggerSynaptic::WithTriggerSynaptic():LearningRule() {

}

WithTriggerSynaptic::~WithTriggerSynaptic() {}

bool WithTriggerSynaptic::ImplementPostSynaptic() {
	return false;
}

bool WithTriggerSynaptic::ImplementTriggerSynaptic(){
	return true;
}