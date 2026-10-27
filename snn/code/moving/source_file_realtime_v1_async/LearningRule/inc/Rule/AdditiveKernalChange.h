#ifndef ADDITIVEKERNELCHANGE_H
#define ADDITIVEKERNELCHANGE_H

#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/WithTriggerSynaptic.h"

class AdditiveKernalChange : public WithTriggerSynaptic {
	public:
		float a1pre = 0.002f;

		float a2prepre = -0.08f;

		AdditiveKernalChange();

		~AdditiveKernalChange();

		virtual void SetParameters(std::map<std::string, boost::any> parametermap);

		virtual void InitState(int NumberOfConnections, int NumberOfNeuron, float basetimestep) = 0;

		virtual void ApplyPreSynaticSpike(Interconnections* connection, int SpikeTime, Simulation* simulation);

		virtual std::map<std::string, boost::any> GetParameters();
};

#endif // ADDITIVEKERNELCHANGE_H
