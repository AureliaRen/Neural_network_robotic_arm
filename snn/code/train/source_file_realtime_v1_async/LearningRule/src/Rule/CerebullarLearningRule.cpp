#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/CerebullarLearningRule.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/SpikeBuffer/BufferedActivityTime.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include <cmath>

CerebullarLearningRule::CerebullarLearningRule(std::map<std::string, boost::any> parametermap) : AdditiveKernalChange(), NoneTriggerSpikeBuffer(0), kernaltable(0) {
	this->SetParameters(parametermap);
}

CerebullarLearningRule::~CerebullarLearningRule() {
	if (this->NoneTriggerSpikeBuffer != 0) {
		delete this->NoneTriggerSpikeBuffer;
	}
	if (this->kernaltable != 0) {
		delete[] this->kernaltable;
	}
}

void CerebullarLearningRule::InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep) {
	(void)NumberOfConnections;
	(void)basetimestep;
	if (this->maxpos <= this->initpos) {
		this->maxpos = this->initpos + 1e-6f;
	}

	const float kernel_width = this->maxpos - this->initpos;
	this->maxtimemeasured = this->maxpos;
	while (true) {
		const float value = (1.0f / kernel_width) * this->maxtimemeasured * std::exp(-(this->maxtimemeasured / kernel_width) + 1.0f);
		if (value < 1e-2f) {
			break;
		}
		this->maxtimemeasured += this->kernel_step_size;
	}
	this->maxtimemeasured += this->initpos;
	this->inv_kernel_step_size = 1.0f / this->kernel_step_size;
	this->N_elements = static_cast<int>(this->maxtimemeasured * this->inv_kernel_step_size) + 1;
	this->kernaltable = new float[this->N_elements];

	for (int i = 0; i < this->N_elements; ++i) {
		const float time = i * this->kernel_step_size;
		if (time < this->initpos) {
			this->kernaltable[i] = 0.0f;
		}
		else {
			const float shifted_time = time - this->initpos;
			this->kernaltable[i] = (1.0f / kernel_width) * shifted_time * std::exp(-(shifted_time / kernel_width) + 1.0f);
		}
	}

	this->NoneTriggerSpikeBuffer = new BufferedActivityTime(NumberOfNeuron);
}

void CerebullarLearningRule::ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation) {
	const float realtime = SpikeTime * simulation->basetimesteps;
	if (connection->TriggerLearning == false) {
		connection->UpdateWeight(this->a1pre);
		const int neuronID = connection->TargetNeuron->Neuron_index;
		const int synapseID = connection->LearningRuleIndex_withTrigger_Target;
		this->NoneTriggerSpikeBuffer->InsetElement(neuronID, realtime, realtime - this->maxtimemeasured, synapseID);
	}
	else {
		Neuron* neuron = connection->TargetNeuron;
		const int neuronID = neuron->Neuron_index;
		const int N_spikes = this->NoneTriggerSpikeBuffer->ProcessElement(neuronID, realtime - this->maxtimemeasured);
		SpikeData* spikedata = this->NoneTriggerSpikeBuffer->GetOutputSpikeData();
		for (int i = 0; i < N_spikes; ++i) {
			Interconnections* interi = neuron->TriggerSynapticLearning[this->LearningRuleID][spikedata[i].synapticID];
			const float elapsetime = realtime - spikedata[i].time;
			int tableindex = static_cast<int>(elapsetime * this->inv_kernel_step_size);
			if (tableindex < 0) {
				continue;
			}
			if (tableindex >= this->N_elements) {
				tableindex = this->N_elements - 1;
			}
			interi->UpdateWeight(this->a2prepre * this->kernaltable[tableindex]);
		}
	}
}

std::map<std::string, boost::any> CerebullarLearningRule::GetParameters() {
	std::map<std::string, boost::any> parameters = AdditiveKernalChange::GetParameters();
	parameters["initpos"] = this->initpos;
	parameters["maxpos"] = this->maxpos;
	return parameters;
}

void CerebullarLearningRule::SetParameters(std::map<std::string, boost::any> parameters) {
	std::map<std::string, boost::any>::iterator iter = parameters.find("initpos");
	if (iter != parameters.end()) {
		this->initpos = boost::any_cast<float>(iter->second);
	}
	iter = parameters.find("maxpos");
	if (iter != parameters.end()) {
		this->maxpos = boost::any_cast<float>(iter->second);
	}
	iter = parameters.find("kernel_step_size");
	if (iter != parameters.end()) {
		this->kernel_step_size = boost::any_cast<float>(iter->second);
	}
	AdditiveKernalChange::SetParameters(parameters);
}