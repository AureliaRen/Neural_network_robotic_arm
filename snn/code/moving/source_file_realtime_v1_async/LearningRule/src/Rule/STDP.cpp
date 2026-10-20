#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/STDP.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/State/STDP_State.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include <boost/any.hpp>

STDP::STDP(std::map<std::string, boost::any> parametermap): WithPostSynaptic() {
	this->SetParameters(parametermap);
}


STDP::~STDP(){}

void STDP::InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep) {
	this->State = new STDP_State(NumberOfConnections, this->LTP_tau, this->LTD_tau);
}

void STDP::ApplyPreSynaticSpike(Interconnections* Connection, int SpikeTime, Simulation* Sim) {
	//鑾峰彇杩炴帴鐨勫涔犺鍒欏湪SynapseState涓殑绱㈠紩
	int IndexInLearningRuleIndex = Connection->LearningRuleIndex_withPost;
	//执行Apre与Apost的衰减
	this->State->SetUpdate(IndexInLearningRuleIndex, SpikeTime, Sim->basetimesteps);
	//执行Apre的跳变
	this->State->ApplyPresynapticSpike(IndexInLearningRuleIndex);
	//鏇存柊鏉冮噸
	Connection->weight += -this->MaxLTD * this->State->StateValue[IndexInLearningRuleIndex * this->State->NumberOfState + 1];
	//检查权重是否越界
	if (Connection->weight < 0) {
        Connection->weight = 0;
	}
	else if (Connection->weight > Connection->maximum_weight) {
		Connection->weight = Connection->maximum_weight;
	}
}

void STDP::ApplyPostSynaticSpike(Neuron* neuron, int SpikeTime, Simulation* Sim) {
	// 获取学习规则的全局索引
	int LearningRuleID = this->LearningRuleID;
	//获取突触后神经元neuron采用STDP学习的全部连接
	for (int i = 0; i < neuron->PostSynapticLearning_Number[LearningRuleID]; i++) {
        //鑾峰彇杩炴帴
		Interconnections* Connection = neuron->PostSynapticLearning[LearningRuleID][i];
		//鑾峰彇杩炴帴鐨勫涔犺鍒欏湪SynapseState涓殑绱㈠紩
		int IndexInLearningRuleIndex = neuron->IndexOfInputLearningIndex[0][LearningRuleID][i];
		//执行Apre与Apost的衰减
		this->State->SetUpdate(IndexInLearningRuleIndex, SpikeTime, Sim->basetimesteps);
		//执行Apost的跳变
		this->State->ApplyPostsynapticSpike(IndexInLearningRuleIndex);
		//鏇存柊鏉冮噸
		Connection->weight += this->MaxLTP * this->State->StateValue[IndexInLearningRuleIndex * this->State->NumberOfState];

		if (Connection->weight < 0) {
			Connection->weight = 0;
		}
		else if (Connection->weight > Connection->maximum_weight) {
			Connection->weight = Connection->maximum_weight;
		}
	}

}

void STDP::SetParameters(std::map<std::string, boost::any> parametermap) {
	std::map<std::string, boost::any>::iterator iter = parametermap.find("Max_LTP");
	if (iter != parametermap.end()) {
		this->MaxLTP = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}

	iter = parametermap.find("LTP_tau");
	if (iter != parametermap.end()) {
		this->LTP_tau = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}

	iter = parametermap.find("Max_LTD");
	if (iter != parametermap.end()) {
		this->MaxLTD = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}

	iter = parametermap.find("LTD_tau");
	if (iter != parametermap.end()) {
		this->LTD_tau = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
}

std::map<std::string, boost::any> STDP::GetParameters() {
	std::map<std::string, boost::any> parametermap;
	parametermap["Max_LTP"] = this->MaxLTP;
	parametermap["LTP_tau"] = this->LTP_tau;
    parametermap["Max_LTD"] = this->MaxLTD;
    parametermap["LTD_tau"] = this->LTD_tau;
	return parametermap;
}