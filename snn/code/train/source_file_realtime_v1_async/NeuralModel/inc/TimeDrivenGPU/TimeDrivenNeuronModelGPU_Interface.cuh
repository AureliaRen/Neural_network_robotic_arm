/*
* 文件名：TimeDrivenNeuronModelGPU_Interface.cuh
* 该类定义了时间驱动神经元模型在GPU上的接口基类，集成了不同类型的时间驱动神经元模型，该类继承自TimeDrivenNeuronModel
*/

#ifndef TIME_DRIVEN_NEURON_MODEL_GPU_INTERFACE_CUH
#define TIME_DRIVEN_NEURON_MODEL_GPU_INTERFACE_CUH

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenModel.h"
#include <cuda_runtime.h>
class Neuron_State_Vector_Interface;
class IntegrationMethodGPU_Interface;


class TimeDrivenNeuronModelGPU_Interface : public TimeDrivenModel {
	public:

		//grid大小
		int gridsize;

		//block大小
		int blocksize;

		//StateVector 的别名视图，不拥有底层状态对象
		Neuron_State_Vector_Interface* State_GPU;

		cudaStream_t copyStream;  // 专用于数据拷贝

		cudaStream_t computeStream; // 专用于核函数计算

		//定义一个同步cuda事件
        cudaEvent_t sync_event;

		//定义GPU的设备信息
		cudaDeviceProp deviceProp;

		//定义GPU的设备ID
		int GPU_ID;

		//GPU上的积分方法
		IntegrationMethodGPU_Interface* integrationMethodGPUInterface;

		/*
		* 构造函数
		* @timestep:时间步长
		*/
		TimeDrivenNeuronModelGPU_Interface(int timestep);

		/*
		* 析构函数
		*/
		~TimeDrivenNeuronModelGPU_Interface();

		/*
		* 纯虚函数，具体实现方法在子类中，用于析构掉GPU上的神经元模型
		*/
		virtual void DestroyGPUModel() = 0;

		/*
	    * 纯虚函数，具体实现方法在子类中，用于初始化神经元模型
	    */
		virtual Neuron_State_Vector* InitState() = 0;

		/*
	    * 纯虚函数，用于处理传入的脉冲事件，返回一个InternalSpike指针对象即检查传入是否产生了新的脉冲
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
		* 纯虚函数，具体实现方法在子类中，用于初始化StateVector
		* NumberOfNeurons:神经元数量
		* openmp:openmp线程数量
		*/
		virtual void InitStateVector(int NumberOfNeurons, int GPUIndex) = 0;

		/*
	    * 纯虚函数，具体实现方法在子类中，用于初始化电流传入
	    */
		virtual void InitializeInputCurrentSynapseStructure() = 0;

		/*
		* 纯虚函数，具体实现方法在子类中，用于初始化接口对应的GPU对象
		* N_neurons:神经元数量
		*/
		virtual void InitializeClassGPU2(int N_neurons) = 0;

		/*
		* 纯虚函数，具体实现方法在子类中，用于初始化GPU上的神经元状态
		*/
		virtual void InitializeVectorNeuronState_GPU2() = 0;

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
		* 纯虚函数，返回神经元模型类型
		*/
		virtual enum NeuronModelType getNeuronModelType() = 0;

		/*
		* 定义比较函数，用于比较两个神经元模型
		*/
		virtual bool compare(NeuronModel* neuronmodel) {
			//调用父类的比较函数
			if (!TimeDrivenModel::compare(neuronmodel)) {
				return false;
			}
			//检查是否为TimeDrivenNeuronModelGPU_Interface类型
			TimeDrivenNeuronModelGPU_Interface* e = dynamic_cast<TimeDrivenNeuronModelGPU_Interface*> (neuronmodel);
			if (e == 0) {
				return false;
			}
			return true;
		}

};




#endif // TIME_DRIVEN_NEURON_MODEL_GPU_INTERFACE_CUH