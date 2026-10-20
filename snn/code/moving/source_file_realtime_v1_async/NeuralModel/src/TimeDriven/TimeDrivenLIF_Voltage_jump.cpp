#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenLIF_Voltage_jump.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenModel.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/InternalSpike.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/TimeDrivenInternalSpike.h"
#include "../source_file_realtime_v1_async/Current/inc/CurrentSynapse.h"
#include <iostream>
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/ModelFactory/IntegrationMethodFactory.h"

TimeDrivenLIF_Voltage_jump::TimeDrivenLIF_Voltage_jump() :TimeDrivenModel(), CurrentSynapeModel(0) {
	//初始化NeuronStateVector
	this->StateVector = new Neuron_State_Vector(this->N_NeuronStateVariables, true);
	std::string name = std::string("TimeDrivenLIF_Voltage_jump");
	this->setModelName(name);
	//this->TimeDriven = true;
}

TimeDrivenLIF_Voltage_jump::TimeDrivenLIF_Voltage_jump(int timesteps) :TimeDrivenModel(timesteps), CurrentSynapeModel(0) {
    //初始化NeuronStateVector
	this->StateVector = new Neuron_State_Vector(this->N_NeuronStateVariables, true);
	std::string name = std::string("TimeDrivenLIF_Voltage_jump");
	this->setModelName(name);
	//this->TimeDriven = true;
}

TimeDrivenLIF_Voltage_jump::~TimeDrivenLIF_Voltage_jump() {
	if (this->CurrentSynapeModel != 0) {
		delete this->CurrentSynapeModel;
		this->CurrentSynapeModel = 0;
	}
}

void TimeDrivenLIF_Voltage_jump::InitStateVector(int NumberOfNeurons, int GPUIndex) {
	//创建初始化状态向量
	float new_init[] = { this->V_reset + this->init[0], this->init[1] };
	float new_sigma[] = { this->sigma[0], this->sigma[1] };

	this->StateVector->InitNeuronState(NumberOfNeurons, new_init, new_sigma);
	this->CurrentSynapeModel = new CurrentSynapse(NumberOfNeurons);

}

Neuron_State_Vector* TimeDrivenLIF_Voltage_jump::InitState() {
	return this->StateVector;
}

void TimeDrivenLIF_Voltage_jump::CaculateDifferentialEquation(float* NeuronState, float* AuxNeuronState, int index) {
	//初始化增量
	float current_dV = 0.0;
	//计算外加电流的增量
	if (this->I_EXT) {
		current_dV += this->R * NeuronState[this->I_EXT_index] / this->tau;
	}
	//判断是否处于不应期内,如果不在则计算膜电位增量
	if (this->StateVector->LastSpike[index] > this->t_ref) {
		AuxNeuronState[index_V] = current_dV + (this->V_rest - NeuronState[index_V]) / this->tau;
	}
	else {
		AuxNeuronState[index_V] = 0;
	}
}

void TimeDrivenLIF_Voltage_jump::CaculateTimeDependentEquation(float* NeuronState, int index, float dt) {
	//重置外界电流输入
	//if (this->I_EXT) {
        //NeuronState[this->I_EXT_index] = 0;
	//}

}


void TimeDrivenLIF_Voltage_jump::CaculateSpike(float previous_V, float* NeuronState, int index) {
	//判断是否发放了脉冲：前一刻电压低于阈值，当前时刻电压高于阈值
	if (NeuronState[this->index_V] > this->V_th) {
		//重置膜电位
		NeuronState[this->index_V] = this->V_reset;
		this->StateVector->SpikeIndex[this->StateVector->NumberofSpike] = index;
		//更新发放脉冲的计数器
		this->StateVector->NumberofSpike += 1;
        //更新上次发放脉冲的时间步数
		this->StateVector->LastSpike[index] = 0;

	}
}


void TimeDrivenLIF_Voltage_jump:: UpdateState(int index, int time, Simulation* simulation) {
	//首先清空放电计数器
	this->StateVector->NumberofSpike = 0;
	//计算微分方程的增量即:dV/dt
	//调用积分函数进行数值积分
	this->integrationMethod->CaculateIncreament(simulation, time);
	//检查积分的有效性
	this->CheckValidIntegeration(time, this->integrationMethod->GetValidIntegrationVariable());

}

InternalSpike* TimeDrivenLIF_Voltage_jump::ProcessSpike(Interconnections* inter, int time) {
	//閻犱礁澧介悿鍡涙偨闂堟稑绔鹃悹鍝勫暱瑜板鏅堕悙鍝勬
	//判断是兴奋性还是抑制性
	if (this->StateVector->LastSpike[inter->TargetNeuronModelIndex] > this->t_ref) {
		if (inter->type == 0 && this->Excited) {
			this->StateVector->SetNeuronStateIncrement(inter->TargetNeuronModelIndex, this->index_V, inter->weight);
		}
		else if (inter->type == 1 && this->Inhibitory) {
			this->StateVector->SetNeuronStateIncrement(inter->TargetNeuronModelIndex, this->index_V, -inter->weight);
		}
	}
	return 0;

}



