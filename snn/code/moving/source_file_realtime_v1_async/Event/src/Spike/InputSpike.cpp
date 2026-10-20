#include "../source_file_realtime_v1_async/Event/inc/Spike/InputSpike.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/PropogatedSpike.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"

InputSpike::InputSpike():Spike(){}

InputSpike::InputSpike(Neuron* neuron, int time, int QueueIndex):Spike(neuron, time, QueueIndex){}

InputSpike::~InputSpike(){}

void InputSpike::ProcessEvent(Simulation* simulation) {
	//按队列循环
	simulation->WriteSpike(this);
	for (int i = 0; i < simulation->NumberOfQueue; i++) {
		//濡傛灉婧愮缁忓厓鍚戠洰鏍囬槦鍒梚鐨勮繛鎺ユ暟涓嶄负0
		if (this->SourceNeuron->Output_Synaps_Number[i] != 0) {
			//创建1个传播脉冲
			PropogatedSpike* propSpike = new PropogatedSpike(this->getTime() + this->SourceNeuron->Output_Synaps[i][0]->delay, i, this->SourceNeuron, 0, this->SourceNeuron->PropogationStructure->NDifferentdelays[i]);
			//濡傛灉闃熷垪绱㈠紩鐩稿悓锛屾彃鍏ヨ浜嬩欢
			if (i == this->getIndex()) {
				simulation->EventHeap->Insert_a_Event(propSpike, i);
			}
			else {
				simulation->EventHeap->Insert_a_Event_to_Buffer(propSpike, this->getIndex(), i);
			}
		}
	}
}

void InputSpike::ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level) {
	if (level >= SPIKES_DISABLED) {
		return;
	}
	this->ProcessEvent(simulation);
}

enum EventPriority InputSpike::getPriority() {
	return PROPOGATEDSPIKE;
}
