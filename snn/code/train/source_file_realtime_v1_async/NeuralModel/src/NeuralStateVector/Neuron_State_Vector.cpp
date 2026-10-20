#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include <iostream>
#include <random>


Neuron_State_Vector::Neuron_State_Vector(int StateNumber, bool isTimedriven):
    NumberofStateVariable(StateNumber), TimeDriven(isTimedriven), NumberofNeuron(0),
    Vector_of_StateVariable(0), IsMonitored(false), IsGPU(false), LastUpdate(0), LastSpike(0), PredictSpike(0), SpikeIndex(0), NumberofSpike(0) {}

Neuron_State_Vector::Neuron_State_Vector(int StateNumber, bool isTimedriven, bool isGPU):
    NumberofStateVariable(StateNumber), TimeDriven(isTimedriven), NumberofNeuron(0),
    Vector_of_StateVariable(0), IsMonitored(false), IsGPU(isGPU), LastUpdate(0), LastSpike(0), PredictSpike(0), SpikeIndex(0), NumberofSpike(0) {}


Neuron_State_Vector::~Neuron_State_Vector(){
    if (this->Vector_of_StateVariable != 0) {
        delete[] this->Vector_of_StateVariable;
        this->Vector_of_StateVariable = NULL;
    }
    if (this->init_Vector_of_StateVariable != 0) {
        delete[] this->init_Vector_of_StateVariable;
        this->init_Vector_of_StateVariable = NULL;
    }
    if (this->LastUpdate != 0) {
        delete[] this->LastUpdate;
        this->LastUpdate = NULL;
    }
    if (this->LastSpike != 0) {
        delete[] this->LastSpike;
        this->LastSpike = NULL;
    }
    if (this->PredictSpike != 0) {
        delete[] this->PredictSpike;
        this->PredictSpike = NULL;
    }
    if (this->SpikeIndex != 0) {
        delete[] this->SpikeIndex;
        this->SpikeIndex = NULL;
    }
}


void Neuron_State_Vector::InitNeuronState(int NumberofNeuron, float* InitialState, float* sigma) {
    this->NumberofNeuron = NumberofNeuron;
    this->SpikeIndex = new int[this->NumberofNeuron];
    std::random_device rd;       // 用于生成随机种子
    std::mt19937 gen(rd());      // 随机数引擎（梅森旋转算法）

    // 创建标准正态分布对象：mean = 0, stddev = 1
    std::normal_distribution<float> dist(0.0, 1.0);
    //分配内存空间
    this->Vector_of_StateVariable = new float[this->NumberofNeuron * this->NumberofStateVariable]();
    this->init_Vector_of_StateVariable = new float[this->NumberofNeuron * this->NumberofStateVariable]();
    this->LastUpdate = new int[this->NumberofNeuron]();
    this->LastSpike = new int[this->NumberofNeuron]();
    if (!this->TimeDriven) {
        PredictSpike = new int[this->NumberofNeuron]();
    }
    for (int i = 0; i < this->NumberofNeuron * this->NumberofStateVariable; i += this->NumberofStateVariable) {
        for (int j = 0; j < this->NumberofStateVariable; j++) {
            this->Vector_of_StateVariable[i + j] = InitialState[j] + sigma[j] * dist(gen);
            this->init_Vector_of_StateVariable[i + j] = this->Vector_of_StateVariable[i + j];
        }
    }
    //为每个神经元初始化一个时间
    for (int i = 0; i < this->NumberofNeuron; i++) {
        this->LastSpike[i] = 10000;
    }
}

void Neuron_State_Vector::SetNeuronState(int NeuronIndex, int StateVariableIndex, float StateVariable) {
    //判断State是否是CPU还是GPU与CPU的接口
    if (this->IsGPU == false) {
        this->Vector_of_StateVariable[NeuronIndex * this->NumberofStateVariable + StateVariableIndex] = StateVariable;
    }
    else {
        //如果是在GPU上运行，则调用GPU的接口,GPU当中的结构采用了相同类型的数据内层连续分布，以适配GPU的连续访问能力
        this->Vector_of_StateVariable[this->NumberofNeuron * StateVariableIndex + NeuronIndex] = StateVariable;
    }
}

void Neuron_State_Vector::SetNeuronStateIncrement(int NeuronIndex, int StateVariableIndex, float StateVariableIncreament) {
    if (this->IsGPU == false) {
        this->Vector_of_StateVariable[NeuronIndex * this->NumberofStateVariable + StateVariableIndex] += StateVariableIncreament;
    }
    else {
        this->Vector_of_StateVariable[this->NumberofNeuron * StateVariableIndex + NeuronIndex] += StateVariableIncreament;
    }
}

float* Neuron_State_Vector::GetNeuronState(int NeuronIndex) {
    //该方法仅供CPU上的NeuronModel使用
    if (this->IsGPU == false) {
        return this->Vector_of_StateVariable + (NeuronIndex * this->NumberofStateVariable);
    }
    return NULL;
}

void Neuron_State_Vector::ResetNeuronState(int NeuronIndex) {
    //重置对应的神经元状态
    for (int i = 0; i < this->NumberofStateVariable; i++) {
        this->Vector_of_StateVariable[NeuronIndex * this->NumberofStateVariable + i] = this->init_Vector_of_StateVariable[NeuronIndex * this->NumberofStateVariable + i];
    }
}

void Neuron_State_Vector::ResetAllNeuronStates() {
    for (int neuron = 0; neuron < this->NumberofNeuron; ++neuron) {
        this->ResetNeuronState(neuron);
    }

    for (int neuron = 0; neuron < this->NumberofNeuron; ++neuron) {
        this->LastUpdate[neuron] = 0;
        this->LastSpike[neuron] = 10000;
        if (this->PredictSpike != 0) {
            this->PredictSpike[neuron] = 0;
        }
        if (this->SpikeIndex != 0) {
            this->SpikeIndex[neuron] = 0;
        }
    }

    this->NumberofSpike = 0;
}



int Neuron_State_Vector::GetNumberOfPrintableValues() {
    return this->NumberofStateVariable + 3;
}

float Neuron_State_Vector::GetPrintableValuesAt(int index, int position) {
    if (position < this->NumberofStateVariable) {
        if (this->IsGPU == false) {
            return this->Vector_of_StateVariable[index * this->NumberofStateVariable + position];
        }
        else {
            return this->Vector_of_StateVariable[this->NumberofNeuron * position + index];
        }
    }
    else if (position == this->NumberofStateVariable) {
        return this->LastUpdate[index];
    }
    else if (this->TimeDriven == true) {
        return -1;
    }
    else if (position == this->NumberofStateVariable + 1) {
        return -1;
    }
    else if (position == this->NumberofStateVariable + 2) {
        return -1;
    }
    else return -1;
}
