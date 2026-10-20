/*
* 文件名:TimeDrivenIzhikevic_I_Exponential_Decay.h
* 定义了一个Izikevic模型，该模型具有指数衰减的电流门控
*/
#ifndef TIME_DRIVEN_IZHIKEVIC_I_EXPONENTIAL_DECAY_H
#define TIME_DRIVEN_IZHIKEVIC_I_EXPONENTIAL_DECAY_H
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenModel.h"
#include <array>
#include <vector>

class CurrentSynapse;

class Synmodel;

class TimeDrivenIzhikevic_I_Exponential_Decay : public TimeDrivenModel {
	public:
		//一些默认参数
		bool I_EXT = true;

		bool Excited = false;

		bool Inhibitory = false;

		bool AMPA = false;

		bool GABA = false;

		bool NMDA = false;

		float a = 0.02f;

		float b = 0.2f;

		float c = -65.0f;

		float d = 8.0f;

		float V_rest = -65.0f;

		float V_reset = -65.0f;

		float V_th = 30.0f;

		float R = 1.0f;

		float E_ampa = 0.0f;

		float ampa_tau = 5.0f;

		float E_gaba = -80.0f;

		float gaba_tau = 10.0f;

		float nmda_tau = 100.0f;

		const int N_NeuronStateVariables = 6; //神经元状态变量个数：电压，电压恢复, AMPA门控，GABA门控，NMDA门控，外加电流

		const int N_DifferentialStates = 2; //微分方程个数：电压， 电压恢复

		const int index_V = 0; //电压

		const int index_U = 1; //电压恢复

		const int index_g_ampa = 2; //AMPA门控

		const int index_g_gaba = 3; //GABA门控

		const int index_g_nmda = 4; //NMDA门控

		const int index_I_ext = 5; //外加电流

		const int TimeDependentInputSize = 4; //时间依赖输入个数：AMPA, GABA, NMDA, 外加电流

		CurrentSynapse* synapse_exc; //电流突触模型

		int NumberOfNeuron = 0; //该模型的神经元数量

		int NumberOfSyn = 0; //通用突触模型数量

		std::vector<Synmodel*> SynapseModelVector; //通用突触模型向量(用于PaperPSP等)

		std::array<float, 6> init = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }; //初始化状态变量的偏差随机数

		std::array<float, 6> sigma = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }; //鍒濆鍖栫姸鎬佸彉閲忕殑鏂瑰樊

		/*
		* 构造函数
		*/
		TimeDrivenIzhikevic_I_Exponential_Decay();

		/*
		* 带参数的构造函数
		*/
		TimeDrivenIzhikevic_I_Exponential_Decay(int timesteps);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~TimeDrivenIzhikevic_I_Exponential_Decay();

		/*
		* 铏氬嚱鏁扮敤浜庡垵濮嬪寲StateVector
		*/
		virtual void InitStateVector(int NumberOfNeurons, int GPUIndex);

		virtual Neuron_State_Vector* InitState();

		virtual void UpdateState(int index, int time, Simulation* simulatiuon);

		void CaculateDifferentialEquation(float* NeuronState, float* AuxNeuronState, int index);

		void CaculateTimeDependentEquation(float* NeuronState, int index, float dt);

		void CaculateSpike(float previous_V, float* NeuronState, int index);

		virtual InternalSpike* ProcessSpike(Interconnections* inter, int time);

		virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current);

		virtual void InitializeInputCurrentSynapseStructure();

		virtual void CheckType(Interconnections* inter);

		virtual int getV_index();

		virtual int get_NumberOfState();

		virtual enum NeuronModelType getNeuronModelType();

		void SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep);

		virtual std::map<std::string, boost::any> getParameters();

		virtual bool compare(NeuronModel* neuralmodel) {
			if (!TimeDrivenModel::compare(neuralmodel)) {
				return false;
			}
			TimeDrivenIzhikevic_I_Exponential_Decay* e = dynamic_cast<TimeDrivenIzhikevic_I_Exponential_Decay*>(neuralmodel);
			if (e == NULL) {
				return false;
			}
			bool whether = this->a == e->a && this->b == e->b && this->c == e->c && this->d == e->d &&
				this->V_rest == e->V_rest && this->V_reset == e->V_reset && this->V_th == e->V_th &&
				this->R == e->R && this->E_ampa == e->E_ampa && this->ampa_tau == e->ampa_tau &&
				this->E_gaba == e->E_gaba && this->gaba_tau == e->gaba_tau &&
				this->nmda_tau == e->nmda_tau && this->init == e->init && this->sigma == e->sigma;
			return whether;
		}
		
	private:
		std::vector<float> ampa_decay_lookup_;
		std::vector<float> gaba_decay_lookup_;
		std::vector<float> nmda_decay_lookup_;

		void ResetConductanceDecayLookup(float dt);
		void EnsureConductanceDecayLookupSize(float dt);
};



#endif // TIME_DRIVEN_IZHIKEVIC_I_EXPONENTIAL_DECAY_H
