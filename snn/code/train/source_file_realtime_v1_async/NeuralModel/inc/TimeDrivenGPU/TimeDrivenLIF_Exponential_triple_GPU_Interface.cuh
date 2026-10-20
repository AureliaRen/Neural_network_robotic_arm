/*

* 文件名:TimeDrivenLIF_Exponential_double_GPU_Interface.cuh

* 定义 LIF 神经元模型对应的 GPU 接口，并管理对应的 GPU 端实现对象

*/

#ifndef TIME_DRIVEN_LIF_EXPONENTIAL_TRIPLE_GPU_INTERFACE_CUH

#define TIME_DRIVEN_LIF_EXPONENTIAL_TRIPLE_GPU_INTERFACE_CUH



#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenNeuronModelGPU_Interface.cuh"

class CurrentSynapse;

class TimeDrivenLIF_Exponential_triple_GPU;

#include <array>



class TimeDrivenLIF_Exponential_triple_GPU_Interface : public TimeDrivenNeuronModelGPU_Interface {

public:



	bool I_EXT = true; 



	bool AMPA = false;



	bool NMDA = false;



	bool GABA = false;



	float V_rest = -60.; 



	float tau = 10.; 



	float V_th = -50.; 



	float R = 1.; 



	float V_reset = -60.; 



	int t_ref = 50; 



	float E_ampa = 0.; 



	float ampa_tau = 10.; 



	float E_gaba = -80.; 



	float gaba_tau = 10.; 



	float nmda_tau = 10.; 



	const int N_NeuronStateVariables = 5; 



	const int N_TimedependentInput = 4; //ampa, gaba, nmda, I_ext



	const int N_DifferentialStates = 1; 



	const int index_V = 0; 



	const int TimeDependentInputSize = 4; 



	const int index_ampa = 1; 



	const int index_nmda = 3; 



	const int index_gaba = 2; 



	const int I_EXT_index = 4; 



	CurrentSynapse* CurrentSynapeModel; 



	TimeDrivenLIF_Exponential_triple_GPU** NeuronModelOnGPU; //GPU 上的神经元模型



	float timestepdouble;



	std::array<float, 5> init = { 0.0, 0.0, 0.0, 0.0, 0.0 }; 



	std::array<float, 5> sigma = { 0.0, 0.0, 0.0, 0.0, 0.0 }; 



	/*

	* 构造函数

	*/

	TimeDrivenLIF_Exponential_triple_GPU_Interface(int timestep);



	/*

	* 析构函数

	*/

	~TimeDrivenLIF_Exponential_triple_GPU_Interface();



	/*

	* 销毁 GPU 端的神经元模型

	*/

	virtual void DestroyGPUModel();



	/*

	* 检查突触连接类型

	*/

	virtual void CheckType(Interconnections* inter);



	/*

	* 初始化状态变量（目前没有实际使用）

	*/

	virtual Neuron_State_Vector* InitState();



	/*

	* 处理输入脉冲

	* @inter: 突触连接

	*/

	virtual InternalSpike* ProcessSpike(Interconnections* inter, int time);



	/*

	* 处理输入电流

	* inter: 突触连接

	* Target: 目标神经元

	* current: 输入电流

	*/

	virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current);



	/*

	* 更新指定神经元模型的状态，并根据结果决定是否产生新的事件

	* index: 要更新的神经元下标

	* time: 当前时间

	* simulation: 仿真对象

	*/

	virtual void UpdateState(int index, int time, Simulation* simulation);



	/*

	* 初始化 StateVector

	* NumberOfNeurons: 神经元数量

	* GPUIndex: GPU 编号

	*/

	virtual void InitStateVector(int NumberOfNeurons, int GPUindex);



	/*

	* 初始化接口对应的 GPU 模型对象

	* N_neurons: 神经元数量

	*/

	virtual void InitializeClassGPU2(int N_neurons);





	/*

	* 初始化 GPU 上的神经元状态

	*/

	virtual void InitializeVectorNeuronState_GPU2();



	/*

	* 初始化输入电流突触结构

	*/

	virtual void InitializeInputCurrentSynapseStructure();



	/*

	* 比较两个神经元模型是否一致

	*/

	virtual bool compare(NeuronModel* neuronmodel) {

		// 先比较基类中的公共部分

		if (!TimeDrivenNeuronModelGPU_Interface::compare(neuronmodel)) {

			return false;

		}

		// 判断是否为同一种模型

		TimeDrivenLIF_Exponential_triple_GPU_Interface* e = dynamic_cast<TimeDrivenLIF_Exponential_triple_GPU_Interface*>(neuronmodel);

		if (e == NULL) {

			return false;

		}

		bool whether = this->V_rest == e->V_rest && this->tau == e->tau && this->V_th == e->V_th && this->R == e->R && this->V_reset == e->V_reset && this->t_ref == e->t_ref && this->E_ampa == e->E_ampa && this->ampa_tau == e->ampa_tau && this->E_gaba == e->E_gaba && this->gaba_tau == e->gaba_tau && this->nmda_tau == e->nmda_tau && this->init == e->init && this->sigma == e->sigma && this->integrationMethodGPUInterface == e->integrationMethodGPUInterface;

		return whether;

	}



	/*

		* 设置神经元模型参数

		* parametermap: 参数字典

		*/

	void SetParameters(std::map<std::string, boost::any> parametermap, float basetimestep);



	/*

	* 获取神经元模型参数

	*/

	virtual std::map<std::string, boost::any> getParameters();



	/*

	* 获取膜电位 V 在状态向量中的下标

	*/

	virtual int getV_index() {

		return this->index_V;

	};



	/*

	* 获取每个神经元包含的状态变量数目

	*/

	virtual int get_NumberOfState() {

		return this->N_NeuronStateVariables;

	};



	/*

	* 获取神经元模型类型

	*/

	virtual enum NeuronModelType getNeuronModelType() {

		return NEURAL_LAYER;

	}



};





#endif

