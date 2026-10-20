#include "../source_file_realtime_v1_async/NeuralModel/inc/EventDriven/EventDrivenInputDevice.h"

EventDrivenInputDevice::EventDrivenInputDevice(): NeuronModel(){
	this->TimeDriven = false;
}

EventDrivenInputDevice::EventDrivenInputDevice(int timesteps): NeuronModel(timesteps){
	this->TimeDriven = false;
}

EventDrivenInputDevice::~EventDrivenInputDevice() {}

