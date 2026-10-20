/*
* 文件名：TimeDrivenLIF_Exponential_double_GPU_Interface.cuh
* 定义了事件驱动 LIF 神经元模型的 GPU 接口，用于与 GPU 上的对应类实现交互
*/
#ifndef TIME_DRIVEN_LIF_EXPONENTIAL_DOUBLE_GPU_CUH
#define TIME_DRIVEN_LIF_EXPONENTIAL_DOUBLE_GPU_CUH
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenNeuronModelGPU_Interface.cuh"
class CurrentSynapse;
class TimeDrivenLIF_Exponential_double_GPU;
#include <array>

class TimeDrivenLIF_Exponential_double_GPU_Interface : public TimeDrivenNeuronModelGPU_Interface {
	public:
		

		//一些默认参数
		bool I_EXT = false;

		bool Excited = false;

		bool Inhibitory = false;

		float V_rest = -60.; //默认静息电位

		float tau = 20.; //默认时间常数

		float V_th = -50.; //默认阈值

		float R = 1.; //默认输入电阻

		float V_reset = -60.; //默认重置电位

		int t_ref = 50; //默认不应期(基准时间步倍数5ms)

		float gexc_tau = 5.; //兴奋性突触门控时间常数

		float Eexc = 0.; //兴奋性反转膜电位

		float ginh_tau = 10.; //抑制性突触门控时间常数

		float Einhibitory = -80.; //抑制性反转膜电位

		const int N_NeuronStateVariables = 4; //神经元状态变量个数：电压，兴奋门控，抑制门控，外加电流

		const int N_TimedependentInput = 3; //时间依赖输入个数:外加电流,兴奋电导，抑制电导

		const int N_DifferentialStates = 1; //微分方程个数：电压

		const int index_V = 0; //电压在状态变量中的索引(优先存储微分方程变量)

		const int index_gexc = 1; //兴奋门控在状态变量中的索引

		const int index_ginh = 2; //抑制门控在状态变量中的索引

		const int TimeDependentInputSize = 3; //时间依赖输入个数:外加电流,兴奋电导，抑制电导

		const int I_EXT_index = 3; //外加电流在状态变量中的索引

		CurrentSynapse* CurrentSynapeModel; //电流突触模型

		TimeDrivenLIF_Exponential_double_GPU** NeuronModelOnGPU; //GPU 上的神经元模型

		float timestepdouble;

		//double* AuxNeuronState; //辅助变量，用于存储神经元状态（放在 GPU 上）

		std::array<float, 4> init = { 0.0, 0.0, 0.0, 0.0 }; //初始状态变量的偏置均值

		std::array<float, 4> sigma = { 0.0, 0.0, 0.0, 0.0 }; //初始状态变量的方差


		/*
		* 构造函数
		*/
		TimeDrivenLIF_Exponential_double_GPU_Interface(int timestep);

		/*
		* 析构函数
		*/
        ~TimeDrivenLIF_Exponential_double_GPU_Interface();

		/*
		* 销毁 GPU 上的神经元模型
		*/
		virtual void DestroyGPUModel();

		/*
		* 检查传入突触类型
		*/
		virtual void CheckType(Interconnections* inter);

		/*
		* 初始化状态（目前没有实际功能）
		*/
		virtual Neuron_State_Vector* InitState();

		/*
		* 处理输入脉冲
		* @inter: 突触连接
		*/
		virtual InternalSpike* ProcessSpike(Interconnections* inter, int time);


		/*
		* 处理电流输入
		* inter: 突触连接
		* Target: 目标神经元
		* current: 电流
		*/
		virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current);

		/*
	    * 虚函数，用于更新神经元模型的状态，返回一个 bool 值，表示是否产生了新的脉冲
	    * index: 要更新的神经元索引
	    * time: 当前时间
		* simulation: 模拟对象
	    */
		virtual void UpdateState(int index, int time, Simulation* simulation);

		/*
	    * 初始化 StateVector
	    * NumberOfNeurons: 神经元数量
	    * GPUIndex: GPU 编号
	    */
		virtual void InitStateVector(int NumberOfNeurons, int GPUindex);

		/*
		* 初始化接口对应的 GPU 对象
		* N_neurons: 神经元数量
		*/
		virtual void InitializeClassGPU2(int N_neurons);

		/*
		* 初始化 GPU 上的神经元状态
		*/
		virtual void InitializeVectorNeuronState_GPU2();

		/*
	    * 初始化输入电流结构
	    */
		virtual void InitializeInputCurrentSynapseStructure();

		/*
		* 比较两个神经元模型
		*/
		virtual bool compare(NeuronModel* neuronmodel) {
			//调用父类的比较函数
			if (!TimeDrivenNeuronModelGPU_Interface::compare(neuronmodel)) {
				return false;
			}
			//判断是否为同类模型
			TimeDrivenLIF_Exponential_double_GPU_Interface* e = dynamic_cast<TimeDrivenLIF_Exponential_double_GPU_Interface*>(neuronmodel);
			if (e == NULL) {
				return false;
			}
			bool whether = this->I_EXT == e->I_EXT && this->Excited == e->Excited && this->Inhibitory == e->Inhibitory && this->V_rest == e->V_rest && this->tau == e->tau && this->V_th == e->V_th && this->R == e->R && this->V_reset == e->V_reset && this->t_ref == e->t_ref && this->gexc_tau == e->gexc_tau && this->Eexc == e->Eexc && this->ginh_tau == e->ginh_tau && this->Einhibitory == e->Einhibitory && this->init == e->init && this->sigma == e->sigma && this->integrationMethodGPUInterface == e->integrationMethodGPUInterface;
			return whether;
		}

		/*
		* 纯虚函数，用于设置神经元模型参数
		* parametermap: 参数字典
		*/
		void SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep);

		/*
		* 虚函数，用于获取神经元模型参数
		*/
		virtual std::map<std::string, boost::any> getParameters();

		/*
		* 返回 V 的索引
		*/
		virtual int getV_index() {
			return this->index_V;
		};

		/*
		* 返回每个神经元包含的状态变量数目
		*/
		virtual int get_NumberOfState() {
			return this->N_NeuronStateVariables;
		};

		/*
		* 返回神经元模型类型
		*/
		virtual enum NeuronModelType getNeuronModelType() {
			return NEURAL_LAYER;
		}

};


#endif
