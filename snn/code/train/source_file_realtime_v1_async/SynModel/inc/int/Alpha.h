/*
* Alpha.h
* 定义 Alpha 电导
*/

#ifndef ALPHA_H
#define ALPHA_H

class NeuronModel;
#include "../source_file_realtime_v1_async/SynModel/inc/int/Int.h"
#include <array>
class Alpha : public Int {
public:
	//突触跳变变量个数
	int NumberOfJumpVariable = 1;

	//神经元个数
	int NumberOfNeuron;

	const int N_DifferentialStates = 1; //微分状态变量个数


	float* g_h; //g 和 h 状态变量

	float g_tau = 20.0; //g 的时间常数

	float h_tau = 2.0; //h 的时间常数

	float E = 0.0; //反转电位

	std::array<float, 2> init = { 0.0, 0.0}; //状态变量的初始均值

	std::array<float, 2> sigma = { 0.0, 0.0}; //状态变量的初始方差

	/*
	* 构造函数
	*/
	Alpha(int timestepsize);

	/*
	* 析构函数
	*/
    virtual ~Alpha();

	/*
	* 更新突触状态
	*/
	virtual void UpdateSynState(float dt);

	/*
	* 计算微分方程
	*/
	virtual void CaculateDifferentialEquation(float* SynSate, float* AuxState, int i);

	/*
	* 获取状态变量
	*/
	virtual float* GetState(int index);


	/*
		* 获取电流
		*/
	virtual float GetCurrent(float* NeuronState, int index);

	/*
	 * 初始化突触状态
	 */
	virtual void InitSynState(int NumberOfNeurons);

	/*
	* 处理 Spike 输入
	*/
	virtual void ProcessInput(Interconnections* inter, float* NeuronState, float weight);


	/*
		* 获取参数
		*/
	virtual std::map<std::string, boost::any> getParameters();

	/*
	* 设置参数
	*/
	virtual void setParameters(std::map<std::string, boost::any> parameters);

	/*
	* 比较两个突触模型是否一致
	*/
	virtual bool compare(Synmodel* synmodel) {
		if (!Int::compare(synmodel)) {
			return false;
		}
		Alpha* e = dynamic_cast<Alpha*>(synmodel);
		if (e == NULL) {
			return false;
		}
		return this->g_tau == e->g_tau && this->h_tau == e->h_tau && this->init == e->init && this->sigma == e->sigma && this->E == e->E;

	}
};


#endif
