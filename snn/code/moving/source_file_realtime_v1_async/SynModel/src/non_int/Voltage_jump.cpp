#include "../source_file_realtime_v1_async/SynModel/inc/non_int/Voltage_jump.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"

Voltage_jump::Voltage_jump(int timestep): Non_int(timestep) {
	std::string name = "Voltage_jump";
	this->setModelName(name);
	this->synapsejumpindex = new int[1];
	this->synapsejumpindex[0] = 0;
}

Voltage_jump::~Voltage_jump() {
}

void Voltage_jump::UpdateSynState(float dt) {

}

float Voltage_jump::GetCurrent(float* NeuronState, int index) {
	return 0.0;
}

void Voltage_jump::ProcessInput(Interconnections* inter, float* NeuronState, float weight) {
	//施加放电的影响
	for (int i = 0; i < this->NumberOfJumpVariable; i++) {
		NeuronState[this->synapsejumpindex[i]] += weight;
	}
}


std::map<std::string, boost::any> Voltage_jump::getParameters() {
	std::map<std::string, boost::any> parameters;
	parameters = Non_int::getParameters();
	return parameters;
}

void Voltage_jump::setParameters(std::map<std::string, boost::any> parameters) {
	Non_int::setParameters(parameters);
}

void Voltage_jump::InitSynState(int NumberOfNeurons) {
	this->NumberOfNeuron = NumberOfNeurons;
	return;
}

