/*
* 定义了指数电导衰减模型
*/

#ifndef EXP_DECAY_H
#define EXP_DECAY_H
class NeuronModel;
#include <array>
#include "../source_file_realtime_v1_async/SynModel/inc/non_int/Non_int.h"

class Exp_decay: public Non_int{
public:

	//电压跳变模型涉及的参数个数
	int NumberOfJumpVariable = 1;

	//神经元数量
	int NumberOfNeuron;

	float g_tau = 12.0; //g的时间常数

	float* g; //鐢靛鍙橀噺

	float E = 0.0;//反转膜电位

	std::array<float, 1> init = {0.0}; //初始化状态变量的偏差随机数

	std::array<float, 1> sigma = {0.0}; //鍒濆鍖栫姸鎬佸彉閲忕殑鏂瑰樊

	/*
	* 构造函数
	*/
	Exp_decay(int timesteps);

	/*
	* 鏋愭瀯鍑芥暟
	*/
	~Exp_decay();

	/*
	* 更新模型参数
	*/
	virtual void UpdateSynState(float dt);

	/*
	* 获取电流
	*/
	virtual float GetCurrent(float* NeuronState, int index);

	/*
	* 初始化模型状态向量
	*/
	virtual void InitSynState(int NumberOfNeurons);

	/*
	* 处理Spike输入
	*/
	virtual void ProcessInput(Interconnections* inter, float* NeuronState, float weight);



	/*
	* 设置参数
	*/
	virtual std::map<std::string, boost::any> getParameters();

	/*
	* 鑾峰彇鍙傛暟
	*/
	virtual void setParameters(std::map<std::string, boost::any> parameters);

	/*
	* 比较函数，用于比较两个模型
	*/
	virtual bool compare(Synmodel* synmodel) {
		if (!Non_int::compare(synmodel)) {
			return false;
		}
		Exp_decay* e = dynamic_cast<Exp_decay*>(synmodel);
		if (e == NULL) {
			return false;
		}
		return this->g_tau == e->g_tau && this->init == e->init && this->sigma == e->sigma && this->E == e->E;

	}
};



#endif // !EXP_DECAY_H
