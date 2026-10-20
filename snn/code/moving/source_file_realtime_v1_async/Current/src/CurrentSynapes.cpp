#include "../source_file_realtime_v1_async/Current/inc/CurrentSynapse.h"

CurrentSynapse::CurrentSynapse():N_target_neurons(0){}


CurrentSynapse::CurrentSynapse(int n_target_neurons):N_target_neurons(n_target_neurons){
	//涓篘_connections鍒嗛厤鍐呭瓨
	this->N_connections = new int[this->N_target_neurons]();
}

CurrentSynapse::~CurrentSynapse() {
	for (int i = 0; i < this->N_target_neurons; i++) {
		if (this->N_connections[i] > 0) {
			delete[] this->currents_per_connection[i];
		}
	}
	delete[] this->currents_per_connection;
	delete[] this->N_connections;
}

void CurrentSynapse::IncrementNInputCurrentSynapsesPerNeuron(int neuron_index) {
	this->N_connections[neuron_index]++;
}

void CurrentSynapse::InitializeInputCurrentPerSynapseStructure() {
	//分配currents_per_connection的内存（目标神经元数量*每个目标神经元的连接数）
	this->currents_per_connection = new float* [this->N_target_neurons]();
	for (int i = 0; i < this->N_target_neurons; i++) {
		if (this->N_connections[i] > 0) {
            this->currents_per_connection[i] = new float[this->N_connections[i]]();
		}
	}
}

void CurrentSynapse::SetInputCurrentPerSynapse(int neuron_index, int synapse_index, float current) {
	this->currents_per_connection[neuron_index][synapse_index] = current;
}

float CurrentSynapse::GetTotalInputCurrentPerNeuron(int neuron_index) {
	float current = 0.0;
	for (int i = 0; i < this->N_connections[neuron_index]; i++) {
		current += this->currents_per_connection[neuron_index][i];
	}
	return current;
}