/*
* GPU上的积分方法
*/
#ifndef INTEGRATIONMETHODONGPU_CUH
#define INTEGRATIONMETHODONGPU_CUH

#include <string>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

class IntegrationMethodOnGPU {
public:
    //积分时间步长
    float dt;
    //有效的积分项
    float valid_Aux = 0;
    //积分方法名称
    char* name;

    /*
    * 构造函数
    */
    __device__ IntegrationMethodOnGPU(void** d_parm) {
        float* int_method_param = (float*)d_parm[0];
        dt = int_method_param[0];
    }

    /*
    * 析构函数
    */
    __device__ virtual ~IntegrationMethodOnGPU() {}

    /*
    * 计算积分增量
    * StateSize: 神经元状态大小
    * NeuronState: 神经元状态
    */
    __device__ virtual void CaculateIncreament(int StateSize, float* NeuronState){}

    /*
    * 重置积分状态
    */
    __device__ virtual void ResetState(int index) {

    }

    /*
    * 计算电导项
    */
    __device__ virtual void Calculate_conductance_exp_values() {}

    

};


#endif // !INTEGRATIONMETHODONGPU_CUH