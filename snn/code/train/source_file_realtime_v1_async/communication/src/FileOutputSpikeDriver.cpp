#include "../source_file_realtime_v1_async/communication/inc/FileOutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include <filesystem>
#include <stdexcept>

FileOutputSpikeDriver::FileOutputSpikeDriver(const char* filename, bool WritePotential) : filename(filename) {
	this->writable = WritePotential;

	std::filesystem::path file_path(filename);
	std::filesystem::path parent_path = file_path.parent_path();
	if (!parent_path.empty()) {
		std::filesystem::create_directories(parent_path);
	}

	this->Handler = fopen(filename, "wt");
	if (this->Handler == nullptr) {
		throw std::runtime_error(std::string("Failed to open output file: ") + filename);
	}
}

FileOutputSpikeDriver::~FileOutputSpikeDriver() {
	if (this->Handler) {
		fclose(this->Handler);
		this->Handler = NULL;
	}
}

void FileOutputSpikeDriver::WriteSpike(Spike* NewSpike, float basetimestep) {
#pragma omp critical (FileOutputSpikeDriver)
	{
		if (this->Handler == nullptr) {
			throw std::runtime_error(std::string("Output spike file handle is null: ") + this->filename);
		}
		fprintf(this->Handler, "%d\t%li\n", NewSpike->getTime(), NewSpike->SourceNeuron->Neuron_index);
	}
}

void FileOutputSpikeDriver::WriteState(int Time, Neuron* SourceNeuron) {
#pragma omp critical (FileOutputSpikeDriver)
	{
		if (this->Handler == nullptr) {
			throw std::runtime_error(std::string("Output state file handle is null: ") + this->filename);
		}
		fprintf(this->Handler, "%d\t%li", Time, SourceNeuron->Neuron_index);

		for (int i = 0; i < SourceNeuron->neuron_state_vector->NumberofStateVariable; ++i) {
			fprintf(this->Handler, "\t%1.12f", SourceNeuron->neuron_state_vector->GetPrintableValuesAt(SourceNeuron->index_in_NeuronModel, i));
		}

		fprintf(this->Handler, "\n");
	}
}

bool FileOutputSpikeDriver::IsBuffered() {
	return false;
}

bool FileOutputSpikeDriver::IsWritePotentialCapable() {
	return this->writable;
}

void FileOutputSpikeDriver::FlushBuffers() {
	return;
}
