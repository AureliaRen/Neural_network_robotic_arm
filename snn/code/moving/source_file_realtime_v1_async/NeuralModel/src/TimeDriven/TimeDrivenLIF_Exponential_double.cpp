#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenLIF_Exponential_double.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include "../source_file_realtime_v1_async/Current/inc/CurrentSynapse.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/ModelFactory/IntegrationMethodFactory.h"

#include <algorithm>
#include <cmath>

TimeDrivenLIF_Exponential_double::TimeDrivenLIF_Exponential_double() : TimeDrivenModel(), CurrentSynapeModel(0) {
    this->StateVector = new Neuron_State_Vector(this->N_NeuronStateVariables, true);
    std::string name = std::string("TimeDrivenLIF_Exponential_double");
    this->setModelName(name);
}

TimeDrivenLIF_Exponential_double::TimeDrivenLIF_Exponential_double(int timesteps) : TimeDrivenModel(timesteps), CurrentSynapeModel(0) {
    this->StateVector = new Neuron_State_Vector(this->N_NeuronStateVariables, true);
    std::string name = std::string("TimeDrivenLIF_Exponential_double");
    this->setModelName(name);
}

TimeDrivenLIF_Exponential_double::~TimeDrivenLIF_Exponential_double() {
    if (this->CurrentSynapeModel != 0) {
        delete this->CurrentSynapeModel;
        this->CurrentSynapeModel = 0;
    }
    if (!this->gexc_decay_lookup_.empty()) {
        this->gexc_decay_lookup_.clear();
    }
    if (!this->ginh_decay_lookup_.empty()) {
        this->ginh_decay_lookup_.clear();
    }
}

void TimeDrivenLIF_Exponential_double::ResetConductanceDecayLookup(float dt) {
    //将gexc_decay_lookup_和ginh_decay_lookup_的大小设置为1，并将所有元素初始化为exp(dt/tau)
    const float inv_gexc_tau = (this->gexc_tau != 0.0f) ? (1.0f / this->gexc_tau) : 0.0f;
    const float inv_ginh_tau = (this->ginh_tau != 0.0f) ? (1.0f / this->ginh_tau) : 0.0f;
    this->gexc_decay_lookup_.assign(1, std::exp(-dt * inv_gexc_tau));
    this->ginh_decay_lookup_.assign(1, std::exp(-dt * inv_ginh_tau));
}

void TimeDrivenLIF_Exponential_double::EnsureConductanceDecayLookupSize(float dt) {
    if (this->gexc_decay_lookup_.empty() || this->ginh_decay_lookup_.empty()) {
        this->ResetConductanceDecayLookup(dt);
    }
}

void TimeDrivenLIF_Exponential_double::InitStateVector(int NumberOfNeurons, int GPUIndex) {
    float new_init[] = { this->V_rest + this->init[0], this->init[1], this->init[2], this->init[3] };
    float new_sigma[] = { this->sigma[0], this->sigma[1], this->sigma[2], this->sigma[3] };
    this->StateVector->InitNeuronState(NumberOfNeurons, new_init, new_sigma);
    this->CurrentSynapeModel = new CurrentSynapse(NumberOfNeurons);
}

Neuron_State_Vector* TimeDrivenLIF_Exponential_double::InitState() {
    return this->StateVector;
}

void TimeDrivenLIF_Exponential_double::CaculateDifferentialEquation(float* NeuronState, float* AuxNeuronState, int index) {
    float current_dV = 0.0f;
    if (this->I_EXT) {
        current_dV += this->R * NeuronState[this->I_EXT_index] / this->tau;
    }
    current_dV += NeuronState[this->index_gexc] * (this->Eexc - NeuronState[this->index_V]) / this->tau;
    current_dV += NeuronState[this->index_ginh] * (this->Einhibitory - NeuronState[this->index_V]) / this->tau;
    if (this->StateVector->LastSpike[index] > this->t_ref) {
        AuxNeuronState[index_V] = current_dV + (this->V_rest - NeuronState[index_V]) / this->tau;
    }
    else {
        AuxNeuronState[index_V] = 0.0f;
    }
}

