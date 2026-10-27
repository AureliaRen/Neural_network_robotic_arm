#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModelPropogationStructure.h"

NeuronModel::NeuronModel() :
	StateVector(0), timestep_size(1) {
	this->PropogationStructure = new NeuronModelPropogationStructure();

}

NeuronModel::NeuronModel(int timestep_size) :
	StateVector(0), timestep_size(timestep_size) {
    this->PropogationStructure = new NeuronModelPropogationStructure();
}

NeuronModel::~NeuronModel() {
	if (this->StateVector != 0) {
		delete this->StateVector;
		this->StateVector = NULL;
	}
	if (this->PropogationStructure != 0) {
		delete this->PropogationStructure;
		this->PropogationStructure = NULL;
	}
}

	int NeuronModel::getTimestepSize() {
		return this->timestep_size;
	}

void NeuronModel::setTimestepSize(int timestep_size) {
	this->timestep_size = timestep_size;
}

void NeuronModel::setModelName(std::string name) {
	this->model_name = name;
}

std::string NeuronModel::getModelName() {
	return this->model_name;
}