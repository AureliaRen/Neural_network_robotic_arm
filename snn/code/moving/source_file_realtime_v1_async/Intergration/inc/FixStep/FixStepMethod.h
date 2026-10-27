/*
* FixStepMethod.h，固定步长积分方法父类
*/

#ifndef FIXSTEPMETHOD_H
#define FIXSTEPMETHOD_H

#include "../source_file_realtime_v1_async/Intergration/inc/IntegerationMethodTemplate.h"
template<typename NeuralModel>
class FixStepMethod :public IntegerationMethodTemplate<NeuralModel> {
public:
	/*
	* 构造函数
	*/
	FixStepMethod(NeuralModel* model) :IntegerationMethodTemplate<NeuralModel>(model) {};

	/*
	* 鏋愭瀯鍑芥暟
	*/
	virtual ~FixStepMethod() {};

	/*
	* 计算积分的增量
	* index: 当前神经元索引
	* NeuronStateVector: 神经元状态向量
	*/
	virtual void CaculateIncreament(Simulation* simulation, int currenttime) = 0;


	/*
	* 初始化状态
	* NumberOfNeuron: 神经元数量
	* intit_vector: 初始状态向量
	*/
	virtual void InitState(int NumberOfNeuron, float* intit_vector) = 0;

	/*
	* 重置积分状态
	* index: 当前神经元索引
	*/
	virtual void ResetState(int index) = 0;

	/*
	* 鑾峰彇鍙傛暟
	*/
	virtual std::map<std::string, boost::any> getParameters() {
		std::map<std::string, boost::any> parameters = IntegerationMethodTemplate<NeuralModel>::getParameters();
		return parameters;
	}

	/*
	* 璁剧疆鍙傛暟
	* parameters: 鍙傛暟瀛楀吀
	*/
	virtual void setParameters(std::map<std::string, boost::any> parameters) {
		IntegerationMethodTemplate<NeuralModel>::setParameters(parameters);
	}

	/*
	* 瀵规瘮涓や釜绉垎妯″瀷鏄惁鐩稿悓
	*/
	virtual bool compare(IntegerationMethod* method) {
		if (!IntegerationMethodTemplate<NeuralModel>::compare(method)) {
			return false;
		}
		FixStepMethod* method1 = dynamic_cast<FixStepMethod*>(method);
		if (method1 == nullptr) {
			return false;
		}
		else {
			return true;
		}
	}


};


#endif
