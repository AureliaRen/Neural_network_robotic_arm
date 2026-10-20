/*
* 文件名：Network.h
* 定义了一个Network类，用于表示神经网络
*/
#ifndef NETWORK_H
#define NETWORK_H
class Interconnections;
#include <list>
#include <vector>
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/LearningRule.h"

class Network {
    public:
        Interconnections* inters; //一个存放所有连接的数组

        int intersNum; //连接的数量

        int timesteps; //基准时间步长的timesteps倍数

        NeuronModel*** neurontypes; //一个存放所有神经元NeuronModel指针的数组,每个元素都是一个指向NeuronModel的指针,neurontypesNum*QueueIndex

        int neurontypesNum; //神经元类型数量

        Neuron* neurons; //一个存放所有神经元Neuron的数组

        int neuronsNum; //神经元数量

        Neuron**** TimeDrivenNeurons; //一个存放所有时间驱动神经元Neuron的数组,每个元素都是一个指向Neuron的指针,neurontype*QueueIndex*number*指针  

        int** TimeDrivenNeuronsNum; //一个存放所有时间驱动神经元数量的数组,每个元素都是一个指向int的指针,neurontype*QueueIndex

        Neuron**** TimeDrivenNeuronsGPU; //一个存放所有GPU时间驱动神经元Neuron的数组,每个元素都是一个指向Neuron的指针,neurontype*QueueIndex*number

        int** TimeDrivenNeuronsNumGPU; //一个存放所有GPU时间驱动神经元数量的数组,每个元素都是一个指向int的指针,neurontype*

        int NumberOfQueue; //队列数量

        Interconnections** wordination; //一个存放排序后连接与排序前连接的对应关系的数组,每个元素都是一个指向Interconnections的指针

        int LearningRuleNum; //学习规则模型数量

        LearningRule** LearningRules; //一个存放所有学习规则LearningRule的数组,每个元素都是一个指向LearningRule的指针

        bool isMonitor; //是否监控

        float basetemestepsize;

        std::vector<float> initial_weights;

        

        /*
        * 无参构造函数
        */
        Network();

        /*
        * 有参构造函数
        * neuron_layer_list: 神经元层描述列表
        * connection_list: 连接描述列表
        */
        Network(const std::list<NeuronLayerDescription>& neuron_layer_list, const std::list<ConnectionDescription>& connection_list, const std::list<LearningRuleDescription>& learning_rule_list, int NumberOfQueue, float basetemestepsize, Simulation* simulation);

        /*
        * 析构函数
        */
        ~Network();

        /*
        * 瀵硅繛鎺ヨ繘琛屽崌搴忔帓鍒楋紝璁＄畻propogationstructure
        */
        void CaculateOutputConnection();

        /*
        * 计算传入结构，用于将突触可塑性信息绑定到神经元
        */
        void CaculateInputConnection();

        /*
        * 缂栬瘧缃戠粶
        * neuron_layer_list: 绁炵粡鍏冨眰鎻忚堪鍒楄〃
        * connection_list: 杩炴帴鎻忚堪鍒楄〃
        */
        void CompileNetwork(const std::list<NeuronLayerDescription>& neuron_layer_list, const std::list<ConnectionDescription>& connection_list, const std::list<LearningRuleDescription>& learning_rule_list, Simulation* simulation);

        /*
        * 创建神经元模型
        * neuron_layer_list: 神经元层描述列表
        */
        void CreateNeuronModel(const std::list<NeuronLayerDescription>& neuron_layer_list, int timesteps, float basetimestepsize, Simulation* simulation);

        /*
        * 初始化网络当中的神经元
        * N_neurons: 每个模型中的神经元数量
        */
        void InitializeStates(int** Neurons);

        /*
        * 寤虹珛缃戠粶杩炴帴
        * connection_list: 杩炴帴鎻忚堪鍒楄〃
        */
        void CreateConnections(const std::list<ConnectionDescription>& connection_list, std::vector<int>& N_connectionsPerRule);

        /*
        * 璁剧疆杩炴帴椤哄簭绱㈠紩
        */
        void setWeightOrdination();

        /*
        * 初始化连接突触可塑性状态
        */
        void InitializeSynapticPlasticityState(const std::vector<int>& N_connectionsPerRule);

        /*
        * 添加连接权值学习规则
        * learning_rule_list: 学习规则描述列表
        * return: 返回一个存放每个突触可塑性模型数量的向量
        */
        std::vector<int> CreateWeightChange(const std::list<LearningRuleDescription>& learning_rule_list);

        /*
        * 计算最小跨线程延迟
        */
        int GetMinInterpropagationTime();

        /*
        * 保存权重到文件
        */
        void SaveWeightsToFile(const char* filename);

        /*
        * 从文件当中加载权重
        */
        void LoadWeightsFromFile(const char* filename);

        /*
        * 重置网络的动态状态，保留或恢复权重
        */
        void ResetDynamicState(bool preserve_weights = true);

        /*
        * 记录当前权重为初始权重快照
        */
        void SnapshotInitialWeights();

        /*
        * 恢复到初始权重快照
        */
        void RestoreInitialWeights();




};

#endif
