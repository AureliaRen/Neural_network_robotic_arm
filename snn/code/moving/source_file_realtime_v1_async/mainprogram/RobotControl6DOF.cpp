#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/communication/inc/DriverType.h"
#include <iostream>
#include <thread>
#include <cassert>
#include <zmq.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <array>
#include <list>
#include <random>

constexpr double PI = 3.14159265358979323846;
constexpr unsigned short TEST_PORT = 5565;

// ==================CF 驱动参数======================
// 六个关节的位置/速度 PD 参数
static const double kp[6] = { 1.5, 2.0, 3.0, 2.0, 3.0, 3.0 };
static const double kv[6] = { 1.5, 1.0, 3.0, 1.0, 3.0, 0.5 };

// 将误差幅值映射到 [0,1]，饱和区间由 eps_sat 控制
static inline double sat01(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

int cf_joint = 3;              // 只对 joint3 注入 CF
double eps_sat = 1.0;          // 误差饱和阈值，|eps|>=eps_sat 时按最大强度触发
double cf_rate_max_hz = 45.0;  // 单个 CF 的最大发放频率（Hz），按每步换算为发放概率
double dt_s = 0.002;           // 仿真步长 2ms

std::mt19937 rng(12345);       // 固定随机种子，保证连接生成可复现
std::uniform_real_distribution<double> uni01(0.0, 1.0);
std::uniform_int_distribution<int> pick_pos(0, 49);
std::uniform_int_distribution<int> pick_neg(50, 99);


int main() {

    const int DOF = 4;

    // 每个关节的各类神经元数量
    const int MF_PER = 28;
    const int CF_PER = 100;
    const int GC_PER = 2401; // 7^4
    const int PC_PER = 100;
    const int DCN_PER = 100;

    // 神经层描述
    std::list<NeuronLayerDescription> neuDescription;
    // 突触连接描述
    std::list<ConnectionDescription> synDescription;
    // 学习规则描述
    std::list<LearningRuleDescription> learnDescription;


    // =========================
    // 1) 创建神经元层（按关节）
    // =========================

    for (int joint = 0; joint < DOF; ++joint) {
        // MF
        NeuronLayerDescription MF;
        MF.ModelName = "InputSpikeNeuronModel";
        MF.numberofneuron = MF_PER;
        MF.isMonitored = false;
        MF.isOutput = false;
        neuDescription.push_back(MF);

        // CF
        NeuronLayerDescription CF;
        CF.ModelName = "InputSpikeNeuronModel";
        CF.numberofneuron = CF_PER;
        CF.isMonitored = false;
        CF.isOutput = false;
        neuDescription.push_back(CF);

        // GC
        NeuronLayerDescription GC;
        GC.ModelName = "TimeDrivenLIF_Exponential_triple_GPU";
        GC.numberofneuron = GC_PER;
        GC.isMonitored = false;
        GC.isOutput = false;
        neuDescription.push_back(GC);

        // PC
        NeuronLayerDescription PC;
        PC.ModelName = "TimeDrivenLIF_Exponential_triple_GPU";
        PC.numberofneuron = PC_PER;
        PC.isMonitored = false;
        PC.isOutput = false;
        neuDescription.push_back(PC);

        // DCN
        NeuronLayerDescription DCN;
        DCN.ModelName = "TimeDrivenLIF_Exponential_triple_GPU";
        DCN.numberofneuron = DCN_PER;
        DCN.isMonitored = false;
        DCN.isOutput = true;
        neuDescription.push_back(DCN);
    }


    // =========================
    // 2) 计算每个关节的起始索引
    // =========================
    // 每个关节包含 MF(28) CF(100) GC(2401) PC(100) DCN(100)
    // 一个关节内按全局 neuron index 顺序连续排布
    const int JOINT_STRIDE = MF_PER + CF_PER + GC_PER + PC_PER + DCN_PER; // 28+100+2401+100+100=2729

    auto MF0 = [&](int joint) { return joint * JOINT_STRIDE + 0; };
    auto CF0 = [&](int joint) { return joint * JOINT_STRIDE + MF_PER; };
    auto GC0 = [&](int joint) { return joint * JOINT_STRIDE + MF_PER + CF_PER; };
    auto PC0 = [&](int joint) { return joint * JOINT_STRIDE + MF_PER + CF_PER + GC_PER; };
    auto DCN0 = [&](int joint) { return joint * JOINT_STRIDE + MF_PER + CF_PER + GC_PER + PC_PER; };


    // 全局 MF/GC/PC/DCN 数量统计
    const int MF_TOTAL = DOF * MF_PER;   // 168
    const int CF_TOTAL = DOF * CF_PER;   // 600
    const int GC_TOTAL = DOF * GC_PER;   // 14406
    const int PC_TOTAL = DOF * PC_PER;   // 600
    const int DCN_TOTAL = DOF * DCN_PER;  // 600


    // =========================
    // 3) 为每个关节构建局部连接，按 joint 内循环展开
    // =========================
    for (int joint = 0; joint < DOF; ++joint) {
        const int mfStart = MF0(joint);
        const int cfStart = CF0(joint);
        const int gcStart = GC0(joint);
        const int pcStart = PC0(joint);
        const int dcnStart = DCN0(joint);


        // -------- MF -> GC（局部一对多）--------
        {
            ConnectionDescription MF2GC;
            std::vector<int> source_neuron_list;
            std::vector<int> target_neuron_list;
            std::vector<int> Type;
            std::vector<float> Weight;
            std::vector<float> MaxWeight;
            std::vector<int> Delay;
            std::vector<int> learning_rule;
            std::vector<int> trigger_rule;
            std::vector<ModelDescription> modelDescription;
            ModelDescription temp{};

            int i = gcStart;
            for (int j1 = 0; j1 < 7; j1++) {
                for (int j2 = 7; j2 < 14; j2++) {
                    for (int j3 = 14; j3 < 21; j3++) {
                        for (int j4 = 21; j4 < 28; j4++) {

                            int pres[4] = { mfStart + j1, mfStart + j2, mfStart + j3, mfStart + j4 };
                            for (int k = 0; k < 4; ++k) {
                                source_neuron_list.push_back(pres[k]);
                                target_neuron_list.push_back(i);
                                Type.push_back(0);          // AMPA
                                Weight.push_back(0.18);
                                MaxWeight.push_back(0.18);
                                Delay.push_back(1);
                                learning_rule.push_back(-1);
                                trigger_rule.push_back(-1);
                                modelDescription.push_back(temp);
                            }
                            i++;
                        }
                    }
                }
            }

            MF2GC.SourceNeuron = source_neuron_list;
            MF2GC.TargetNeuron = target_neuron_list;
            MF2GC.Type = Type;
            MF2GC.Weight = Weight;
            MF2GC.MaxWeight = MaxWeight;
            MF2GC.Delay = Delay;
            MF2GC.SynapseRule = learning_rule;
            MF2GC.TriggerRule = trigger_rule;
            MF2GC.SynapseModel = modelDescription;
            synDescription.push_back(MF2GC);
        }


        // -------- PC -> DCN（一对一，每个关节 100 条）--------
        {
            ConnectionDescription PC2DCN;
            std::vector<int> source_neuron_list;
            std::vector<int> target_neuron_list;
            std::vector<int> Type;
            std::vector<float> Weight;
            std::vector<float> MaxWeight;
            std::vector<int> Delay;
            std::vector<int> learning_rule;
            std::vector<int> trigger_rule;
            std::vector<ModelDescription> modelDescription;
            ModelDescription temp{};

            for (int k = 0; k < PC_PER; ++k) {
                source_neuron_list.push_back(pcStart + k);
                target_neuron_list.push_back(dcnStart + k);
                Type.push_back(1); // GABA
                Weight.push_back(1.0);
                MaxWeight.push_back(1.0);
                Delay.push_back(1);
                learning_rule.push_back(-1);
                trigger_rule.push_back(-1);
                modelDescription.push_back(temp);
            }

            PC2DCN.SourceNeuron = source_neuron_list;
            PC2DCN.TargetNeuron = target_neuron_list;
            PC2DCN.Type = Type;
            PC2DCN.Weight = Weight;
            PC2DCN.MaxWeight = MaxWeight;
            PC2DCN.Delay = Delay;
            PC2DCN.SynapseRule = learning_rule;
            PC2DCN.TriggerRule = trigger_rule;
            PC2DCN.SynapseModel = modelDescription;
            synDescription.push_back(PC2DCN);
        }


        // -------- CF -> PC（一对一，每个关节 100 条）--------
        {
            ConnectionDescription CF2PC;
            std::vector<int> source_neuron_list;
            std::vector<int> target_neuron_list;
            std::vector<int> Type;
            std::vector<float> Weight;
            std::vector<float> MaxWeight;
            std::vector<int> Delay;
            std::vector<int> learning_rule;
            std::vector<int> trigger_rule;
            std::vector<ModelDescription> modelDescription;
            ModelDescription temp{};

            for (int k = 0; k < CF_PER; ++k) {
                source_neuron_list.push_back(cfStart + k);
                target_neuron_list.push_back(pcStart + k);
                Type.push_back(0); // AMPA
                Weight.push_back(0.0);
                MaxWeight.push_back(0.0);
                Delay.push_back(1);
                learning_rule.push_back(-1);
                trigger_rule.push_back(0);
                modelDescription.push_back(temp);
            }

            CF2PC.SourceNeuron = source_neuron_list;
            CF2PC.TargetNeuron = target_neuron_list;
            CF2PC.Type = Type;
            CF2PC.Weight = Weight;
            CF2PC.MaxWeight = MaxWeight;
            CF2PC.Delay = Delay;
            CF2PC.SynapseRule = learning_rule;
            CF2PC.TriggerRule = trigger_rule;
            CF2PC.SynapseModel = modelDescription;
            synDescription.push_back(CF2PC);
        }

        // -------- CF -> DCN（AMPA，一对一，每个关节 100 条）--------
        {
            ConnectionDescription CF2DCN_AMPA;
            std::vector<int> source_neuron_list;
            std::vector<int> target_neuron_list;
            std::vector<int> Type;
            std::vector<float> Weight;
            std::vector<float> MaxWeight;
            std::vector<int> Delay;
            std::vector<int> learning_rule;
            std::vector<int> trigger_rule;
            std::vector<ModelDescription> modelDescription;
            ModelDescription temp{};

            for (int k = 0; k < CF_PER; ++k) {
                source_neuron_list.push_back(cfStart + k);
                target_neuron_list.push_back(dcnStart + k);
                Type.push_back(0); // AMPA
                Weight.push_back(0.5);
                MaxWeight.push_back(0.5);
                Delay.push_back(1);
                learning_rule.push_back(-1);
                trigger_rule.push_back(-1);
                modelDescription.push_back(temp);
            }

            CF2DCN_AMPA.SourceNeuron = source_neuron_list;
            CF2DCN_AMPA.TargetNeuron = target_neuron_list;
            CF2DCN_AMPA.Type = Type;
            CF2DCN_AMPA.Weight = Weight;
            CF2DCN_AMPA.MaxWeight = MaxWeight;
            CF2DCN_AMPA.Delay = Delay;
            CF2DCN_AMPA.SynapseRule = learning_rule;
            CF2DCN_AMPA.TriggerRule = trigger_rule;
            CF2DCN_AMPA.SynapseModel = modelDescription;
            synDescription.push_back(CF2DCN_AMPA);
        }

        // -------- CF -> DCN（NMDA，一对一，每个关节 100 条）--------
        {
            ConnectionDescription CF2DCN_NMDA;
            std::vector<int> source_neuron_list;
            std::vector<int> target_neuron_list;
            std::vector<int> Type;
            std::vector<float> Weight;
            std::vector<float> MaxWeight;
            std::vector<int> Delay;
            std::vector<int> learning_rule;
            std::vector<int> trigger_rule;
            std::vector<ModelDescription> modelDescription;
            ModelDescription temp{};

            for (int k = 0; k < CF_PER; ++k) {
                source_neuron_list.push_back(cfStart + k);
                target_neuron_list.push_back(dcnStart + k);
                Type.push_back(2); // NMDA
                Weight.push_back(0.25);
                MaxWeight.push_back(0.25);
                Delay.push_back(1);
                learning_rule.push_back(-1);
                trigger_rule.push_back(-1);
                modelDescription.push_back(temp);
            }

            CF2DCN_NMDA.SourceNeuron = source_neuron_list;
            CF2DCN_NMDA.TargetNeuron = target_neuron_list;
            CF2DCN_NMDA.Type = Type;
            CF2DCN_NMDA.Weight = Weight;
            CF2DCN_NMDA.MaxWeight = MaxWeight;
            CF2DCN_NMDA.Delay = Delay;
            CF2DCN_NMDA.SynapseRule = learning_rule;
            CF2DCN_NMDA.TriggerRule = trigger_rule;
            CF2DCN_NMDA.SynapseModel = modelDescription;
            synDescription.push_back(CF2DCN_NMDA);
        }
    }


    // =========================
    // 4) 构建跨层全连接：MF(all)->DCN(all), GC(all)->PC(all)
    // =========================

    // -------- MF(all 168) -> DCN(all 600): 全连接，共 100800 条 --------
    {
        ConnectionDescription MF2DCN;
        std::vector<int> source_neuron_list;
        std::vector<int> target_neuron_list;
        std::vector<int> Type;
        std::vector<float> Weight;
        std::vector<float> MaxWeight;
        std::vector<int> Delay;
        std::vector<int> learning_rule;
        std::vector<int> trigger_rule;
        std::vector<ModelDescription> modelDescription;
        ModelDescription temp{};


        // 收集全部 MF 索引
        std::vector<int> allMF;
        allMF.reserve(MF_TOTAL);
        for (int joint = 0; joint < DOF; ++joint) {
            for (int k = 0; k < MF_PER; ++k) {
                allMF.push_back(MF0(joint) + k);
            }
        }

        // 收集全部 DCN 索引
        std::vector<int> allDCN;
        allDCN.reserve(DCN_TOTAL);
        for (int joint = 0; joint < DOF; ++joint) {
            for (int k = 0; k < DCN_PER; ++k) {
                allDCN.push_back(DCN0(joint) + k);
            }
        }

        for (int di = 0; di < (int)allDCN.size(); ++di) {
            int dcn = allDCN[di];
            for (int mi = 0; mi < (int)allMF.size(); ++mi) {
                int mf = allMF[mi];
                source_neuron_list.push_back(mf);
                target_neuron_list.push_back(dcn);
                Type.push_back(0); // AMPA
                Weight.push_back(0.1);
                MaxWeight.push_back(0.1);
                Delay.push_back(1);
                learning_rule.push_back(-1);
                trigger_rule.push_back(-1);
                modelDescription.push_back(temp);
            }
        }

        MF2DCN.SourceNeuron = source_neuron_list;
        MF2DCN.TargetNeuron = target_neuron_list;
        MF2DCN.Type = Type;
        MF2DCN.Weight = Weight;
        MF2DCN.MaxWeight = MaxWeight;
        MF2DCN.Delay = Delay;
        MF2DCN.SynapseRule = learning_rule;
        MF2DCN.TriggerRule = trigger_rule;
        MF2DCN.SynapseModel = modelDescription;
        synDescription.push_back(MF2DCN);
    }

    // -------- GC(all 14406) -> PC(all 600): 全连接 --------
    {
        ConnectionDescription GC2PC;
        std::vector<int> source_neuron_list;
        std::vector<int> target_neuron_list;
        std::vector<int> Type;
        std::vector<float> Weight;
        std::vector<float> MaxWeight;
        std::vector<int> Delay;
        std::vector<int> learning_rule;
        std::vector<int> trigger_rule;
        std::vector<ModelDescription> modelDescription;
        ModelDescription temp{};

        // 收集全部 GC 索引
        std::vector<int> allGC;
        allGC.reserve(GC_TOTAL);
        for (int joint = 0; joint < DOF; ++joint) {
            for (int k = 0; k < GC_PER; ++k) {
                allGC.push_back(GC0(joint) + k);
            }
        }

        // 收集全部 PC 索引
        std::vector<int> allPC;
        allPC.reserve(PC_TOTAL);
        for (int joint = 0; joint < DOF; ++joint) {
            for (int k = 0; k < PC_PER; ++k) {
                allPC.push_back(PC0(joint) + k);
            }
        }

        for (int pi = 0; pi < (int)allPC.size(); ++pi) {
            int pc = allPC[pi];
            for (int gi = 0; gi < (int)allGC.size(); ++gi) {
                int gc = allGC[gi];
                source_neuron_list.push_back(gc);
                target_neuron_list.push_back(pc);
                Type.push_back(0); // AMPA
                Weight.push_back(1.6);
                MaxWeight.push_back(5.0);
                Delay.push_back(1);
                learning_rule.push_back(0);
                trigger_rule.push_back(-1);
                modelDescription.push_back(temp);
            }
        }

        GC2PC.SourceNeuron = source_neuron_list;
        GC2PC.TargetNeuron = target_neuron_list;
        GC2PC.Type = Type;
        GC2PC.Weight = Weight;
        GC2PC.MaxWeight = MaxWeight;
        GC2PC.Delay = Delay;
        GC2PC.SynapseRule = learning_rule;
        GC2PC.TriggerRule = trigger_rule;
        GC2PC.SynapseModel = modelDescription;
        synDescription.push_back(GC2PC);
    }


    // =========================
    // 5) 配置突触学习规则
    // =========================
    LearningRuleDescription cerebullar_learning;
    cerebullar_learning.RuleName = "CerebullarLearningRule";
    cerebullar_learning.RuleParameter["fixweightchange"] = 0.002f;
    cerebullar_learning.RuleParameter["kernalchange"] = -0.0008f;
    learnDescription.push_back(cerebullar_learning);


    // =========================
    // 6) Simulation
    // =========================



    const int G_THETA_A = 0;
    const int G_DTHETA_A = 7;
    const int G_THETA_D = 14;
    const int G_DTHETA_D = 21;

    const float theta_min = -1.57;
    const float theta_max = 1.57;
    const float dth_min = -3.0;
    const float dth_max = 3.0;
    for (int i = 1; i <= 5; i++) {

        // 创建仿真模型
        Simulation* mysim;
        mysim = new Simulation(neuDescription, synDescription, learnDescription, 500, 0.1, 1);
        std::cout << "Simulation " << i << " object created" << std::endl;

        // 添加 ZMQ 输入输出驱动
        mysim->AddZMQInputOutputSpikeDriver(CLIENT, "localhost", TEST_PORT, 10);

        // 添加文件输出驱动
        mysim->AddFileOutputMonitorDriver("D:/Programing/My_Program/SNN_simulator/My_project_build/Pyplot/output_monitor.txt");
        std::cout << "File Output monitor driver added" << std::endl;
        mysim->AddFileOutputSpikeDriver("D:/Programing/My_Program/SNN_simulator/My_project_build/Pyplot/output_spike.txt");
        std::cout << "File Output spike driver added" << std::endl;
        mysim->AddFileOutputWeightDriver("D:/Programing/My_Program/SNN_simulator/My_project_build/Pyplot/Weight/outputweight.txt", 100);
        std::cout << "File Output weight driver added" << std::endl;

        mysim->InitSimulation();
        std::cout << "Simulation initialized" << std::endl;
        // 载入已有权重
        if (i > 1) {
            /*std::string filename = "E:/fengrucode/python_plot/output_weight" + std::to_string(i) + ".txt";
            mysim->LoadWeight(filename.c_str());
            std::cout << "Weight " + std::to_string(i) + " readed" << std::endl;*/
            mysim->LoadWeight("D:/Programing/My_Program/SNN_simulator/My_project_build/Pyplot/Weight/outputweight500.txt");
        }
        // 开始仿真
        mysim->RunSimulationStep(500);
        std::cout << i << " Simulation finished" << std::endl;
        delete mysim;

    }


}