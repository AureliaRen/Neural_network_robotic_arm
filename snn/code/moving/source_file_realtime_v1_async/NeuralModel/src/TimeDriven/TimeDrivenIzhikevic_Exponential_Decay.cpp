#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenIzhikevic_Exponential_Decay.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include "../source_file_realtime_v1_async/Current/inc/CurrentSynapse.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/ModelFactory/IntegrationMethodFactory.h"
#include "../source_file_realtime_v1_async/ModelFactory/SynModelFactory.h"
#include "../source_file_realtime_v1_async/SynModel/inc/Synmodel.h"
#include "../source_file_realtime_v1_async/SynModel/inc/non_int/PaperPSPHistory.h"

#include <cmath>

TimeDrivenIzhikevic_I_Exponential_Decay::TimeDrivenIzhikevic_I_Exponential_Decay() : TimeDrivenModel(), synapse_exc(0) {
	this->StateVector = new Neuron_State_Vector(this->N_NeuronStateVariables, true);
	std::string name = std::string("TimeDrivenIzhikevic_I_Exponential_Decay");
	this->setModelName(name);
}

TimeDrivenIzhikevic_I_Exponential_Decay::TimeDrivenIzhikevic_I_Exponential_Decay(int timesteps) : TimeDrivenModel(timesteps), synapse_exc(0) {
	this->StateVector = new Neuron_State_Vector(this->N_NeuronStateVariables, true);
	std::string name = std::string("TimeDrivenIzhikevic_I_Exponential_Decay");
	this->setModelName(name);
}

TimeDrivenIzhikevic_I_Exponential_Decay::~TimeDrivenIzhikevic_I_Exponential_Decay() {
	if (this->synapse_exc != 0) {
		delete this->synapse_exc;
		this->synapse_exc = 0;
	}
	for (int i = 0; i < (int)this->SynapseModelVector.size(); i++) {
		delete this->SynapseModelVector[i];
	}
	this->SynapseModelVector.clear();
	this->ampa_decay_lookup_.clear();
	this->gaba_decay_lookup_.clear();
	this->nmda_decay_lookup_.clear();
}

void TimeDrivenIzhikevic_I_Exponential_Decay::ResetConductanceDecayLookup(float dt) {
	const float inv_ampa_tau = (this->ampa_tau != 0.0f) ? (1.0f / this->ampa_tau) : 0.0f;
	const float inv_gaba_tau = (this->gaba_tau != 0.0f) ? (1.0f / this->gaba_tau) : 0.0f;
	const float inv_nmda_tau = (this->nmda_tau != 0.0f) ? (1.0f / this->nmda_tau) : 0.0f;
	this->ampa_decay_lookup_.assign(1, std::exp(-dt * inv_ampa_tau));
	this->gaba_decay_lookup_.assign(1, std::exp(-dt * inv_gaba_tau));
	this->nmda_decay_lookup_.assign(1, std::exp(-dt * inv_nmda_tau));
}

void TimeDrivenIzhikevic_I_Exponential_Decay::EnsureConductanceDecayLookupSize(float dt) {
	if (this->ampa_decay_lookup_.empty() || this->gaba_decay_lookup_.empty() || this->nmda_decay_lookup_.empty()) {
		this->ResetConductanceDecayLookup(dt);
	}
}

void TimeDrivenIzhikevic_I_Exponential_Decay::InitStateVector(int NumberOfNeurons, int GPUIndex) {
	(void)GPUIndex;
	float new_init[] = { this->V_rest + this->init[0], this->b * this->V_rest + this->init[1], this->init[2], this->init[3], this->init[4], this->init[5] };
	float new_sigma[] = { this->sigma[0], this->sigma[1], this->sigma[2], this->sigma[3], this->sigma[4], this->sigma[5] };
	this->StateVector->InitNeuronState(NumberOfNeurons, new_init, new_sigma);
	this->synapse_exc = new CurrentSynapse(NumberOfNeurons);
	this->NumberOfNeuron = NumberOfNeurons;
}

Neuron_State_Vector* TimeDrivenIzhikevic_I_Exponential_Decay::InitState() {
	return this->StateVector;
}