void TimeDrivenLIF_Exponential_double::CaculateTimeDependentEquation(float* NeuronState, int index, float dt) {
    (void)index;
    if (!this->Excited && !this->Inhibitory) {
        return;
    }

    this->EnsureConductanceDecayLookupSize(dt);

    NeuronState[this->index_gexc] *= this->gexc_decay_lookup_[0];
    NeuronState[this->index_ginh] *= this->ginh_decay_lookup_[0];
}

void TimeDrivenLIF_Exponential_double::CaculateSpike(float previous_V, float* NeuronState, int index) {
    (void)previous_V;
    if (NeuronState[this->index_V] > this->V_th) {
        NeuronState[this->index_V] = this->V_reset;
        this->StateVector->SpikeIndex[this->StateVector->NumberofSpike] = index;
        this->StateVector->NumberofSpike += 1;
        this->StateVector->LastSpike[index] = 0;
    }
}

void TimeDrivenLIF_Exponential_double::UpdateState(int index, int time, Simulation* simulation) {
    (void)index;
    this->StateVector->NumberofSpike = 0;
    this->integrationMethod->CaculateIncreament(simulation, time);
    this->CheckValidIntegeration(time, this->integrationMethod->GetValidIntegrationVariable());
}

InternalSpike* TimeDrivenLIF_Exponential_double::ProcessSpike(Interconnections* inter, int time) {
    (void)time;
    if (inter->type == 0 && this->Excited) {
        this->StateVector->SetNeuronStateIncrement(inter->TargetNeuronModelIndex, this->index_gexc, inter->weight);
    }
    else if (inter->type == 1 && this->Inhibitory) {
        this->StateVector->SetNeuronStateIncrement(inter->TargetNeuronModelIndex, this->index_ginh, inter->weight);
    }
    return 0;
}

void TimeDrivenLIF_Exponential_double::ProcessCurrent(Interconnections* inter, Neuron* Target, float current) {
    this->CurrentSynapeModel->SetInputCurrentPerSynapse(Target->index_in_NeuronModel, inter->subindex_type, current);
    float total_current = this->CurrentSynapeModel->GetTotalInputCurrentPerNeuron(Target->index_in_NeuronModel);
    this->StateVector->Vector_of_StateVariable[this->N_NeuronStateVariables * Target->index_in_NeuronModel + this->I_EXT_index] = total_current;
}

void TimeDrivenLIF_Exponential_double::InitializeInputCurrentSynapseStructure() {
    if (this->CurrentSynapeModel != 0) {
        this->CurrentSynapeModel->InitializeInputCurrentPerSynapseStructure();
    }
}

void TimeDrivenLIF_Exponential_double::CheckType(Interconnections* inter) {
    int Type = inter->type;
    if (Type == 0) {
        this->Excited = true;
    }
    else if (Type == 1) {
        this->Inhibitory = true;
    }
    else if (Type == 3) {
        this->I_EXT = true;
        inter->subindex_type = this->CurrentSynapeModel->N_connections[inter->TargetNeuron->index_in_NeuronModel];
        this->CurrentSynapeModel->IncrementNInputCurrentSynapsesPerNeuron(inter->TargetNeuron->index_in_NeuronModel);
    }
    else if (Type == 2) {
        std::cout << "Error: NMDA is not supported in this model" << std::endl;
    }
    else {
        std::cout << "Error: Type of synapse is not supported in this model" << std::endl;
    }
}

int TimeDrivenLIF_Exponential_double::getV_index() {
    return this->index_V;
}

int TimeDrivenLIF_Exponential_double::get_NumberOfState() {
    return this->N_NeuronStateVariables;
}

enum NeuronModelType TimeDrivenLIF_Exponential_double::getNeuronModelType() {
    return NEURAL_LAYER;
}

