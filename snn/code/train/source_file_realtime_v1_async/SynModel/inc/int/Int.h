/*
* Int.h
* 定义突触模型的积分方法接口
*/
#ifndef INT_H
#define INT_H

#include "../source_file_realtime_v1_async/SynModel/inc/Synmodel.h"
class IntegerationMethod_Syn;
class Int : public Synmodel {
	public:

		//积分方法
		IntegerationMethod_Syn* integrationMethod;


		/*
		* 构造函数
		*/
		Int(int timestep);

		/*
		* 析构函数
		*/
        ~Int();

		/*
		* 处理输入
		*/
		virtual void ProcessInput(Interconnections* inter, float* NeuronState, float weight) = 0;

		/*
		* 更新突触状态
		*/
		virtual void UpdateSynState(float dt) = 0;

		/*
		* 计算微分方程
		*/
		virtual void CaculateDifferentialEquation(float* SynSate, float* AuxState, int index) = 0;

		/*
		* 获取电流
		*/
		virtual float GetCurrent(float* NeuronState, int index) = 0;

		/*
	   * 初始化突触状态
	   */
		virtual void InitSynState(int NumberOfNeurons) = 0;



		/*
		* 获取状态变量
		*/
		virtual float* GetState(int index) = 0;


		/*
		* 比较两个突触模型是否一致
		*/
		virtual bool compare(Synmodel* synmodel) {
			if (!Synmodel::compare(synmodel)) {
				return false;
			}
			Int* e = dynamic_cast<Int*>(synmodel);
			if (e == NULL) {
				return false;
			}
			return this->getTimestepSize() == e->getTimestepSize();

		}

		/*
		* 获取参数
		*/
		virtual std::map<std::string, boost::any> getParameters();

		/*
		* 设置参数
		*/
		virtual void setParameters(std::map<std::string, boost::any> parameters);

};

#endif