void TimeDrivenIzhikevic_I_Exponential_Decay::CaculateDifferentialEquation(float* NeuronState, float* AuxNeuronState, int index) {
	const float V = NeuronState[this->index_V];
	const float U = NeuronState[this->index_U];
	float total_current = 0.0f;
	if (this->AMPA) {
		total_current += NeuronState[this->index_g_ampa] * (this->E_ampa - V);
	}
	if (this->GABA) {
		total_current += NeuronState[this->index_g_gaba] * (this->E_gaba - V);
	}
	if (this->NMDA) {
		const float g_nmda_inf = 1.0f / (1.0f + std::exp(-0.062f * V) * (1.2f / 3.57f));
		total_current += NeuronState[this->index_g_nmda] * g_nmda_inf * (this->E_ampa - V);
	}
	if (this->I_EXT) {
		total_current += NeuronState[this->index_I_ext];
	}
	// generic synapse models (Type 4/5), e.g. PaperPSP: additive current
	for (int i = 0; i < (int)this->SynapseModelVector.size(); i++) {
		total_current += this->SynapseModelVector[i]->GetCurrent(NeuronState, index);
	}

	// Eq.(1)-(3): resume derivatives immediately after the reset.
	AuxNeuronState[this->index_V] = 0.04f * V * V + 5.0f * V + 140.0f - U + this->R * total_current;
	AuxNeuronState[this->index_U] = this->a * (this->b * V - U);
}

void TimeDrivenIzhikevic_I_Exponential_Decay::CaculateTimeDependentEquation(float* NeuronState, int index, float dt) {
	(void)index;
	this->EnsureConductanceDecayLookupSize(dt);
	if (this->AMPA) {
		NeuronState[this->index_g_ampa] *= this->ampa_decay_lookup_[0];
	}
	if (this->GABA) {
		NeuronState[this->index_g_gaba] *= this->gaba_decay_lookup_[0];
	}
	if (this->NMDA) {
		NeuronState[this->index_g_nmda] *= this->nmda_decay_lookup_[0];
	}
	// NOTE: shared synapse models (Type 4/5) are advanced once per step in
	// UpdateState, NOT here (this function runs per neuron; advancing a shared
	// model here would make different neurons read it at different stages).
}

void TimeDrivenIzhikevic_I_Exponential_Decay::CaculateSpike(float previous_V, float* NeuronState, int index) {
	(void)previous_V;
	if (NeuronState[this->index_V] >= this->V_th) {
		NeuronState[this->index_V] = this->c;
		NeuronState[this->index_U] += this->d;
		this->StateVector->SpikeIndex[this->StateVector->NumberofSpike] = index;
		this->StateVector->NumberofSpike += 1;
		this->StateVector->LastSpike[index] = 0;
	}
}

void TimeDrivenIzhikevic_I_Exponential_Decay::UpdateState(int index, int time, Simulation* simulation) {
	(void)index;
	this->StateVector->NumberofSpike = 0;
    for (auto* s : this->SynapseModelVector)
        if (auto* h = dynamic_cast<PaperPSPHistory*>(s)) h->BeginStep(time);
	this->integrationMethod->CaculateIncreament(simulation, time);
	for (int i = 0; i < (int)this->SynapseModelVector.size(); i++) {
		this->SynapseModelVector[i]->UpdateSynState(simulation->basetimesteps);
	}
	this->CheckValidIntegeration(time, this->integrationMethod->GetValidIntegrationVariable());
}

