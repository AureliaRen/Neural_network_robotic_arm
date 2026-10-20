/*
* 文件名：TimeDrivenModel.h
* 用于定义时间驱动模型，该类将继承NeuronModel类,主要将包含时间驱动的神经元模型
*/

#ifndef TIME_DRIVEN_MODEL_H
#define TIME_DRIVEN_MODEL_H

#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/InternalSpike.h"
#include "../source_file_realtime_v1_async/Intergration/inc/IntegerationMethod.h"

class TimeDrivenModel : public NeuronModel {

public:

	//绉垎鏂规硶鍙橀噺
	IntegerationMethod* integrationMethod;

	/*
	* 默认的构造函数
	*/
    TimeDrivenModel();

	/*
	* 带参数的构造函数
	*/
	TimeDrivenModel(int timestep);
	

	/*
	* 鏋愭瀯鍑芥暟
	*/
	virtual ~TimeDrivenModel();


	/*
	* 绾櫄鍑芥暟锛岀敤浜庡鐞嗕紶鍏ョ殑鑴夊啿浜嬩欢锛岃繑鍥炰竴涓狪nternalSpike鎸囬拡瀵硅薄鍗虫鏌ヤ紶鍏ユ槸鍚︿骇鐢熶簡鏂扮殑鑴夊啿
	*/
    virtual InternalSpike* ProcessSpike(Interconnections* inter, int time) = 0;

	/*
	* 纯虚函数，用于处理输入的电流事件，返回一个InternalSpike指针对象即检查传入是否产生了新的脉冲
	* inter: 连接对象
	* neuron: 神经元对象
	* current: 电流值
	*/
	virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current) = 0;

	/*
	* 纯虚函数，用于更新神经元模型的状态，返回一个bool值，表示是否产生了新的脉冲
	* index: 要更新的神经元索引
	* time: 当前时间
	*/
	virtual void UpdateState(int index, int time, Simulation* simulation) = 0;

	/*
	* 绾櫄鍑芥暟锛屽叿浣撳疄鐜版柟娉曞湪瀛愮被涓紝鐢ㄤ簬鍒濆鍖朣tateVector
    * NumberOfNeurons:神经元数量
	* GPUIndex:GPU绱㈠紩
	*/
	virtual void InitStateVector(int NumberOfNeurons, int GPUIndex) = 0;

	/*
	* 纯虚函数，具体实现方法在子类中，用于计算微分方程
	*/
	virtual void CaculateDifferentialEquation(float* NeuronState, float* AuxNeuronState, float dt) {};


	/*
	* 纯虚函数，具体实现方法在子类中，用于初始化神经元模型
	*/
	virtual Neuron_State_Vector* InitState() = 0;

	/*
	* 纯虚函数，具体实现方法在子类中，用于初始化电流传入
	*/
	virtual void InitializeInputCurrentSynapseStructure() = 0;

	/*
	* 纯虚函数，具体实现方法在子类中，用于检查传入突触类型
	*/
	virtual void CheckType(Interconnections* inter) = 0;

	/*
		* 虚函数，用于返回V的索引
		*/
	virtual int getV_index() = 0;

	/*
	* 虚函数，用于返回每个神经元中变量的数目
	*/
	virtual int get_NumberOfState() = 0;

	/*
	* 绾櫄鍑芥暟锛岃繑鍥炵缁忓厓妯″瀷绫诲瀷
	*/
	virtual enum NeuronModelType getNeuronModelType() = 0;


	/*
	* 检查积分过程的有效性
	*/
	void CheckValidIntegeration(int currenttimestep, float valid_integeration);

	/*
	* 比较函数，用于比较两个神经元模型是否相同
	*/
	virtual bool compare(NeuronModel* neuronmodel) {
		//首先调用父类的函数
		if (!NeuronModel::compare(neuronmodel)) {
			return false;
		}

		TimeDrivenModel* e = dynamic_cast<TimeDrivenModel*>(neuronmodel);
		if (e == NULL) {
			return false;
		}

		return this->getTimestepSize() == e->getTimestepSize();
	};

	/*
	* 铏氬嚱鏁帮紝鑾峰彇妯″瀷鍙傛暟
	*/
	virtual std::map<std::string, boost::any> getParameters() = 0;


};




#endif
