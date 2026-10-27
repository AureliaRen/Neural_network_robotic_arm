#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"


Neuron::Neuron():
	neuron_model(0), neuron_state_vector(0), Output_Synaps_Number(0), Output_Synaps(0), PropogationStructure(0), PostSynapticLearning(0), TriggerSynapticLearning(0), TriggerAndPostSynapticLearning(0), PostSynapticLearning_Number(0), TriggerSynapticLearning_Number(0), TriggerAndPostSynapticLearning_Number(0)
	,NumberOfRule(0), NumberOfTriggerConnectionPerRule(0), TriggerConnectionPerRule(0), TriggerConnectionOwned(false)
{
	this->IndexOfInputLearningIndex = new int** [3]();
}

Neuron::~Neuron() {
	if (this->Output_Synaps != 0) {
		for (int i = 0; i < NumberOfOpenMPQueues; i++) {
			delete[] this->Output_Synaps[i];
		}
		delete[] this->Output_Synaps;
	}

	if (this->TriggerAndPostSynapticLearning != 0) {
		for (int i = 0; i < this->NumberOfRule; i++) {
			if (this->TriggerAndPostSynapticLearning_Number[i] > 0) {
				delete[] this->TriggerAndPostSynapticLearning[i];
			}
		}
		delete[] this->TriggerAndPostSynapticLearning;
	}
	if (this->TriggerAndPostSynapticLearning_Number != 0) {
		delete[] this->TriggerAndPostSynapticLearning_Number;
	}

	if (this->PostSynapticLearning != 0) {
		for (int i = 0; i < this->NumberOfRule; i++) {
			if (this->PostSynapticLearning_Number[i] > 0) {
				delete[] this->PostSynapticLearning[i];
			}
		}
		delete[] this->PostSynapticLearning;
	}
	if (this->PostSynapticLearning_Number != 0) {
		delete[] this->PostSynapticLearning_Number;
	}

	if (this->TriggerSynapticLearning != 0) {
		for (int i = 0; i < this->NumberOfRule; i++) {
			if (this->TriggerSynapticLearning_Number[i] > 0) {
				delete[] this->TriggerSynapticLearning[i];
			}
		}
		delete[] this->TriggerSynapticLearning;
	}
	if (this->TriggerSynapticLearning_Number != 0) {
		delete[] this->TriggerSynapticLearning_Number;
	}

	if (this->NumberOfTriggerConnectionPerRule != 0) {
		for (int i = 0; i < this->NumberOfRule; i++) {
			if (this->NumberOfTriggerConnectionPerRule[i] > 0) {
				delete[] this->TriggerConnectionPerRule[i];
			}
		}
		delete[] this->NumberOfTriggerConnectionPerRule;
		if (this->TriggerConnectionPerRule != 0) {
			delete[] this->TriggerConnectionPerRule;
		}
	}

	if (this->IndexOfInputLearningIndex) {
		for (int bucket = 0; bucket < 3; bucket++) {
			if (this->IndexOfInputLearningIndex[bucket] != 0) {
				for (int i = 0; i < this->NumberOfRule; i++) {
					delete[] this->IndexOfInputLearningIndex[bucket][i];
				}
				delete[] this->IndexOfInputLearningIndex[bucket];
			}
		}

		delete[] this->IndexOfInputLearningIndex;
		this->IndexOfInputLearningIndex = nullptr;
	}


	delete this->PropogationStructure;
	delete[] this->Output_Synaps_Number;
}

void Neuron::InitNeuron(int NeuronIndex, NeuronModel* NeuronModel, int index_in_NeuronModel, int Queue_index, bool monitor, bool output) {
	this->neuron_model = NeuronModel;
	this->Neuron_index = NeuronIndex;
	this->index_in_NeuronModel = index_in_NeuronModel;
	this->Queue_index = Queue_index;
	this->neuron_state_vector = NeuronModel->InitState();
	this->Output_Synaps = NULL;
	this->Output_Synaps_Number = 0;
	this->PropogationStructure = NULL;
	this->IsMonitor = monitor;
	this->IsOutput = output;
}