InternalSpike* TimeDrivenIzhikevic_I_Exponential_Decay::ProcessSpike(Interconnections* inter, int time) {
    if ((inter->type == 4 || inter->type == 5) && inter->Synmodel_Index >= 0) {
        if (auto* h = dynamic_cast<PaperPSPHistory*>(this->SynapseModelVector[inter->Synmodel_Index])) {
            h->OnArrival(inter, time, inter->type == 5 ? -inter->weight : inter->weight); return 0;
        }
    }
	if (inter->type == 0 && this->Excited) {
		this->StateVector->SetNeuronStateIncrement(inter->TargetNeuronModelIndex, this->index_g_ampa, inter->weight);
	}
	else if (inter->type == 1 && this->Inhibitory) {
		this->StateVector->SetNeuronStateIncrement(inter->TargetNeuronModelIndex, this->index_g_gaba, inter->weight);
	}
	else if (inter->type == 2 && this->NMDA) {
		this->StateVector->SetNeuronStateIncrement(inter->TargetNeuronModelIndex, this->index_g_nmda, inter->weight);
	}
	else if (inter->type == 4 && inter->Synmodel_Index >= 0) {
		this->SynapseModelVector[inter->Synmodel_Index]->ProcessInput(inter, this->StateVector->GetNeuronState(inter->TargetNeuronModelIndex), inter->weight);
	}
	else if (inter->type == 5 && inter->Synmodel_Index >= 0) {
		this->SynapseModelVector[inter->Synmodel_Index]->ProcessInput(inter, this->StateVector->GetNeuronState(inter->TargetNeuronModelIndex), -inter->weight);
	}
	return 0;
}

void TimeDrivenIzhikevic_I_Exponential_Decay::ProcessCurrent(Interconnections* inter, Neuron* Target, float current) {
	this->synapse_exc->SetInputCurrentPerSynapse(Target->index_in_NeuronModel, inter->subindex_type, current);
	float total_current = this->synapse_exc->GetTotalInputCurrentPerNeuron(Target->index_in_NeuronModel);
	this->StateVector->Vector_of_StateVariable[this->N_NeuronStateVariables * Target->index_in_NeuronModel + this->index_I_ext] = total_current;
}

void TimeDrivenIzhikevic_I_Exponential_Decay::InitializeInputCurrentSynapseStructure() {
	if (this->synapse_exc != 0) {
		this->synapse_exc->InitializeInputCurrentPerSynapseStructure();
	}
}

void TimeDrivenIzhikevic_I_Exponential_Decay::CheckType(Interconnections* inter) {
	int Type = inter->type;
	if (Type == 0) {
		this->Excited = true;
		this->AMPA = true;
	}
	else if (Type == 1) {
		this->Inhibitory = true;
		this->GABA = true;
	}
	else if (Type == 2) {
		this->NMDA = true;
	}
	else if (Type == 3) {
		this->I_EXT = true;
		inter->subindex_type = this->synapse_exc->N_connections[inter->TargetNeuron->index_in_NeuronModel];
		this->synapse_exc->IncrementNInputCurrentSynapsesPerNeuron(inter->TargetNeuron->index_in_NeuronModel);
	}
	else if (Type == 4 || Type == 5) {
		if (Type == 4) {
			this->Excited = true;
		}
		else {
			this->Inhibitory = true;
		}
		if (!inter->synmodel.ModelName.empty()) {
			std::map<std::string, boost::any> paramap = this->integrationMethod->getParameters();
			ModelDescription temp_modelDescription;
			temp_modelDescription.ModelName = boost::any_cast<std::string>(paramap["name"]);
			temp_modelDescription.ModelParameter = paramap;
			inter->synmodel.ModelParameter["int_method"] = temp_modelDescription;
			float dt = boost::any_cast<float>(paramap["step"]);
			Synmodel* temp = SynModelFactory::createSynModel(inter->synmodel.ModelName, inter->synmodel.ModelParameter, this->getTimestepSize(), dt);
			if (temp == 0) {
				// unknown / unsupported synapse model name; factory already printed it
				return;
			}
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
				temp->InitSynState(this->NumberOfNeuron);
				this->SynapseModelVector.push_back(temp);
				this->NumberOfSyn++;
			}
			else {
				// an equal model already exists: free the temporary probe (avoid leak)
				delete temp;
			}
			inter->Synmodel_Index = type_index;
		}
	}
	else {
		std::cout << "Error: Type of synapse is not supported in this model" << std::endl;
	}
}

int TimeDrivenIzhikevic_I_Exponential_Decay::getV_index() {
	return this->index_V;
}

int TimeDrivenIzhikevic_I_Exponential_Decay::get_NumberOfState() {
	return this->N_NeuronStateVariables;
}

