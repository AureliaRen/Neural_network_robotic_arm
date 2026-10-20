/*
* FixStepMethod_Syn.h
*/
#ifndef FIXSTEPMETHOD_SYN_H
#define FIXSTEPMETHOD_SYN_H
#include "../source_file_realtime_v1_async/Intergration/inc/IntegrationMethodTemplate_Syn.h"

template<typename SynModel>
class FixStepMethod_Syn :public IntegerationMethodTemplate_Syn<SynModel> {
public:
    /*
    * 构造函数
    */
    FixStepMethod_Syn(SynModel* model) :IntegerationMethodTemplate_Syn<SynModel>(model) {};

    /*
    * 析构函数
    */
    virtual ~FixStepMethod_Syn() {};

    virtual void CaculateIncreament() = 0;

    /*
    * 重置积分状态
    * index: 积分状态变量下标
    */
    virtual void ResetState(int index) = 0;

    /*
    * 获取参数
    */
    virtual std::map<std::string, boost::any> getParameters() {
        std::map<std::string, boost::any> parameters = IntegerationMethodTemplate_Syn<SynModel>::getParameters();
        return parameters;
    }

    /*
    * 设置参数
    * parameters: 参数映射表
    */
    virtual void setParameters(std::map<std::string, boost::any> parameters) {
        // 调用基类 IntegerationMethodTemplate_Syn 的参数接口
        IntegerationMethodTemplate_Syn<SynModel>::setParameters(parameters);
    }

    /*
    * 比较两个积分方法是否一致
    */
    virtual bool compare(IntegerationMethod_Syn* method) {
        if (!IntegerationMethodTemplate_Syn<SynModel>::compare(method)) {
            return false;
        }
        FixStepMethod_Syn* method1 = dynamic_cast<FixStepMethod_Syn*>(method);
        if (method1 == nullptr) {
            return false;
        }
        else {
            return true;
        }
    }
};

#endif /* FIXSTEPMETHOD_SYN_H */