void TimeDrivenLIF_Voltage_jump::ProcessCurrent(Interconnections* inter, Neuron* Target, float current) {
	//设置Target传入突触的电流大小
	this->CurrentSynapeModel->SetInputCurrentPerSynapse(Target->index_in_NeuronModel, inter->subindex_type, current);
	//计算总电流
	float total_current = this->CurrentSynapeModel->GetTotalInputCurrentPerNeuron(Target->index_in_NeuronModel);
	//设置NeuronState
	this->StateVector->Vector_of_StateVariable[this->N_NeuronStateVariables * Target->index_in_NeuronModel + this->I_EXT_index] = total_current;

}

void TimeDrivenLIF_Voltage_jump::InitializeInputCurrentSynapseStructure() {
	if (this->CurrentSynapeModel != 0) {
		this->CurrentSynapeModel->InitializeInputCurrentPerSynapseStructure();
	}
}

void TimeDrivenLIF_Voltage_jump::CheckType(Interconnections* inter) {
	int Type = inter->type;
	if (Type == 0) {
		this->Excited = true;
	}
	else if (Type == 1) {
		this->Inhibitory = true;
	}
	else if (Type == 3) {
		this->I_EXT = true;
		//设置外界电流输入的索引
		inter->subindex_type = this->CurrentSynapeModel->N_connections[inter->TargetNeuron->index_in_NeuronModel];
		//更新每个神经元接收外电流的计数器
		this->CurrentSynapeModel->IncrementNInputCurrentSynapsesPerNeuron(inter->TargetNeuron->index_in_NeuronModel);
	}
	else if (Type == 2) {
		std::cout << "Error: NMDA is not supported in this model" << std::endl;
	}
	else {
        std::cout << "Error: Type of synapse is not supported in this model" << std::endl;
	}

}

int TimeDrivenLIF_Voltage_jump::getV_index(){
	return this->index_V;
}

int TimeDrivenLIF_Voltage_jump::get_NumberOfState() {
	return this->N_NeuronStateVariables;
}

enum NeuronModelType TimeDrivenLIF_Voltage_jump::getNeuronModelType() {
	return NEURAL_LAYER;
}

void TimeDrivenLIF_Voltage_jump::SetParameters(std::map<std::string, boost::any> parametermap, float basetimesteps) {
	//在字典中查找参数
	//查找静息电位
	std::map<std::string, boost::any>::iterator iter = parametermap.find("V_rest");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->V_rest = new_parameter;
		parametermap.erase(iter);
	}
	//闁哄被鍎叉竟姒痑u
	iter = parametermap.find("tau");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->tau = new_parameter;
		parametermap.erase(iter);
	}
	//查找阈值
	iter = parametermap.find("V_th");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->V_th = new_parameter;
		parametermap.erase(iter);
	}
	// 返回一个参数字典
	iter = parametermap.find("R");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->R = new_parameter;
		parametermap.erase(iter);
	}
	//查找重置电位
	iter = parametermap.find("V_reset");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->V_reset = new_parameter;
		parametermap.erase(iter);
	}
	//查找不应期
	iter = parametermap.find("t_ref");
	if (iter != parametermap.end()) {
		int new_parameter = boost::any_cast<int>(iter->second);
		this->t_ref = new_parameter;
		parametermap.erase(iter);
	}
	//查找随机性参数
	iter = parametermap.find("random_mu");
	if (iter != parametermap.end()) {
		std::array<float, 2> new_parameter = boost::any_cast<std::array<float, 2>>(iter->second);
		this->init = new_parameter;
		parametermap.erase(iter);
	}

	iter = parametermap.find("random_sigma");
	if (iter != parametermap.end()) {
		std::array<float, 2> new_parameter = boost::any_cast<std::array<float, 2>>(iter->second);
		this->sigma = new_parameter;
		parametermap.erase(iter);
	}
	//查找积分方法

	iter = parametermap.find("int_method");
	if (iter != parametermap.end()) {
		if (this->integrationMethod != 0) {
			delete this->integrationMethod;
		}
		ModelDescription temp = boost::any_cast<ModelDescription>(iter->second);
		temp.ModelParameter["step"] = basetimesteps;
		//为神经元模型创建积分方法
		this->integrationMethod = IntegrationMethodFactory<TimeDrivenLIF_Voltage_jump>::createIntegerationMethod(temp, this);
		parametermap.erase(iter);

	}
	else {
		//创建一个默认的前向欧拉积分方法
		ModelDescription temp_modelDescription;
		temp_modelDescription.ModelName = "ForwardEulerMethod";
		temp_modelDescription.ModelParameter["step"] = basetimesteps;
		this->integrationMethod = IntegrationMethodFactory<TimeDrivenLIF_Voltage_jump>::createIntegerationMethod(temp_modelDescription, this);
	}
	
}

std::map<std::string, boost::any> TimeDrivenLIF_Voltage_jump::getParameters() {
	std::map<std::string, boost::any> parametermap;
	parametermap["V_rest"] = this->V_rest;
	parametermap["tau"] = this->tau;
	parametermap["V_th"] = this->V_th;
	parametermap["R"] = this->R;
	parametermap["V_reset"] = this->V_reset;
	parametermap["t_ref"] = this->t_ref;
	parametermap["random_mu"] = this->init;
	parametermap["random_sigma"] = this->sigma;
	ModelDescription temp;
	temp.ModelParameter = this->integrationMethod->getParameters();
	temp.ModelName = boost::any_cast<std::string>(temp.ModelParameter["name"]);
	parametermap["int_method"] = temp;
	return parametermap;
}
