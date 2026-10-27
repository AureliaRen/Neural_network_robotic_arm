/*
* 文件名：NeuronModelFactory.h
* 定义了一个工厂类，用于创建不同类型的神经元模型
*/
#ifndef NEURONMODELFACTORY_H
#define NEURONMODELFACTORY_H
#include <iostream>
#include <string>
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenLIF_Voltage_jump.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/EventDriven/InputSpikeNeuronModel.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/EventDriven/InputCurrentNeuronModel.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenLIF_Exponential_Decay.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenLIF_Exponential_double.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenIzhikevic_Exponential_Decay.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenLIFModel.h"
#if !SNN_CPU_ONLY
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenLIF_Exponential_double_GPU_Interface.cuh"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenLIF_Exponential_triple_GPU_Interface.cuh"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface.cuh"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/EDLUTLikeLIF_GPU_Interface.cuh"
#endif
//#include "../source_file/NeuralModel/inc/EventDriven/MFInputNeuronModel.h"
class NeuronModelFactory {
    public:
        static NeuronModel* createNeuronModel(const std::string& type, std::map<std::string, boost::any> NeuronParameter,int timestepsize, float basetimestepsize, int QueueIndex) {
            if (type == "TimeDrivenLIF_Voltage_jump") {
               TimeDrivenLIF_Voltage_jump* model = new TimeDrivenLIF_Voltage_jump(timestepsize);
               model->SetParameters(NeuronParameter, basetimestepsize);
               return model;
            }
            else if (type == "InputSpikeNeuronModel") {
                InputSpikeNeuronModel* model = new InputSpikeNeuronModel();
                return model;
            }
            else if (type == "InputCurrentNeuronModel") {
                InputCurrentNeuronModel* model = new InputCurrentNeuronModel();
                return model;
            }
            else if (type == "TimeDrivenLIF_Exponential_Decay") {
                TimeDrivenLIF_Exponential_Decay* model = new TimeDrivenLIF_Exponential_Decay(timestepsize);
                model->SetParameters(NeuronParameter, basetimestepsize);
                return model;
            }
            else if (type == "TimeDrivenLIF_Exponential_double") {
                TimeDrivenLIF_Exponential_double* model = new TimeDrivenLIF_Exponential_double(timestepsize);
                model->SetParameters(NeuronParameter, basetimestepsize);
                return model;
            }
            else if (type == "TimeDrivenIzhikevic_I_Exponential_Decay" || type == "TimeDrivenIzhikevic_Exponential_Decay") {
                TimeDrivenIzhikevic_I_Exponential_Decay* model = new TimeDrivenIzhikevic_I_Exponential_Decay(timestepsize);
                model->SetParameters(NeuronParameter, basetimestepsize);
                return model;
            }
#if !SNN_CPU_ONLY
            else if (type == "TimeDrivenLIF_Exponential_double_GPU") {
                TimeDrivenLIF_Exponential_double_GPU_Interface* model = new TimeDrivenLIF_Exponential_double_GPU_Interface(timestepsize);
                model->SetParameters(NeuronParameter, basetimestepsize);
                model->timestepdouble = basetimestepsize;
                return model;
            }
            else if (type == "TimeDrivenLIF_Exponential_triple_GPU") {
                TimeDrivenLIF_Exponential_triple_GPU_Interface* model = new TimeDrivenLIF_Exponential_triple_GPU_Interface(timestepsize);
                model->SetParameters(NeuronParameter, basetimestepsize);
                model->timestepdouble = basetimestepsize;
                return model;
            }
            else if (type == "TimeDrivenIzhikevic_I_Exponential_Decay_GPU" || type == "TimeDrivenIzhikevic_Exponential_Decay_GPU") {
                TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface* model = new TimeDrivenIzhikevic_Exponential_Decay_GPU_Interface(timestepsize);
                model->SetParameters(NeuronParameter, basetimestepsize);
                model->timestepdouble = basetimestepsize;
                return model;
            }
            else if (type == "EDLUTLikeLIF_GPU") {
                EDLUTLikeLIF_GPU_Interface* model = new EDLUTLikeLIF_GPU_Interface(timestepsize);
                model->SetParameters(NeuronParameter, basetimestepsize);
                model->timestepdouble = basetimestepsize;
                return model;
            }
#else
            else if (type == "TimeDrivenLIF_Exponential_double_GPU" || type == "TimeDrivenLIF_Exponential_triple_GPU" || type == "TimeDrivenIzhikevic_I_Exponential_Decay_GPU" || type == "TimeDrivenIzhikevic_Exponential_Decay_GPU" || type == "EDLUTLikeLIF_GPU") {
                std::cout << "GPU neuron model type is unavailable in CPU-only build: " << type << std::endl;
                return 0;
            }
#endif
            else if (type == "TimeDrivenLIFModel") {
                TimeDrivenLIFModel* model = new TimeDrivenLIFModel(timestepsize);
                model->SetParameters(NeuronParameter, basetimestepsize);
                return model;
            }
            /*
            else if (type == "MF_InputSpikeNeuronModel") {
                MFInputNeuronModel* model = new MFInputNeuronModel();
            }
            */
            else {
                std::cout << "Unknown neuron model type: " << type << std::endl;
                return 0;
            }

        }
};

#endif
