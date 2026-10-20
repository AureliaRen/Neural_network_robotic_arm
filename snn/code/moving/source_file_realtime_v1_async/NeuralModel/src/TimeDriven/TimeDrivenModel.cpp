#include <cmath>
#include <stdexcept>
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenModel.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include "../source_file_realtime_v1_async/ModelFactory/IntegrationMethodFactory.h"

TimeDrivenModel::TimeDrivenModel():NeuronModel(), integrationMethod(0){
	this->TimeDriven = true;
}

TimeDrivenModel::TimeDrivenModel(int timestep): NeuronModel(timestep), integrationMethod(0){
	this->TimeDriven = true;
}

TimeDrivenModel::~TimeDrivenModel(){
	if (this->integrationMethod != 0) {
		delete this->integrationMethod;
		this->integrationMethod = 0;
	}
}

void TimeDrivenModel::CheckValidIntegeration(int timestep, float valid_integeration) {
    if (!std::isfinite(valid_integeration)) {
        throw std::runtime_error("Non-finite neuron integration at step " + std::to_string(timestep));
    }
}

