#include "../source_file_realtime_v1_async/LearningRule/inc/SynapseState.h"
#include <iostream>
SynapseState::SynapseState(int NumberOfConnections, int NumberOfState) :NumberOfConnections(NumberOfConnections), NumberOfState(NumberOfState) {
	this->LastUpdate = new int[NumberOfConnections]();
	this->StateValue = new float[NumberOfConnections * NumberOfState]();
}

SynapseState::~SynapseState() {
	if (this->LastUpdate != NULL) {
		delete[] this->LastUpdate;
	}
	if (this->StateValue != NULL) {
		delete[] this->StateValue;
	}
}