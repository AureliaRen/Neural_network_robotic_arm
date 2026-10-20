#include "../source_file_realtime_v1_async/NeuralModel/inc/EventDriven/InputSpikeNeuronModel.h"


InputSpikeNeuronModel::InputSpikeNeuronModel() :EventDrivenInputDevice() {
	std::string name = "InputSpikeNeuronModel";
	this->setModelName(name);
	this->TimeDriven = false;
};

InputSpikeNeuronModel::InputSpikeNeuronModel(int timesteps): EventDrivenInputDevice(timesteps) {
	std::string name = "InputSpikeNeuronModel";
	this->setModelName(name);
	this->TimeDriven = false;
};

InputSpikeNeuronModel::~InputSpikeNeuronModel() {}