/*
* 文件名：NetworkConstructStructure.h
* 定义了两个结构体，用于描述神经元层与连接关系，用于构造网络
*/
#ifndef NETWORKCONSTRUCTSTRUCTURE_H
#define NETWORKCONSTRUCTSTRUCTURE_H
#include <string>
#include <vector>
#include <map>
#include <boost/any.hpp>

/*
* 通用模型描述结构体
*/
struct ModelDescription {
	std::string ModelName;

	std::map<std::string, boost::any> ModelParameter;
};


/*
* 神经元层结构体
*/
struct NeuronLayerDescription
{
    //神经元层的神经元数量
	int numberofneuron;
	//神经元层的神经元类型
	std::string ModelName;
	//神经元是否被监视
	bool isMonitored = false;
	//神经元是否是用于输出的
	bool isOutput = false;
	//神经元层是否是communication输入层
	bool isCommunicationInput = false;
	//神经元参数列表
	std::map<std::string, boost::any> NeuronParameter;

};

/*
* 连接关系结构体
*/
struct ConnectionDescription {
	//源神经元编号向量
	std::vector<int> SourceNeuron;
	//目标神经元编号向量
    std::vector<int> TargetNeuron;
	//连接类别向量
	std::vector<int> Type;
	//连接权重向量
	std::vector<float> Weight;
	//最大连接权重
	std::vector<float> MaxWeight;
	//连接延迟
	std::vector<int> Delay;
	//突触规则
	std::vector<int> SynapseRule;
	//触发学习规则
	std::vector<int> TriggerRule;
	//突触模型
	std::vector<ModelDescription> SynapseModel;
	
};

/*
* 学习规则结构体
*/
struct LearningRuleDescription {
	//学习规则名称
	std::string RuleName;
	//学习规则参数
	std::map<std::string, boost::any> RuleParameter;
};

/*
* 外部动力学模块描述结构体
*/
struct OuterDynamicDescription {
	// 外部动力学模型名称
	std::string ModelName;
	// 外部动力学参数
	std::map<std::string, boost::any> ModelParameter;
	// 该模块的时间步长（基准步长的倍数）
	int update_timestep = 1;
	// 建议通信窗口，供外部模型做解码/平滑使用
	int communication_interval = 1;
	// 默认执行队列
	int queue_index = 0;
	// 每个关节对应的 GC 输入神经元索引。对于 total_model 风格复现，
	// 外部动力学模块会直接对这些 GC 输入层发放 one-hot 脉冲。
	std::vector<std::vector<int>> gc_neuron_indices_by_joint;
	// 每个关节对应的 MF 输入神经元索引。约定每个关节内按
	// [q_des, q_act, qd_des, qd_act] 四个通道顺序展开。
	std::vector<std::vector<int>> mf_neuron_indices_by_joint;
	// 每个关节对应的正误差 CF 输入神经元索引。
	std::vector<std::vector<int>> cf_positive_neuron_indices_by_joint;
	// 每个关节对应的负误差 CF 输入神经元索引。
	std::vector<std::vector<int>> cf_negative_neuron_indices_by_joint;
	// 每个关节对应的 DCN 正向输出神经元索引。
	std::vector<std::vector<int>> dcn_positive_neuron_indices_by_joint;
	// 每个关节对应的 DCN 负向输出神经元索引。
	std::vector<std::vector<int>> dcn_negative_neuron_indices_by_joint;
};



#endif