void Neuron::CaculateOutputDelayStructure() {
	this->PropogationStructure = new NeuronPropogationStructure(this);

	for (int i = 0; i < NumberOfOpenMPQueues; i++) {
		for (int j = 0; j < this->PropogationStructure->NDifferentdelays[i]; j++) {
			this->neuron_model->PropogationStructure->IncludeNewDelay(i, this->PropogationStructure->SynapseDelay[i][j]);
		}
	}
}

void Neuron::SetPostSynapticLearningRule(Interconnections*** Connections, int* ConnectionsNumPerRule, int NumberOfRule) {
	//清空突触可塑性列表
	if (this->PostSynapticLearning != 0) {
		for (int i = 0; i < this->NumberOfRule; i++) {
			if (this->PostSynapticLearning_Number[i] > 0) {
				delete[] this->PostSynapticLearning[i];
			}
		}
		delete[] this->PostSynapticLearning;
	}

	if (this->PostSynapticLearning_Number != 0) {
		delete[] this->PostSynapticLearning_Number;
	}
	//赋值
	this->PostSynapticLearning = Connections;
	this->PostSynapticLearning_Number = ConnectionsNumPerRule;
	this->NumberOfRule = NumberOfRule;
}

void Neuron::SetTriggerSynapticLearningRule(Interconnections*** Connections, int* ConnectionsNumPerRule, int NumberOfRule) {
	int previousRuleCount = this->NumberOfRule;
	//清空突触可塑性列表
	if (this->TriggerSynapticLearning != 0) {
		for (int i = 0; i < previousRuleCount; i++) {
			if (this->TriggerSynapticLearning_Number[i] > 0) {
				delete[] this->TriggerSynapticLearning[i];
			}
		}
		delete[]this->TriggerSynapticLearning;
	}
	

	if (this->TriggerSynapticLearning_Number != 0) {
		delete[] this->TriggerSynapticLearning_Number;
	}
	//赋值
	this->TriggerSynapticLearning = Connections;
	this->TriggerSynapticLearning_Number = ConnectionsNumPerRule;
	this->NumberOfRule = NumberOfRule;

	if (this->TriggerConnectionOwned) {
		for (int i = 0; i < previousRuleCount; i++) {
			delete[] this->TriggerConnectionPerRule[i];
		}
		delete[] this->TriggerConnectionPerRule;
		delete[] this->NumberOfTriggerConnectionPerRule;
	}

	this->NumberOfTriggerConnectionPerRule = new int[this->NumberOfRule]();
	this->TriggerConnectionPerRule = new Interconnections**[this->NumberOfRule]();
	this->TriggerConnectionOwned = true;

	for (int i = 0; i < this->NumberOfRule; i++) {
		if (ConnectionsNumPerRule[i] > 0) {
			Interconnections** aux = new Interconnections * [ConnectionsNumPerRule[i]];
			for (int j = 0; j < ConnectionsNumPerRule[i]; j++) {
				if (Connections[i][j]->TriggerLearning) {
					aux[this->NumberOfTriggerConnectionPerRule[i]] = Connections[i][j];
					this->NumberOfTriggerConnectionPerRule[i]++;
				}
			}

			if (this->NumberOfTriggerConnectionPerRule[i] > 0) {
				this->TriggerConnectionPerRule[i] = new Interconnections*[this->NumberOfTriggerConnectionPerRule[i]];
				for (int j = 0; j < this->NumberOfTriggerConnectionPerRule[i]; j++) {
					this->TriggerConnectionPerRule[i][j] = aux[j];
				}
			}
			delete[] aux;

		}
	}
}

