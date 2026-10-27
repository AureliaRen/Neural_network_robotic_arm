#ifndef CEREBULLAR_LEARNING_RULE_H
#define CEREBULLAR_LEARNING_RULE_H

#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/AdditiveKernalChange.h"
class BufferedActivityTime;

class CerebullarLearningRule : public AdditiveKernalChange {
	public:
		float initpos = 120.0f;

		float maxpos = 150.0f;

		float maxtimemeasured;

		float kernel_step_size = 0.1f;

		float inv_kernel_step_size;

		int N_elements;

		float* kernaltable;

		BufferedActivityTime* NoneTriggerSpikeBuffer;

		CerebullarLearningRule(std::map<std::string, boost::any> parametermap);

		~CerebullarLearningRule();

		virtual void InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep);

		virtual void ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation);

		virtual std::map<std::string, boost::any> GetParameters();

		virtual void SetParameters(std::map<std::string, boost::any> parametermap);
};

#endif