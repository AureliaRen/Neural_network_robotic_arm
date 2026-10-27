#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/WithTriggerAndPostSynaptic.h"

WithTriggerAndPostSynaptic::WithTriggerAndPostSynaptic() :LearningRule() {

}

WithTriggerAndPostSynaptic::~WithTriggerAndPostSynaptic() {}

bool WithTriggerAndPostSynaptic::ImplementPostSynaptic() {
	return true;
}

bool WithTriggerAndPostSynaptic::ImplementTriggerSynaptic() {
	return true;
}