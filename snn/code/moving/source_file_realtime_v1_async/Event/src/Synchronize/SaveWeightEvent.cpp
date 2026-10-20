#include "../source_file_realtime_v1_async/Event/inc/Synchronize/SaveWeightEvent.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Event/inc/Synchronize/SynchronizeSimulationEvent.h"

SaveWeightEvent::SaveWeightEvent(int newtime, Simulation* simulation) :Event(newtime, 0) {
	for (int i = 0; i < simulation->NumberOfQueue; i++) {
		//创建并插入并行事件堆的同步事件
		SynchronizeSimulationEvent* event = new SynchronizeSimulationEvent(newtime, i);
		simulation->EventHeap->Insert_a_Event(event, i);
	}
}

SaveWeightEvent::~SaveWeightEvent() {}


void SaveWeightEvent::ProcessEvent(Simulation* simulation) {
	simulation->SaveWeight();
	
	if (simulation->WeightSaveInterval > 0) {
		//鍒涘缓鏂扮殑鏉冮噸淇濆瓨浜嬩欢
		SaveWeightEvent* event = new SaveWeightEvent(this->getTime() + simulation->WeightSaveInterval, simulation);
		//插入同步事件堆
		simulation->EventHeap->Insert_a_syn_Event(event);
	}
}

enum EventPriority SaveWeightEvent::getPriority() {
	return SAVEWEIGHTEVENT;
}