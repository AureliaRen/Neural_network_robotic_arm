/*
* ForwardEulerMethod.h
* 定义了前向欧拉积分方法
*/

#ifndef FORWARD_EULER_METHOD_H
#define FORWARD_EULER_METHOD_H
#include "../source_file_realtime_v1_async/Intergration/inc/FixStep/FixStepMethod.h"

template<typename NeuralModel>
class ForwardEulerMethod :public FixStepMethod<NeuralModel> {
	public:
		/*
		* 构造函数
		*/
		ForwardEulerMethod(NeuralModel* neuralmodel) :FixStepMethod<NeuralModel>(neuralmodel) {}

	    /*
		* 鏋愭瀯鍑芥暟
		*/
		~ForwardEulerMethod(){
		}

		/*
	    * 计算积分的增量
	    * index: 当前神经元索引
	    * NeuronStateVector: 神经元状态向量
	    */
		virtual void CaculateIncreament(Simulation* simulation, int currenttime){
			//创建辅助变量
			float AuxState[MAX_VARIABLES];
			for (int i = 0; i < this->neuralmodel->StateVector->NumberofNeuron; i++) {
				//获取神经元状态变量
				float* NeuronState = this->neuralmodel->StateVector->GetNeuronState(i);
				//记录之前的电压状态
				float previous_V = NeuronState[0];
				//计算微分方程右端项
				this->neuralmodel->CaculateDifferentialEquation(NeuronState, AuxState, i);
				//璁＄畻澧為噺
				for (int j = 0; j < this->neuralmodel->N_DifferentialStates; j++) {
					NeuronState[j] += this->neuralmodel->getTimestepSize() * AuxState[j] * this->dt;
				}
				//璁＄畻鐢靛
				this->neuralmodel->CaculateTimeDependentEquation(NeuronState, i, this->dt);
				//更新上一次放电
				this->neuralmodel->StateVector->LastSpike[i] += 1;
				//检查是否放电
				this->neuralmodel->CaculateSpike(previous_V, NeuronState, i);
				//累加膜电位
				this->IncrementValidIntegrationVariable(NeuronState[0]);
				this->neuralmodel->StateVector->LastUpdate[i] = currenttime;
			}
		}


		
		

		/*
		* 重置状态(此处无实际方法)
		*/
		virtual void ResetState(int index) {};

		/*
	    * 初始化状态
	    * NumberOfNeuron: 神经元数量
	    * intit_vector: 初始状态向量
	    */
		virtual void InitState(int NumberOfNeuron, float* intit_vector) {
			this->neuralmodel->InitState();
		}

		/*
	    * 鑾峰彇鍙傛暟
	    */
		virtual std::map<std::string, boost::any> getParameters() {
			std::map<std::string, boost::any> newMap = FixStepMethod<NeuralModel>::getParameters();
			newMap["name"] = getName();
			newMap["step"] = this->dt;
			return newMap;
		}

		/*
		* 璁剧疆鍙傛暟
		*/
		virtual void setParameters(std::map<std::string, boost::any> parameters) {
			FixStepMethod<NeuralModel>::setParameters(parameters);
		}

		/*
		* 鑾峰彇绉垎鏂规硶鍚嶇О
		*/
		static std::string getName() {
			return "ForwardEulerMethod";
		}

		/*
		* 鍒涘缓绉垎鏂规硶
		*/
		static IntegerationMethod* CreateIntegerationMethod(std::map<std::string, boost::any> NeuronParameter, NeuralModel* neuralmodel) {
			ForwardEulerMethod* newmodel = new ForwardEulerMethod(neuralmodel);
			newmodel->setParameters(NeuronParameter);
			return newmodel;
		}

		/*
	    * 瀵规瘮涓や釜绉垎妯″瀷鏄惁鐩稿悓
	    */
		virtual bool compare(IntegerationMethod* method) {
			if (!FixStepMethod<NeuralModel>::compare(method)) {
				return false;
			}
			ForwardEulerMethod* method2 = dynamic_cast<ForwardEulerMethod*>(method);
			if (method2 == NULL) {
				return false;
			}
			return true;
		}

};


#endif
