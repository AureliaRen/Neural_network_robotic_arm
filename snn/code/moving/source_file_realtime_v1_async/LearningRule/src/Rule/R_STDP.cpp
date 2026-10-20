#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/R_STDP.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/State/STDP_State.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <boost/any.hpp>


R_STDP::R_STDP(std::map<std::string, boost::any> parametermap) : WithTriggerAndPostSynaptic() {
    this->SetParameters(parametermap);
}

R_STDP::~R_STDP() {}

void R_STDP::InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep) {
    this->State = new STDP_State(NumberOfConnections, this->LTP_tau, this->LTD_tau, true);
}

void R_STDP::ApplyPostSynaticSpike(Neuron* neuron, int SpikeTime, Simulation* simulation) {
    int LearningRuleID = this->LearningRuleID;
    for (int i = 0; i < neuron->TriggerAndPostSynapticLearning_Number[LearningRuleID]; i++) {
        int IndexInLearningRuleIndex = neuron->IndexOfInputLearningIndex[2][LearningRuleID][i];
        if (IndexInLearningRuleIndex < 0) {
            continue;
        }

        Interconnections* Connection = neuron->TriggerAndPostSynapticLearning[LearningRuleID][i];
        if (Connection->TriggerLearning) {
            continue;
        }

        this->State->SetUpdate(IndexInLearningRuleIndex, SpikeTime, simulation->basetimesteps);
        this->State->ApplyPostsynapticSpike(IndexInLearningRuleIndex);
        float EligibilityDelta = this->State->GetStateValue(IndexInLearningRuleIndex, 0) * this->MaxLTP;
        this->State->StateJump(IndexInLearningRuleIndex, 2, EligibilityDelta);
    }
}

void R_STDP::ApplyPreSynaticSpike(Interconnections* Connection, int SpikeTime, Simulation* simulation) {
    if (Connection->TriggerLearning == false) {
        int IndexInLearningRuleIndex = Connection->LearningRuleIndex_withPostAndTrigger;
        this->State->SetUpdate(IndexInLearningRuleIndex, SpikeTime, simulation->basetimesteps);
        this->State->ApplyPresynapticSpike(IndexInLearningRuleIndex);

        float EligibilityDelta = -this->MaxLTD * this->State->GetStateValue(IndexInLearningRuleIndex, 1);
        this->State->StateJump(IndexInLearningRuleIndex, 2, EligibilityDelta);
        return;
    }

    int LearningRuleID = this->LearningRuleID;
    Neuron* neuron = Connection->TargetNeuron;
    float TriggerFactor = (Connection->type == 1) ? this->PunishmentFactor : this->RewardFactor;
    for (int i = 0; i < neuron->TriggerAndPostSynapticLearning_Number[LearningRuleID]; i++) {
        int IndexInLearningRuleIndex = neuron->IndexOfInputLearningIndex[2][LearningRuleID][i];
        if (IndexInLearningRuleIndex < 0) {
            continue;
        }
        Interconnections* PlasticConnection = neuron->TriggerAndPostSynapticLearning[LearningRuleID][i];
        if (PlasticConnection->TriggerLearning) {
            continue;
        }

        this->State->SetUpdate(IndexInLearningRuleIndex, SpikeTime, simulation->basetimesteps);
        float Eligibility = this->State->GetStateValue(IndexInLearningRuleIndex, 2);
        PlasticConnection->UpdateWeight(TriggerFactor * Eligibility);
        if (this->ClearEligibilityAfterTrigger) {
            this->State->SetStateValue(IndexInLearningRuleIndex, 2, 0.0f);
        }
    }
}

void R_STDP::SetParameters(std::map<std::string, boost::any> parametermap) {
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

    iter = parametermap.find("RewardFactor");
    if (iter != parametermap.end()) {
        this->RewardFactor = boost::any_cast<float>(iter->second);
        parametermap.erase(iter);
    }

    iter = parametermap.find("PunishmentFactor");
    if (iter != parametermap.end()) {
        this->PunishmentFactor = boost::any_cast<float>(iter->second);
        parametermap.erase(iter);
    }

    iter = parametermap.find("ClearEligibilityAfterTrigger");
    if (iter != parametermap.end()) {
        this->ClearEligibilityAfterTrigger = boost::any_cast<bool>(iter->second);
        parametermap.erase(iter);
    }
}

std::map<std::string, boost::any> R_STDP::GetParameters() {
    std::map<std::string, boost::any> parametermap;
    parametermap["Max_LTP"] = this->MaxLTP;
    parametermap["LTP_tau"] = this->LTP_tau;
    parametermap["Max_LTD"] = this->MaxLTD;
    parametermap["LTD_tau"] = this->LTD_tau;
    parametermap["RewardFactor"] = this->RewardFactor;
    parametermap["PunishmentFactor"] = this->PunishmentFactor;
    parametermap["ClearEligibilityAfterTrigger"] = this->ClearEligibilityAfterTrigger;
    return parametermap;
}