void Neuron::SetTriggerAndPostSynapticLearningRule(Interconnections*** Connections, int* ConnectionsNumPerRule, int NumberOfRule) {
	if (this->TriggerAndPostSynapticLearning != 0) {
		for (int i = 0; i < this->NumberOfRule; i++) {
			if (this->TriggerAndPostSynapticLearning_Number[i] > 0) {
				delete[]this->TriggerAndPostSynapticLearning[i];
			}
		}
		delete[]this->TriggerAndPostSynapticLearning;
	}
	if (this->TriggerAndPostSynapticLearning_Number != 0) {
		delete[] this->TriggerAndPostSynapticLearning_Number;
	}
	this->TriggerAndPostSynapticLearning = Connections;
	this->TriggerAndPostSynapticLearning_Number = ConnectionsNumPerRule;
	this->NumberOfRule = NumberOfRule;

	if (!this->TriggerConnectionOwned) {
		this->NumberOfTriggerConnectionPerRule = new int[this->NumberOfRule]();
		this->TriggerConnectionPerRule = new Interconnections**[this->NumberOfRule]();
		this->TriggerConnectionOwned = true;
	}

	for (int i = 0; i < this->NumberOfRule; i++) {
		if (ConnectionsNumPerRule[i] > 0) {
			Interconnections** aux = new Interconnections * [ConnectionsNumPerRule[i]];
			for (int j = 0; j < ConnectionsNumPerRule[i]; j++) {
				if (Connections[i][j]->TriggerLearning) {
					aux[this->NumberOfTriggerConnectionPerRule[i]] = Connections[i][j];
					this->NumberOfTriggerConnectionPerRule[i]++;
				}
			}

			if (this->NumberOfTriggerConnectionPerRule[i] > 0) {
				this->TriggerConnectionPerRule[i] = new Interconnections*[this->NumberOfTriggerConnectionPerRule[i]];
				for (int j = 0; j < this->NumberOfTriggerConnectionPerRule[i]; j++) {
					this->TriggerConnectionPerRule[i][j] = aux[j];
				}
			}
            delete[] aux;

		}
	}

}

void Neuron::InitLearningConnections() {
	//分配内存
	if (this->NumberOfRule > 0) {
		//突触后学习
		this->IndexOfInputLearningIndex[0] = new int* [this->NumberOfRule]();
		//突触前学习
		this->IndexOfInputLearningIndex[1] = new int* [this->NumberOfRule]();
		//突触前突触后学习
		this->IndexOfInputLearningIndex[2] = new int* [this->NumberOfRule]();
	}

	for (int i = 0; i < this->NumberOfRule; i++) {
		if (this->PostSynapticLearning_Number[i] > 0) {
			this->IndexOfInputLearningIndex[0][i] = new int[this->PostSynapticLearning_Number[i]];
			for (int j = 0; j < this->PostSynapticLearning_Number[i]; j++){
				if (this->PostSynapticLearning[i][j]->TriggerLearning) {
					this->IndexOfInputLearningIndex[0][i][j] = -1;
				}
				else {
					//记录突触可塑性在SynapseState中索引
					this->IndexOfInputLearningIndex[0][i][j] = this->PostSynapticLearning[i][j]->LearningRuleIndex_withPost;
				}
				//记录突触可塑性模型对应的全局索引
				this->PostSynapticLearning[i][j]->LearningRuleIndex_withPost_Target = j;
			}
		}

		if (this->TriggerSynapticLearning_Number[i] > 0) {
			this->IndexOfInputLearningIndex[1][i] = new int[this->TriggerSynapticLearning_Number[i]];
			for (int j = 0; j < this->TriggerSynapticLearning_Number[i]; j++) {
				if (this->TriggerSynapticLearning[i][j]->TriggerLearning) {
					this->IndexOfInputLearningIndex[1][i][j] = -1;
				}
				else {
					this->IndexOfInputLearningIndex[1][i][j] = this->TriggerSynapticLearning[i][j]->LearningRuleIndex_withTrigger;
				}

				this->TriggerSynapticLearning[i][j]->LearningRuleIndex_withTrigger_Target = j;
			}
		}

		if (this->TriggerAndPostSynapticLearning_Number[i] > 0) {
			this->IndexOfInputLearningIndex[2][i] = new int[this->TriggerAndPostSynapticLearning_Number[i]];
			for (int j = 0; j < this->TriggerAndPostSynapticLearning_Number[i]; j++) {
				if (this->TriggerAndPostSynapticLearning[i][j]->TriggerLearning) {
					this->IndexOfInputLearningIndex[2][i][j] = -1;
				}
				else {
					this->IndexOfInputLearningIndex[2][i][j] = this->TriggerAndPostSynapticLearning[i][j]->LearningRuleIndex_withPostAndTrigger;
				}

				this->TriggerAndPostSynapticLearning[i][j]->LearningRuleIndex_withPostAndTrigger_Target = j;
			}
		}

	}
}
