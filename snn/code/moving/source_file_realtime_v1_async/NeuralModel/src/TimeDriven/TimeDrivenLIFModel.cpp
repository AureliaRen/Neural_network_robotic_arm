#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenLIFModel.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include "../source_file_realtime_v1_async/ModelFactory/IntegrationMethodFactory.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Current/inc/CurrentSynapse.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/SynModel/inc/Synmodel.h"
#include "../source_file_realtime_v1_async/ModelFactory/SynModelFactory.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

TimeDrivenLIFModel::TimeDrivenLIFModel(int timesteps) :TimeDrivenModel(timesteps), CurrentSynapeModel(0) {
	this->StateVector = new Neuron_State_Vector(this->N_NeuronStateVariables, true);
	std::string name = std::string("TimeDrivenLIFModel");
	this->setModelName(name);
}


TimeDrivenLIFModel::~TimeDrivenLIFModel() {
	if (this->CurrentSynapeModel != 0) {
		delete this->CurrentSynapeModel;
		this->CurrentSynapeModel = 0;
	}
}

void TimeDrivenLIFModel::InitStateVector(int NumberOfNeurons, int GPUIndex) {
	float new_init[] = { this->V_rest + this->init[0], this->init[1]};
	float new_sigma[] = { this->sigma[0], this->sigma[1]};
	this->StateVector->InitNeuronState(NumberOfNeurons, new_init, new_sigma);
	this->CurrentSynapeModel = new CurrentSynapse(NumberOfNeurons);
	this->NumberOfNeuron = NumberOfNeurons;
}

Neuron_State_Vector* TimeDrivenLIFModel::InitState() {
	return this->StateVector;
}

void TimeDrivenLIFModel::CaculateDifferentialEquation(float* NeuronState, float* AuxNeuronState, int index) {
	float current_dV = 0.0;
	if (this->I_EXT) {
		current_dV += this->R * NeuronState[this->I_EXT_index] / this->tau;
	}
	for (int i = 0; i < this->SynapseModelVector.size(); i++) {
		current_dV += this->SynapseModelVector[i]->GetCurrent(NeuronState, index) / this->tau;
	}
	if (this->StateVector->LastSpike[index] > this->t_ref) {
		AuxNeuronState[this->index_V] = current_dV + (this->V_rest - NeuronState[index_V]) / this->tau;
	}
	else {
		AuxNeuronState[index_V] = 0.0;
	}
}

void TimeDrivenLIFModel::CaculateTimeDependentEquation(float* NeuronState, int index, float dt) {
	if (this->Excited || this->Inhibitory) {
		for (int i = 0; i < this->SynapseModelVector.size(); i++) {
			this->SynapseModelVector[i]->UpdateSynState(dt);
		}
	}
}