enum NeuronModelType TimeDrivenIzhikevic_I_Exponential_Decay::getNeuronModelType() {
	return NEURAL_LAYER;
}

void TimeDrivenIzhikevic_I_Exponential_Decay::SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep) {
	std::map<std::string, boost::any>::iterator iter = parametermap.find("a");
	if (iter != parametermap.end()) {
		this->a = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("b");
	if (iter != parametermap.end()) {
		this->b = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("c");
	if (iter != parametermap.end()) {
		this->c = boost::any_cast<float>(iter->second);
		this->V_reset = this->c;
		parametermap.erase(iter);
	}
	iter = parametermap.find("d");
	if (iter != parametermap.end()) {
		this->d = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("V_rest");
	if (iter != parametermap.end()) {
		this->V_rest = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("V_reset");
	if (iter != parametermap.end()) {
		this->V_reset = boost::any_cast<float>(iter->second);
		this->c = this->V_reset;
		parametermap.erase(iter);
	}
	iter = parametermap.find("V_th");
	if (iter != parametermap.end()) {
		this->V_th = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("R");
	if (iter != parametermap.end()) {
		this->R = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("E_ampa");
	if (iter != parametermap.end()) {
		this->E_ampa = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("ampa_tau");
	if (iter != parametermap.end()) {
		this->ampa_tau = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("E_gaba");
	if (iter != parametermap.end()) {
		this->E_gaba = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("gaba_tau");
	if (iter != parametermap.end()) {
		this->gaba_tau = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("nmda_tau");
	if (iter != parametermap.end()) {
		this->nmda_tau = boost::any_cast<float>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("random_mu");
	if (iter != parametermap.end()) {
		this->init = boost::any_cast<std::array<float, 6>>(iter->second);
		parametermap.erase(iter);
	}
	iter = parametermap.find("random_sigma");
	if (iter != parametermap.end()) {
		this->sigma = boost::any_cast<std::array<float, 6>>(iter->second);
		parametermap.erase(iter);
	}

	if (this->integrationMethod != 0) {
		delete this->integrationMethod;
		this->integrationMethod = 0;
	}

	iter = parametermap.find("int_method");
	if (iter != parametermap.end()) {
		ModelDescription temp = boost::any_cast<ModelDescription>(iter->second);
		temp.ModelParameter["step"] = basetimestep;
		this->integrationMethod = IntegrationMethodFactory<TimeDrivenIzhikevic_I_Exponential_Decay>::createIntegerationMethod(temp, this);
		parametermap.erase(iter);
	}
	else {
		ModelDescription temp_modelDescription;
		temp_modelDescription.ModelName = "ForwardEulerMethod";
		temp_modelDescription.ModelParameter["step"] = basetimestep;
		this->integrationMethod = IntegrationMethodFactory<TimeDrivenIzhikevic_I_Exponential_Decay>::createIntegerationMethod(temp_modelDescription, this);
	}
}

std::map<std::string, boost::any> TimeDrivenIzhikevic_I_Exponential_Decay::getParameters() {
	std::map<std::string, boost::any> parametermap;
	parametermap["a"] = this->a;
	parametermap["b"] = this->b;
	parametermap["c"] = this->c;
	parametermap["d"] = this->d;
	parametermap["V_rest"] = this->V_rest;
	parametermap["V_reset"] = this->V_reset;
	parametermap["V_th"] = this->V_th;
	parametermap["R"] = this->R;
	parametermap["E_ampa"] = this->E_ampa;
	parametermap["ampa_tau"] = this->ampa_tau;
	parametermap["E_gaba"] = this->E_gaba;
	parametermap["gaba_tau"] = this->gaba_tau;
	parametermap["nmda_tau"] = this->nmda_tau;
	parametermap["random_mu"] = this->init;
	parametermap["random_sigma"] = this->sigma;
	ModelDescription temp;
	temp.ModelParameter = this->integrationMethod->getParameters();
	temp.ModelName = boost::any_cast<std::string>(temp.ModelParameter["name"]);
	parametermap["int_method"] = temp;
	return parametermap;
}
