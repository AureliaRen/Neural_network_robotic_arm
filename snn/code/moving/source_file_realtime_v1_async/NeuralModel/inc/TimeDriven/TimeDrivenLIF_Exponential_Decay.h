/*
* 文件名：TimeDrivenLIF_Exponential_Decay.h
* 定义了时间驱动的LIF神经元模型，该模型具有指数衰减的突触模型，继承自TimeDrivenModel类。
* 作者：李梓沛
*/

#ifndef TIME_DRIVEN_LIF_EXPONENTIAL_DECAY_H
#define TIME_DRIVEN_LIF_EXPONENTIAL_DECAY_H

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenModel.h"
#include <array>
class CurrentSynapse;

class TimeDrivenLIF_Exponential_Decay : public TimeDrivenModel {
	public:

		//一些默认参数
		bool I_EXT = true;

		bool Excited = false;

		bool Inhibitory = false;

		float V_rest = -65.; //榛樿闈欐伅鐢典綅

		float tau = 20.; //榛樿鏃堕棿甯告暟

		float V_th = -20.; //默认阈值

		float R = 1.; //榛樿杈撳叆鐢甸樆

		float V_reset = -65.; //榛樿閲嶇疆鐢典綅

		int t_ref = 50; //默认不应期(基准时间步倍数5ms)

		float g_tau = 12.0; //榛樿绐佽Е闂ㄦ帶鏃堕棿甯告暟

		float E = 0.; //默认反转膜电位

		const int N_NeuronStateVariables = 3; //神经元状态变量个数：电压，门控，外加电流

		const int N_DifferentialStates = 1; //微分方程个数：电压

		const int index_V = 0; //电压在状态变量中的索引(优先存储微分方程变量)

        const int index_g = 1; //门控在状态变量中的索引

		const int TimeDependentInputSize = 2; //时间依赖输入个数:外加电流, 电导

		const int I_EXT_index = 2; //外加电流在状态变量中的索引

		CurrentSynapse* CurrentSynapeModel; //电流突触模型

		std::array<float, 3> init = { 0.0, 0.0, 0.0 }; //初始化状态变量的偏差随机数

		std::array<float, 3> sigma = { 0.0, 0.0, 0.0 }; //鍒濆鍖栫姸鎬佸彉閲忕殑鏂瑰樊

		/*
		* 构造函数
		*/
		TimeDrivenLIF_Exponential_Decay();

		/*
		* 带参数的构造函数
		*/
		TimeDrivenLIF_Exponential_Decay(int timesteps);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~TimeDrivenLIF_Exponential_Decay();

		/*
		* 铏氬嚱鏁扮敤浜庡垵濮嬪寲StateVector
		* NumberOfNeurons:神经元数量
		* GPUIndex:GPU绱㈠紩
		*/
		virtual void InitStateVector(int NumberOfNeurons, int GPUIndex);

		/*
		* 铏氬嚱鏁帮紝鐢ㄤ簬鍒濆鍖栫缁忓厓妯″瀷鍚庤繑鍥濻tateVector
		*/
		virtual Neuron_State_Vector* InitState();

		/*
		* 用于更新第index个神经元的状态
		* index:绁炵粡鍏冨湪StateVector绱㈠紩
		* time:褰撳墠鏃堕棿
		*/
		virtual void UpdateState(int index, int time, Simulation* simulatiuon);

		/*
		* 用于计算微分方程的右端项
		* NeuronState: 神经元状态
		* AuxNeuronState: 中间神经元状态
		* index: 参数索引
		*/
		void CaculateDifferentialEquation(float* NeuronState, float* AuxNeuronState, int index);

		/*
		* 用于计算时间依赖项的变更
		* NeuronState: 神经元状态
		* index: 参数索引
		*/
		void CaculateTimeDependentEquation(float* NeuronState, int index, float dt);


		/*
		* 用于计算神经元发放脉冲
		* previous_V: 上一时刻电压
		* NeuronState: 神经元状态
		* index: 参数索引
		*/
		void CaculateSpike(float previous_V, float* NeuronState, int index);

		/*
		* 虚函数，用于处理输入的Spike事件
		* inter: 连接
		* time: 传入时间
		* return: 返回一个InternalSpike对象（在该模型中并没有使用这一返回值）
		*/
		virtual InternalSpike* ProcessSpike(Interconnections* inter, int time);

		/*
		* 虚函数，用于处理输入的Current事件
		* inter: 连接
		* time: 传入时间
		* return: 返回一个InternalCurrent对象（在该模型中并没有使用这一返回值）
		*/
		virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current);

		/*
		 * 虚函数，用于初始化电流传入
		*/
		virtual void InitializeInputCurrentSynapseStructure();

		/*
		* 虚函数，用于生成电流的传入结构与兴奋或抑制性标识
		* inter: 杩炴帴
		* 如果连接是兴奋性的则将 Excited 设为 true，如果是抑制性的则将 Inhibitory 设为 true
		*/
		virtual void CheckType(Interconnections* inter);

		/*
		* 虚函数，用于返回V的索引
		*/
		virtual int getV_index();

		/*
		* 虚函数，用于返回每个神经元中变量的数目
		*/
		virtual int get_NumberOfState();

		/*
	    * 绾櫄鍑芥暟锛岃繑鍥炵缁忓厓妯″瀷绫诲瀷
	    */
		virtual enum NeuronModelType getNeuronModelType();

		/*
	    * 虚函数，用于设置神经元模型参数
	    * parametermap:参数字典
	    */
		void SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep);

		/*
		* 获取模型参数
		*/
		virtual std::map<std::string, boost::any> getParameters();

		/*
	    * 定义一个比较虚函数，用于比较两个神经元模型
	    */
		virtual bool compare(NeuronModel* neuralmodel) {
			//调用父类的比较函数
			if (!TimeDrivenModel::compare(neuralmodel)) {
				return false;
			}
			//判断是否为同类模型
			TimeDrivenLIF_Exponential_Decay* e = dynamic_cast<TimeDrivenLIF_Exponential_Decay*>(neuralmodel);
			if (e == NULL) {
				return false;
			}
			bool whether = this->V_rest == e->V_rest && this->tau == e->tau && this->V_th == e->V_th && this->R == e->R && this->V_reset == e->V_reset && this->t_ref == e->t_ref && this->g_tau == e->g_tau && this->E == e->E  && this->init == e->init && this->sigma == e->sigma;
			return whether;
		}

		private:
			std::vector<float> g_decay_lookup_;

			void ResetConductanceDecayLookup(float dt);
			void EnsureConductanceDecayLookupSize(float dt);


};



#endif // !TIME_DRIVEN_LIF_EXPONENTIAL_DECAY_H
