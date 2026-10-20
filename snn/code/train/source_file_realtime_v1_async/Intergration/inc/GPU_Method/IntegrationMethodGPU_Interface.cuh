/*
* 文件名：IntegrationMethodGPU_Interface.cuh
* 定义了一个接口类，用于 GPU 上的积分方法
*/

#ifndef INTEGRATION_METHOD_GPU_INTERFACE_CUH
#define INTEGRATION_METHOD_GPU_INTERFACE_CUH
#include "../source_file_realtime_v1_async/error/cudaerror.h"

#include <string>
#include <map>
#include <boost/any.hpp>

class IntegrationMethodGPU_Interface {
public:
    // 指向 GPU 端积分方法参数表的指针数组
    void ** d_param;

    //时间步长
    float dt;

    //积分方法名称
    std::string name;

    /*
    * 构造函数
    */
    IntegrationMethodGPU_Interface():d_param(0), dt(0), name(""){}

    /*
    * 析构函数
    */
    virtual ~IntegrationMethodGPU_Interface() {
        if (this->d_param != 0) {
            HANDLE_ERROR(cudaFree(d_param));
        }
    }

    /*
    * 为 GPU 上的积分方法初始化参数
    */
    virtual void InitIntegrationMethodOnGPU(int N_neurons, int Total_N_Thread) = 0;

    /*
    * 获取积分步长
    */
    float Gettimestep() {
        return this->dt;
    }

    /*
    * 设置积分步长
    */
    virtual void Settimestep(float dt) {};

    /*
     * 获取积分参数
     */
    virtual std::map<std::string, boost::any> getParameters() {
        // 返回参数表
        std::map<std::string, boost::any> newMap;
        newMap["step"] = boost::any(this->dt);
        newMap["name"] = boost::any(this->name);
        return newMap;
    }


    /*
     * 设置积分参数
     */
    virtual void setParameters(std::map<std::string, boost::any> parameters) {
        // 写入参数表
        std::map<std::string, boost::any>::iterator it;
        it = parameters.find("name");
        if (it != parameters.end()) {
            std::string newParam = boost::any_cast<std::string>(it->second);
            this->name = newParam;
            parameters.erase(it);
        }

        it = parameters.find("step");
        if (it != parameters.end()) {
            float newParam = boost::any_cast<float>(it->second);
            this->dt = newParam;
            parameters.erase(it);
        }
    }

    /*
     * 比较两个积分方法是否相同
     */
    virtual bool compare(IntegrationMethodGPU_Interface* method) {
        if (this->name != method->name) {
            return false;
        }
        else if (this->dt != method->dt) {
            return false;
        }
        else {
            return true;
        }
    }

};


#endif // INTEGRATION_METHOD_GPU_INTERFACE_CUH