void TimeDrivenLIF_Exponential_double::SetParameters(std::map<std::string, boost::any> parametermap, float basetimesteps) {
    std::map<std::string, boost::any>::iterator iter = parametermap.find("V_rest");
    if (iter != parametermap.end()) {
        this->V_rest = boost::any_cast<float>(iter->second);
        parametermap.erase(iter);
    }
    iter = parametermap.find("tau");
    if (iter != parametermap.end()) {
        this->tau = boost::any_cast<float>(iter->second);
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
    iter = parametermap.find("V_reset");
    if (iter != parametermap.end()) {
        this->V_reset = boost::any_cast<float>(iter->second);
        parametermap.erase(iter);
    }
    iter = parametermap.find("t_ref");
    if (iter != parametermap.end()) {
        this->t_ref = boost::any_cast<int>(iter->second);
        parametermap.erase(iter);
    }
    iter = parametermap.find("gexc_tau");
    if (iter != parametermap.end()) {
        this->gexc_tau = boost::any_cast<float>(iter->second);
        parametermap.erase(iter);
    }
    iter = parametermap.find("Eexc");
    if (iter != parametermap.end()) {
        this->Eexc = boost::any_cast<float>(iter->second);
        parametermap.erase(iter);
    }
    iter = parametermap.find("ginh_tau");
    if (iter != parametermap.end()) {
        this->ginh_tau = boost::any_cast<float>(iter->second);
        parametermap.erase(iter);
    }
    iter = parametermap.find("Einh");
    if (iter != parametermap.end()) {
        this->Einhibitory = boost::any_cast<float>(iter->second);
        parametermap.erase(iter);
    }
    iter = parametermap.find("random_mu");
    if (iter != parametermap.end()) {
        this->init = boost::any_cast<std::array<float, 4>>(iter->second);
        parametermap.erase(iter);
    }
    iter = parametermap.find("random_sigma");
    if (iter != parametermap.end()) {
        this->sigma = boost::any_cast<std::array<float, 4>>(iter->second);
        parametermap.erase(iter);
    }

    if (this->integrationMethod != 0) {
        delete this->integrationMethod;
        this->integrationMethod = 0;
    }

    iter = parametermap.find("int_method");
    if (iter != parametermap.end()) {
        ModelDescription temp = boost::any_cast<ModelDescription>(iter->second);
        temp.ModelParameter["step"] = basetimesteps * this->getTimestepSize();
        this->integrationMethod = IntegrationMethodFactory<TimeDrivenLIF_Exponential_double>::createIntegerationMethod(temp, this);
        parametermap.erase(iter);
    }
    else {
        ModelDescription temp_modelDescription;
        temp_modelDescription.ModelName = "ForwardEulerMethod";
        temp_modelDescription.ModelParameter["step"] = basetimesteps * this->getTimestepSize();
        this->integrationMethod = IntegrationMethodFactory<TimeDrivenLIF_Exponential_double>::createIntegerationMethod(temp_modelDescription, this);
    }

}

std::map<std::string, boost::any> TimeDrivenLIF_Exponential_double::getParameters() {
    std::map<std::string, boost::any> parametermap;
    parametermap["V_rest"] = this->V_rest;
    parametermap["tau"] = this->tau;
    parametermap["V_th"] = this->V_th;
    parametermap["R"] = this->R;
    parametermap["V_reset"] = this->V_reset;
    parametermap["t_ref"] = this->t_ref;
    parametermap["gexc_tau"] = this->gexc_tau;
    parametermap["Eexc"] = this->Eexc;
    parametermap["ginh_tau"] = this->ginh_tau;
    parametermap["Einh"] = this->Einhibitory;
    parametermap["random_mu"] = this->init;
    parametermap["random_sigma"] = this->sigma;
    ModelDescription temp;
    temp.ModelParameter = this->integrationMethod->getParameters();
    temp.ModelName = boost::any_cast<std::string>(temp.ModelParameter["name"]);
    parametermap["int_method"] = temp;
    return parametermap;
}