void TimeDrivenLIFModel::CaculateSpike(float previous_V, float* NeuronState, int index) {
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

void TimeDrivenLIFModel::UpdateState(int index, int time, Simulation* simulation) {
	//首先清空放电计数器
	this->StateVector->NumberofSpike = 0;
	//计算微分方程的增量即:dV/dt
	this->integrationMethod->CaculateIncreament(simulation, time);
	//检查积分的有效性
	this->CheckValidIntegeration(time, this->integrationMethod->GetValidIntegrationVariable());
}


InternalSpike* TimeDrivenLIFModel::ProcessSpike(Interconnections* inter, int time) {
	//閺傝棄濮為懘澶婂暱鏉堟挸鍙嗚ぐ鍗炴惙
	if (inter->type == 4 && inter->Synmodel_Index >= 0) {
		this->SynapseModelVector[inter->Synmodel_Index]->ProcessInput(inter, this->StateVector->GetNeuronState(inter->TargetNeuronModelIndex), inter->weight);
	}
	else if (inter->type == 5 && inter->Synmodel_Index >= 0) {
		this->SynapseModelVector[inter->Synmodel_Index]->ProcessInput(inter, this->StateVector->GetNeuronState(inter->TargetNeuronModelIndex), -inter->weight);
	}
	return 0;
}

void TimeDrivenLIFModel::ProcessCurrent(Interconnections* inter, Neuron* Target, float current) {
	//设置Target传入突触的电流大小
	this->CurrentSynapeModel->SetInputCurrentPerSynapse(Target->index_in_NeuronModel, inter->subindex_type, current);
	//计算总电流
	float total_current = this->CurrentSynapeModel->GetTotalInputCurrentPerNeuron(Target->index_in_NeuronModel);
	//设置NeuronState
	this->StateVector->Vector_of_StateVariable[this->N_NeuronStateVariables * Target->index_in_NeuronModel + this->I_EXT_index] = total_current;

}

void TimeDrivenLIFModel::InitializeInputCurrentSynapseStructure() {
	if (this->CurrentSynapeModel != 0) {
		this->CurrentSynapeModel->InitializeInputCurrentPerSynapseStructure();
	}
}

void TimeDrivenLIFModel::CheckType(Interconnections* inter) {
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
	else if (Type == 4 || Type == 5) {
		//将模型加入到向量中
		if (Type == 4) {
			this->Excited = true;
		}
		else if (Type == 5) {
			this->Inhibitory = true;
		}
		if ((!inter->synmodel.ModelName.empty())) {
			std::map<std::string, boost::any> paramap = this->integrationMethod->getParameters();
			ModelDescription temp_modelDescription;
			temp_modelDescription.ModelName = boost::any_cast<std::string>(paramap["name"]);
            temp_modelDescription.ModelParameter = paramap;
			inter->synmodel.ModelParameter["int_method"] = temp_modelDescription;
			float dt = boost::any_cast<float>(paramap["step"]);
			Synmodel* temp = SynModelFactory::createSynModel(inter->synmodel.ModelName, inter->synmodel.ModelParameter, this->getTimestepSize(), dt);
			//检查是否已经存在该模型
			std::vector<Synmodel*>::iterator it;
			bool found = false;
			int type_index = 0;
			for (it = this->SynapseModelVector.begin(); it != this->SynapseModelVector.end() && !found; ++it) {
				found = temp->compare(*it);
				if (!found) {
					type_index++;
				}
			}
			if (!found) {
				temp->InitSynState(this->NumberOfNeuron);//初始化突触状态
				this->SynapseModelVector.push_back(temp);//将新模型加入
				this->NumberOfSyn++;
			}
			inter->Synmodel_Index = type_index;
		}
	}
	else {
		std::cout << "Error: Type of synapse is not supported in this model" << std::endl;
	}

}

int TimeDrivenLIFModel::getV_index() {
	return this->index_V;
}

int TimeDrivenLIFModel::get_NumberOfState() {
	return this->N_NeuronStateVariables;
}

enum NeuronModelType TimeDrivenLIFModel::getNeuronModelType() {
	return NEURAL_LAYER;
}

void TimeDrivenLIFModel::SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep) {
	std::map<std::string, boost::any>::iterator iter = parametermap.find("V_rest");
	if (iter != parametermap.end()) {
		float new_parameter = boost::any_cast<float>(iter->second);
		this->V_rest = new_parameter;
		parametermap.erase(iter);
	}
	//閺屻儲澹榯au
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
	//閺屻儲澹橀悽鐢告▎
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
		temp.ModelParameter["step"] = basetimestep;
		// 根据参数创建对应的积分方法
		this->integrationMethod = IntegrationMethodFactory<TimeDrivenLIFModel>::createIntegerationMethod(temp, this);
		parametermap.erase(iter);

	}
	else {
		//閸掓稑缂撴稉鈧稉顏堢帛鐠併倗娈戦崜宥呮倻濞喲勫缁夘垰鍨庨弬瑙勭《
		ModelDescription temp_modelDescription;
		temp_modelDescription.ModelName = "ForwardEulerMethod";
		temp_modelDescription.ModelParameter["step"] = basetimestep;
		this->integrationMethod = IntegrationMethodFactory<TimeDrivenLIFModel>::createIntegerationMethod(temp_modelDescription, this);
	}

}

std::map<std::string, boost::any> TimeDrivenLIFModel::getParameters() {
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
