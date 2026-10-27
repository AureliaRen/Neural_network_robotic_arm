/*
* 文件名：TimeDrivenLIF_Voltage_jump.h
* 定义了时间驱动LIF神经元模型，采用电压跳变连接模式,该类继承自TimeDrivenModel，是其派生的子类
*/

#ifndef TIME_DRIVEN_LIF_VOLTAGE_JUMP_H
#define TIME_DRIVEN_LIF_VOLTAGE_JUMP_H

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenModel.h"
#include "../source_file_realtime_v1_async/Current/inc/CurrentSynapse.h"
#include <array>

class TimeDrivenLIF_Voltage_jump : public TimeDrivenModel {
	public:
		//一些默认参数
		bool I_EXT = true; //默认接收外加电流

		bool Excited = false;

		bool Inhibitory = false;

		float V_rest = 0.; //榛樿闈欐伅鐢典綅

		float tau = 10.; //榛樿鏃堕棿甯告暟

		float V_th = 20.; //默认阈值

        float R = 1.; //榛樿杈撳叆鐢甸樆

		float V_reset = -5.; //榛樿閲嶇疆鐢典綅

		int t_ref = 50; //默认不应期(基准时间步倍数5ms)

		const int N_NeuronStateVariables = 2; //神经元状态变量个数：电压，外加电流

		const int N_DifferentialStates = 1; //微分方程个数：电压

		const int index_V = 0; //电压在状态变量中的索引(优先存储微分方程变量)

		const int TimeDependentInputSize = 1; //时间依赖输入个数:外加电流

        const int I_EXT_index = 1; //外加电流在状态变量中的索引

		CurrentSynapse* CurrentSynapeModel; //电流突触模型

		std::array<float, 2> init = { 0.0, 0.0 }; //初始化状态变量的偏差随机数

		std::array<float, 2> sigma = { 0.0, 0.0 }; //鍒濆鍖栫姸鎬佸彉閲忕殑鏂瑰樊

		/*
		* 构造函数
		*/
		TimeDrivenLIF_Voltage_jump();

		/*
		* 有参构造函数
		*/
		TimeDrivenLIF_Voltage_jump(int timestep);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~TimeDrivenLIF_Voltage_jump();

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
		virtual void CaculateDifferentialEquation(float* NeuronState, float* AuxNeuronState, int index);

		/*
		* 用于计算时间依赖项的变更
		* NeuronState: 神经元状态
		* index: 参数索引
		* dt: 时间步长
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
		* 设置模型参数
		*/
		void SetParameters(std::map<std::string, boost::any> parametermap, float basetimesteps);

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
			TimeDrivenLIF_Voltage_jump* e = dynamic_cast<TimeDrivenLIF_Voltage_jump*>(neuralmodel);
			if (e == NULL) {
				return false;
			}
			bool whether = this->V_rest == e->V_rest && this->tau == e->tau && this->V_th == e->V_th && this->R == e->R && this->V_reset == e->V_reset && this->t_ref == e->t_ref && this->init == e->init && this->sigma == e->sigma;
			whether = whether && this->integrationMethod->compare(e->integrationMethod);
			return whether;
		}

};


#endif
