/*
* 文件名: Interconnections.h
* 定义了突触连接的类
*/





#ifndef INTERCONNECTIONS_H
#define INTERCONNECTIONS_H

#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

class LearningRule;



class Interconnections {
    public:
        /*鍏抽敭鎴愬憳鏁版嵁缁撴瀯*/
        Neuron* SourceNeuron; //源神经元

        Neuron* TargetNeuron; //目标神经元

        NeuronModel* TargetNeuronModel; //源神经元模型

        int delay; //连接延迟

        float weight; //杩炴帴鏉冮噸

        float maximum_weight; //连接最大权重

        int Index; //连接的索引

        int Synmodel_Index; //Synmodel索引(由checktype函数确定)

        int TargetNeuronModelIndex; //目标神经元在其模型的索引

        int type; //连接类型兴奋或抑制(兴奋0，抑制1， NMDA2, 外电流3）

        int subindex_type; //电流传入突触对应的局部索引

        int LearningRuleIndex_withPost; //后突触学习规则在SynapseState中的索引

        int LearningRuleIndex_withTrigger; //前突触学习规则在SynapseState中的索引

        int LearningRuleIndex_withPostAndTrigger; //前突触和后突触学习规则在SynapseState中的索引

        int LearningRuleIndex_withPost_Target; //突触学习规则在学习规则对应的连接列表中的索引

        int LearningRuleIndex_withTrigger_Target; //前突触学习规则在学习规则对应的连接列表中的索引

        int LearningRuleIndex_withPostAndTrigger_Target; //前突触和后突触学习规则在学习规则对应的连接列表中的索引

        LearningRule* LearningRule_withPost; //无示教学习规则

        LearningRule* LearningRule_withTrigger; //示教学习规则

        LearningRule* LearningRule_withPostAndTrigger; //前突触和后突触学习规则

        bool TriggerLearning = false; //是否示教学习

        ModelDescription synmodel; //连接模型

        /*
        * 默认的无参构造函数
        */
        Interconnections();

        /*
        * 有参构造函数
        */
        Interconnections(Neuron* source, Neuron* target, NeuronModel* targetModel, int delay, float weight, int type, int index);

        /*
        * 析构函数
        */
        ~Interconnections();

        /*
        * 璁剧疆杩炴帴绱㈠紩
        */
        void SetIndex(int index);

        /*
        * 璁剧疆婧愮缁忓厓
        */
        void SetSourceNeuron(Neuron* source);

        /*
        * 设置目标神经元
        */
        void SetTargetNeuron(Neuron* target);

        /*
        * 设置目标神经元模型
        */
        void SetTargetNeuronModel(NeuronModel* targetModel);

        /*
        * 设置目标神经元在其模型中的索引
        */
        void SetTargetNeuronModelIndex(int index);

        /*
        * 设置连接权重
        */
        void SetWeight(float weight);

        /*
        * 设置连接延迟
        */
        void SetDelay(int delay);

        /*
        * 璁剧疆杩炴帴绫诲瀷
        */
        void SetType(int type);

        /*
        * 权重更新内联函数
        */
        inline void UpdateWeight(float Increament) {
            this->weight += Increament;
            if (this->weight > this->maximum_weight) {
                this->weight = this->maximum_weight;
            }
            else if (this->weight < 0.0) {
                this->weight = 0.0;
            }
        }
        
};


#endif

