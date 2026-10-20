#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/AdditiveKernalChange.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/State/STDP_State.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"

AdditiveKernalChange::AdditiveKernalChange() : WithTriggerSynaptic() {
}

AdditiveKernalChange::~AdditiveKernalChange() {}

void AdditiveKernalChange::SetParameters(std::map<std::string, boost::any> parametermap) {
	std::map<std::string, boost::any>::iterator iter = parametermap.find("a1pre");
	if (iter != parametermap.end()) {
		this->a1pre = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}

	iter = parametermap.find("a2prepre");
	if (iter != parametermap.end()) {
		this->a2prepre = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}

	iter = parametermap.find("fixweightchange");
	if (iter != parametermap.end()) {
		this->a1pre = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}

	iter = parametermap.find("kernalchange");
	if (iter != parametermap.end()) {
		this->a2prepre = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
}

std::map<std::string, boost::any> AdditiveKernalChange::GetParameters() {
	std::map<std::string, boost::any> parametermap;
	parametermap["a1pre"] = boost::any(this->a1pre);
	parametermap["a2prepre"] = boost::any(this->a2prepre);
	parametermap["fixweightchange"] = boost::any(this->a1pre);
	parametermap["kernalchange"] = boost::any(this->a2prepre);
	return parametermap;
}

void AdditiveKernalChange::ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation) {
	if (connection->TriggerLearning == false) {
		const int rule_index = connection->LearningRuleIndex_withTrigger;
		connection->UpdateWeight(this->a1pre);
		this->State->SetUpdate(rule_index, SpikeTime, simulation->basetimesteps);
		this->State->ApplyPresynapticSpike(rule_index);
	}
	else {
		Neuron* target_neuron = connection->TargetNeuron;
		const int learning_rule_index = this->LearningRuleID;
		for (int i = 0; i < target_neuron->TriggerSynapticLearning_Number[learning_rule_index]; ++i) {
			Interconnections* inter = target_neuron->TriggerSynapticLearning[learning_rule_index][i];
			if (inter->TriggerLearning == false) {
				const int rule_index = inter->LearningRuleIndex_withTrigger;
				this->State->SetUpdate(rule_index, SpikeTime, simulation->basetimesteps);
				inter->UpdateWeight(this->a2prepre * this->State->GetStateValue(rule_index, 0));
			}
		}
	}
}