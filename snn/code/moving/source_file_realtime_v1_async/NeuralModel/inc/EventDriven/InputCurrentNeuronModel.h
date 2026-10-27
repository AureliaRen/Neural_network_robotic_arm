/*
* 文件名：InputCurrentNeuronModel.h
* 定义了输入外界电流的神经元模型
*/
#ifndef INPUTCURRENTNEURONMODEL_H
#define INPUTCURRENTNEURONMODEL_H

#include "../source_file_realtime_v1_async/NeuralModel/inc/EventDriven/EventDrivenInputDevice.h"
#include <iostream>
#include <map>
class Neuron_State_Vector;

class InputCurrentNeuronModel : public EventDrivenInputDevice {
	public:
		/*
		* 构造函数
		*/
		InputCurrentNeuronModel();

		/*
		* 带参构造函数
		*/
		InputCurrentNeuronModel(int timesteps);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~InputCurrentNeuronModel();

		/*
		* 铏氬嚱鏁帮紝鍒濆鍖栫缁忓厓
		*/
		virtual Neuron_State_Vector* InitState() {
			return NULL;
		}

		/*
		* 虚函数，初始化状态向量
		*/
		virtual void InitStateVector(int NumberOfNeurons, int GPUIndex) {

		}

		/*
		* 虚函数，用于处理输入脉冲(输入设备不接收脉冲输入)
		*/
		virtual InternalSpike* ProcessSpike(Interconnections* inter, int time) {
			return NULL;
		}

		/*
		* 虚函数，用于处理输入电流（输入设备不接收电流输入）
		*/
		virtual void ProcessCurrent(Interconnections* inter, Neuron* Target, float current) {

		}

		/*
		* 虚函数，用于更新神经元模型（没有实际方法）
		*/
		virtual void UpdateState(int index, int time, Simulation* simulation) {

		}

		/*
		 * 虚函数，用于初始化电流传入（没有实际方法）
		*/
		virtual void InitializeInputCurrentSynapseStructure() {

		}

		/*
	    * 虚函数，具体实现方法在子类中，用于检查传入突触类型(没有实际方法)
	    */
		virtual void CheckType(Interconnections* inter) {

		}

		/*
		* 虚函数，用于返回V的索引
		*/
		virtual int getV_index() {
			return -1;
		}

		/*
		* 虚函数，用于返回每个神经元中变量的数目
		*/
		virtual int get_NumberOfState() {
			return -1;
		}

		/*
	    * 虚函数，返回神经元模型类型
	    */
		virtual enum NeuronModelType getNeuronModelType() {
			return INPUT_DEVICE;
		}

		virtual bool compare(NeuronModel* neuronModel) {
			if (!EventDrivenInputDevice::compare(neuronModel)) {
				return false;
			}
			InputCurrentNeuronModel* e = dynamic_cast<InputCurrentNeuronModel*>(neuronModel);
			if (e == NULL) {
				return false;
			}
			return true;
		};

		/*
		* 铏氬嚱鏁帮紝鐢ㄤ簬杩斿洖绁炵粡鍙傛暟鍒楄〃
		*/
		virtual std::map<std::string, boost::any> getParameters() {
			return std::map<std::string, boost::any>();
		}
};


#endif
