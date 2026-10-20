/*
* 文件名：NeuronModelPropogationStructure.h
* 定义了NeuronModel当中所使用的传播结构类，按照延迟的大小进行分类
*/



#ifndef NEURONMODELPROPAGATIONSTRUCTURE_H
#define NEURONMODELPROPAGATIONSTRUCTURE_H

class NeuronModelPropogationStructure {
	public:
		int* NumberOfDelays; //每个目标线程上面的不同延迟个数

		int* AllocatedSize; //每个目标线程上面分配的内存大小

		int** SynapseDelay; //每个目标线程上面的不同延迟的数组


		//构造函数
        NeuronModelPropogationStructure();

		//析构函数
		~NeuronModelPropogationStructure();

		/*
		* 在目标队列上添加一个延迟，并将延迟按照升序排列
		* Queue_index 目标队列的索引
		* Delay 延迟
		*/
		void IncludeNewDelay(int Queue_index, int Delay);




};




#endif // NEURONMODELPROPAGATIONSTRUCTURE_H
