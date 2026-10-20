#include "../source_file_realtime_v1_async/SynModel/inc/int/Alpha.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Intergration/inc/IntegerationMethod.h"
#include "../source_file_realtime_v1_async/ModelFactory/IntegrationMethodFactory_Syn.h"
#include <random>

Alpha::Alpha(int timestep) : Int(timestep), g_h(0) {
	std::string name = "Alpha";
	this->setModelName(name);
}

Alpha::~Alpha() {
	if (this->g_h != 0) {
		delete[] this->g_h;
	}
	
}

void Alpha::UpdateSynState(float dt) {
	for (int i = 0; i < this->NumberOfNeuron; i++) {
		//鏇存柊h鐨勮“鍑?
		this->g_h[2 * i + 1] *= exp(-dt / this->h_tau);
	}
	this->integrationMethod->CaculateIncreament();

}

void Alpha::CaculateDifferentialEquation(float* SynSate, float* AuxState, int index) {
	AuxState[0] = -SynSate[0] / this->g_tau + SynSate[1];

}

float* Alpha::GetState(int index) {
	return this->g_h + index * 2;
}


float Alpha::GetCurrent(float* NeuronState, int index) {
	float current = 0.0;
	current = this->g_h[index * 2] * (this->E - NeuronState[0]);
	return current;
}

void Alpha::InitSynState(int NumberOfNeurons) {
	this->g_h = new float[NumberOfNeurons * 2]();
	std::random_device rd;
	std::mt19937 gen(rd());
	std::normal_distribution<float> dis(0.0, 1.0);
	this->NumberOfNeuron = NumberOfNeurons;
	for (int i = 0; i < NumberOfNeurons; i++) {
		this->g_h[2*i] = this->init[0] + this->sigma[0] * dis(gen);
		this->g_h[2 * i + 1] = this->init[1] + this->sigma[1] * dis(gen);
	}
}


void Alpha::ProcessInput(Interconnections* inter, float* NeuronSrtate, float weight) {
	this->g_h[2 * inter->TargetNeuronModelIndex + 1] += weight;
	return;
}

std::map<std::string, boost::any> Alpha::getParameters() {
	std::map<std::string, boost::any> param;
	param = Int::getParameters();
	param["g_tau"] = this->g_tau;
	param["h_tau"] = this->h_tau;
	param["E"] = this->E;
	param["random_mu"] = this->init;
	param["random_sigma"] = this->sigma;
	return param;
}


void Alpha::setParameters(std::map<std::string, boost::any> param) {
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
		std::array<float, 2> new_param = boost::any_cast<std::array<float, 2>>(iter->second);
		this->init = new_param;
		param.erase(iter);
	}
	iter = param.find("random_sigma");
	if (iter != param.end()) {
		std::array<float, 2> new_param = boost::any_cast<std::array<float, 2>>(iter->second);
		this->sigma = new_param;
		param.erase(iter);
	}
	iter = param.find("h_tau");
	if (iter != param.end()) {
		float new_param = boost::any_cast<float>(iter->second);
		this->h_tau = new_param;
		param.erase(iter);
	}
	//鏌ユ壘绉垎鏂规硶

	iter = param.find("int_method");
	if (iter != param.end()) {
		if (this->integrationMethod != 0) {
			delete this->integrationMethod;
		}
		ModelDescription temp = boost::any_cast<ModelDescription>(iter->second);
		//涓虹缁忓厓妯″瀷鍒涘缓绉垎鏂规硶
		this->integrationMethod = IntegrationMethodFactory_Syn<Alpha>::createIntegerationMethod(temp, this);
		param.erase(iter);

	}
	Int::setParameters(param);
}

