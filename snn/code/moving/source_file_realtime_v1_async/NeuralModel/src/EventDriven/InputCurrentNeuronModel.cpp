#include "../source_file_realtime_v1_async/NeuralModel/inc/EventDriven/InputCurrentNeuronModel.h"

InputCurrentNeuronModel::InputCurrentNeuronModel() :EventDrivenInputDevice() {
	std::string name = "InputCurrentNeuronModel";
	this->setModelName(name);
	this->TimeDriven = false;
};

InputCurrentNeuronModel::InputCurrentNeuronModel(int timesteps) : EventDrivenInputDevice(timesteps) {
	std::string name = "InputCurrentNeuronModel";
	this->setModelName(name);
	this->TimeDriven = false;
};

InputCurrentNeuronModel::~InputCurrentNeuronModel() {}