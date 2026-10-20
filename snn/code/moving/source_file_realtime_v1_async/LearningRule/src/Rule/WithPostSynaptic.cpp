#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/WithPostSynaptic.h"

WithPostSynaptic::WithPostSynaptic():LearningRule() {

}

WithPostSynaptic::~WithPostSynaptic() {}

bool WithPostSynaptic::ImplementPostSynaptic() {
	return true;
}

bool WithPostSynaptic::ImplementTriggerSynaptic() {
	return false;
}