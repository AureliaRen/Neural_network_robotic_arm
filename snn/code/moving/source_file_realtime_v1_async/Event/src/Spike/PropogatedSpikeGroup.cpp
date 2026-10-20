
#include "../source_file_realtime_v1_async/Event/inc/Spike/PropogatedSpikeGroup.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/InternalSpike.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/LearningRule.h"

PropogatedSpikeGroup::PropogatedSpikeGroup(int time, int QueueIndex):Spike(NULL, time, QueueIndex), N_Elements(0){}


PropogatedSpikeGroup::~PropogatedSpikeGroup(){}


bool PropogatedSpikeGroup::IncludeNewSourceNeuron(int NumberOfConnections, Interconnections* FirstConnection) {
	// 将第 Elements 个连接数赋值为 NumberOfConnections
	this->N_ConnectionsWithEqualDelay[N_Elements] = NumberOfConnections;
	// 将第 Elements 个连接赋值为 FirstConnection
	this->ConnectionsWithEqualDelay[N_Elements] = FirstConnection;
    N_Elements++;
	// 检查是否超过最大连接数
	if (N_Elements < this->MaxSize) {
		return false;
	}
	return true;

}

void PropogatedSpikeGroup::ProcessEvent(Simulation* simulation) {
	this->ProcessEvent(simulation, ALL_EVENTS_ENABLED);
}

void PropogatedSpikeGroup::ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level) {
	if (level >= SPIKES_DISABLED) {
		return;
	}
	// 定义一个 Interconnections 指针
	Interconnections* inter;
	LearningRule* lr;
	for (int i = 0; i < this->N_Elements; i++) {
        //灏嗙i涓涓繛鎺ヨ祴鍊肩粰inter
        inter = this->ConnectionsWithEqualDelay[i];
		//访问其中的全部连接
		for (int j = 0; j < this->N_ConnectionsWithEqualDelay[i]; j++) {
			//处理脉冲并返回一个InternalSpike指针，标记传入是否产生放电
			InternalSpike* GeneratedSpike = inter->TargetNeuronModel->ProcessSpike(inter, this->getTime());
			//状态写入
			//if (simulation->neuronMonitorExist) {
				//simulation->WriteState(this->getTime(), inter->TargetNeuron);
			//}

			//濡傛灉浼犲叆浜х敓鏀剧數锛屽垯灏嗗叾鍔犲叆闃熷垪
			if (GeneratedSpike != NULL) {
				simulation->EventHeap->Insert_a_Event(GeneratedSpike, this->getIndex());
			}
			if (level < LEARNING_RULES_DISABLED) {
				lr = inter->LearningRule_withPost;
				if (lr != 0) {
					lr->ApplyPreSynaticSpike(inter, this->getTime(), simulation);
				}
				lr = inter->LearningRule_withTrigger;
				if (lr != 0) {
					lr->ApplyPreSynaticSpike(inter, this->getTime(), simulation);
				}
				lr = inter->LearningRule_withPostAndTrigger;
				if (lr != 0) {
					lr->ApplyPreSynaticSpike(inter, this->getTime(), simulation);
				}
			}

			//灏唅nter鎸囬拡鍚戝悗绉诲姩
			inter++;
		}


	}


}

enum EventPriority PropogatedSpikeGroup::getPriority() {
	return PROPOGATEDSPIKE;
}
