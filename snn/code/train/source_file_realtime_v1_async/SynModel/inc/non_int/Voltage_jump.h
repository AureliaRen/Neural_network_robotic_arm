/*
* 文件名: Voltage_jump.h
* 定义了电压跳变模型
*/
#ifndef VOLTAGE_JUMP_H
#define VOLTAGE_JUMP_H

class NeuronModel;
#include "../source_file_realtime_v1_async/SynModel/inc/Non_int/Non_int.h"

class Voltage_jump : public Non_int {
public:

	//鐢靛帇璺冲彉妯″瀷娑夊強鐨勮烦鍙樺弬閲忕殑涓暟
	int NumberOfJumpVariable = 1;

	//瀵瑰簲鐨勭缁忓厓涓暟
	int NumberOfNeuron;

	

	/*
	* 构造函数
	*/
	Voltage_jump(int timestepsize);

	/*
	* 鏋愭瀯鍑芥暟
	*/
	~Voltage_jump();

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
	* 比较函数，用于比较两个模型是否相同
	*/
	virtual bool compare(Synmodel* synmodel) {
		if (!Non_int::compare(synmodel)) {
			return false;
		}
		Voltage_jump* e = dynamic_cast<Voltage_jump*>(synmodel);
		if (e == NULL) {
			return false;
		}
		return true;

	}

};



#endif // !VOLTAGE_JUMP_H
