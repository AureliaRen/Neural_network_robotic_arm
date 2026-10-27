/*
* ForwardEulerMethod_Syn.h
*/
#ifndef FORWARDEULERMETHOD_SYN_H
#define FORWARDEULERMETHOD_SYN_H

#include"../source_file_realtime_v1_async/Intergration/inc/FixStep/FixStepMethod_Syn.h"

template<typename SynModel>
class ForwardEulerMethod_Syn :public FixStepMethod_Syn<SynModel> {
public:
    /*
    * 构造函数
    */
    ForwardEulerMethod_Syn(SynModel* synmodel) :FixStepMethod_Syn<SynModel>(synmodel) {}

    /*
    * 析构函数
    */
    ~ForwardEulerMethod_Syn() {}

    /*
    * 计算增量（前向欧拉法）
    */
    virtual void CaculateIncreament() {
    
        float AuxState[MAX_VARIABLES];
        for (int i = 0; i < this->synmodel->NumberOfNeuron; i++) {
            // 获取当前突触状态
            float* SynState = this->synmodel->GetState(i);
            // 计算微分方程的导数
            this->synmodel->CaculateDifferentialEquation(SynState, AuxState, i);
            // 累加状态增量
            for (int j = 0; j < this->synmodel->N_DifferentialStates; j++) {
                SynState[j] += this->synmodel->getTimestepSize() * AuxState[j] * this->dt;
            }
            this->IncrementValidIntegrationVariable(SynState[0]);
        }
    }

    /*
    * 重置状态（该方法无需额外处理）
    */
    virtual void ResetState(int index) {};

    /*
    * 获取参数
    */
    virtual std::map<std::string, boost::any> getParameters() {
        // 调用基类参数接口，并更新方法名
        std::map<std::string, boost::any> newMap = FixStepMethod_Syn<SynModel>::getParameters();
        newMap["name"] = getName();
        return newMap;
    }

    /*
    * 设置参数
    */
    virtual void setParameters(std::map<std::string, boost::any> parameters) {
        FixStepMethod_Syn<SynModel>::setParameters(parameters);
    }

    /*
    * 获取积分方法名称
    */
    static std::string getName() {
        return "ForwardEulerMethod";
    }

    /*
    * 创建积分方法实例
    */
    static IntegerationMethod_Syn* CreateIntegerationMethod(std::map<std::string, boost::any> NeuronParameter, SynModel* synmodel) {
        // 创建方法对象并写入参数
        ForwardEulerMethod_Syn<SynModel>* newmodel = new ForwardEulerMethod_Syn<SynModel>(synmodel);
        newmodel->setParameters(NeuronParameter);
        return newmodel;
    }

    /*
    * 比较两个积分方法是否一致
    */
    virtual bool compare(IntegerationMethod_Syn* method) {
        // 先比较基类部分
        if (!FixStepMethod_Syn<SynModel>::compare(method)) {
            return false;
        }
        ForwardEulerMethod_Syn* method2 = dynamic_cast<ForwardEulerMethod_Syn*>(method);
        if (method2 == nullptr) {
            return false;
        }
        return true;
    }
};

#endif
