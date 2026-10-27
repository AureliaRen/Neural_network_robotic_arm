#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"


Interconnections::Interconnections():
	SourceNeuron(0), TargetNeuron(0), TargetNeuronModel(0), weight(0), delay(0), type(0), TargetNeuronModelIndex(0), Index(0), LearningRule_withPost(0), LearningRule_withTrigger(0), LearningRule_withPostAndTrigger(0), Synmodel_Index(-1), synmodel{} {}


Interconnections::Interconnections(Neuron* source, Neuron* target, NeuronModel* targetModel, int delay, float weight, int type, int index):
	SourceNeuron(source), TargetNeuron(target), TargetNeuronModel(targetModel), weight(weight), delay(delay), type(type), Index(index), LearningRule_withPost(0), LearningRule_withTrigger(0), LearningRule_withPostAndTrigger(0), Synmodel_Index(-1), synmodel{} {}


Interconnections::~Interconnections() {
	
}

void Interconnections::SetIndex(int index) {
	this->Index = index;
}

void Interconnections::SetTargetNeuronModelIndex(int index) {
	this->TargetNeuronModelIndex = index;
}

void Interconnections::SetSourceNeuron(Neuron* source) {
	this->SourceNeuron = source;
}

void Interconnections::SetTargetNeuron(Neuron* target) {
	this->TargetNeuron = target;
}

void Interconnections::SetTargetNeuronModel(NeuronModel* targetModel) {
	this->TargetNeuronModel = targetModel;
}

void Interconnections::SetWeight(float weight) {
	this->weight = weight;
}

void Interconnections::SetDelay(int delay) {
	this->delay = delay;
}

void Interconnections::SetType(int type) {
	this->type = type;
}