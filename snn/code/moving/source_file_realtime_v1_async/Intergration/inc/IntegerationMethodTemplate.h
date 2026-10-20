/*
* 定义了一个模版类，用于处理不同神经元模型的通用积分方法
*/

#ifndef INTEGERATIONMETHODTEMPLATE_H
#define INTEGERATIONMETHODTEMPLATE_H

#include "../source_file_realtime_v1_async/Intergration/inc/IntegerationMethod.h"

template <typename NeuralModel>
class IntegerationMethodTemplate : public IntegerationMethod {
public:
	NeuralModel* neuralmodel;

	/*
	* 构造函数
	*/
	IntegerationMethodTemplate(NeuralModel* neuralmodel) :IntegerationMethod(), neuralmodel(neuralmodel) {};

	/*
	* 鏋愭瀯鍑芥暟
	*/
	virtual ~IntegerationMethodTemplate() {};

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
		std::map<std::string, boost::any> parameters = IntegerationMethod::getParameters();
		return parameters;
	}

	/*
	* 璁剧疆鍙傛暟
	*/
	virtual void setParameters(std::map<std::string, boost::any> parameters) {
		IntegerationMethod::setParameters(parameters);
	}

	/*
	* 瀵规瘮涓や釜绉垎妯″瀷鏄惁鐩稿悓
	*/
	virtual bool compare(IntegerationMethod* method) {
		if (!IntegerationMethod::compare(method)) {
			return false;
		}
		IntegerationMethodTemplate* method1 = dynamic_cast<IntegerationMethodTemplate*>(method);
		if (method1 == nullptr) {
			return false;
		}
		else {
			return true;
		}
	}


};

#endif // INTEGERATIONMETHODTEMPLATE_H
