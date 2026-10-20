/*
* IntegrationMethodTemplate_Syn.h
*/
#ifndef _INTEGRATIONMETHODTEMPLATE_SYN_H_
#define _INTEGRATIONMETHODTEMPLATE_SYN_H_

#include "../source_file_realtime_v1_async/Intergration/inc/IntegrationMethod_Syn.h"

template <typename SynModel>
class IntegerationMethodTemplate_Syn : public IntegerationMethod_Syn {
public:
    SynModel* synmodel;

    /*
    * 构造函数
    */
    IntegerationMethodTemplate_Syn(SynModel* syn_model) :IntegerationMethod_Syn(), synmodel(syn_model) {};

    /*
    * 析构函数
    */
    virtual ~IntegerationMethodTemplate_Syn() {};

    virtual void CaculateIncreament() {};

    /*
    * 重置积分状态
    * index: 积分状态变量下标
    */
    virtual void ResetState(int index) {};

    /*
    * 获取参数
    */
    virtual std::map<std::string, boost::any> getParameters() {
        // 调用基类 IntegerationMethod_Syn 的参数接口
        std::map<std::string, boost::any> parameters = IntegerationMethod_Syn::getParameters();
        return parameters;
    }

    /*
    * 设置参数
    */
    virtual void setParameters(std::map<std::string, boost::any> parameters) {
        // 调用基类 IntegerationMethod_Syn 的参数接口
        IntegerationMethod_Syn::setParameters(parameters);
    }

    /*
    * 比较两个积分方法是否一致
    */
    virtual bool compare(IntegerationMethod_Syn* method) {
        if (!IntegerationMethod_Syn::compare(method)) {
            return false;
        }
        IntegerationMethodTemplate_Syn* method1 = dynamic_cast<IntegerationMethodTemplate_Syn*>(method);
        if (method1 == nullptr) {
            return false;
        }
        else {
            return true;
        }
    }
};

#endif
