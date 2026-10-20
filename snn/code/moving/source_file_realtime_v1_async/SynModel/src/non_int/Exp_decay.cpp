#include "../source_file_realtime_v1_async/SynModel/inc/non_int/Exp_decay.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <random>

Exp_decay::Exp_decay(int timestep) : Non_int(timestep), g(0) {
	std::string name = "Exp_decay";
	this->setModelName(name);
}

Exp_decay::~Exp_decay() {
	if (this->g != 0) {
		delete[] this->g;
	}
}

void Exp_decay::UpdateSynState(float dt) {
	for (int i = 0; i < this->NumberOfNeuron; i++) {
		//乘上指数衰减因子
		this->g[i] *= exp(-dt / this->g_tau);
	}
}

float Exp_decay::GetCurrent(float* NeuronState, int index) {
	float current = 0.0;
	current = this->g[index] * (this->E - NeuronState[0]);
	return current;
}

void Exp_decay::InitSynState(int NumberOfNeurons) {
	//初始化电导g
	this->g = new float[NumberOfNeurons]();
	std::random_device rd;
	std::mt19937 gen(rd());
	std::normal_distribution<float> dis(0.0, 1.0);
	this->NumberOfNeuron = NumberOfNeurons;

	for (int i = 0; i < NumberOfNeurons; i++) {
		this->g[i] = this->init[0] + this->sigma[0] * dis(gen);
	}

}


void Exp_decay::ProcessInput(Interconnections* inter, float* NeuronSrtate, float weight) {
	//施加放电的影响
	this->g[inter->TargetNeuronModelIndex] += weight;
	return;
}

std::map<std::string, boost::any> Exp_decay::getParameters() {
	std::map<std::string, boost::any> param;
	param = Non_int::getParameters();
	param["g_tau"] = this->g_tau;
	param["E"] = this->E;
	param["random_mu"] = this->init;
	param["random_sigma"] = this->sigma;
	return param;
}

void Exp_decay::setParameters(std::map<std::string, boost::any> param) {
	std::map<std::string, boost::any>::iterator iter = param.find("g_tau");
	if (iter != param.end()) {
		float new_param = boost::any_cast<float>(iter->second);
		this->g_tau = new_param;
		param.erase(iter);
	}
	iter = param.find("E");
	if (iter != param.end()) {
		float new_param = boost::any_cast<float>(iter->second);
		this->E = new_param;
		param.erase(iter);
	}
	iter = param.find("random_mu");
	if (iter != param.end()) {
		std::array<float, 1> new_param = boost::any_cast<std::array<float, 1>>(iter->second);
		this->init = new_param;
		param.erase(iter);
	}
	iter = param.find("random_sigma");
	if (iter != param.end()) {
		std::array<float, 1> new_param = boost::any_cast<std::array<float, 1>>(iter->second);
		this->sigma = new_param;
		param.erase(iter);
	}
	Non_int::setParameters(param);

}