#ifndef SIMULATION_H
#define SIMULATION_H
#include "../source_file_realtime_v1_async/Network/inc/Network.h"
#include "../source_file_realtime_v1_async/EventQueue/inc/EventQueue.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/InputSpikeDriver/inc/ArrayInputSpikeDriver.h"
#include "../source_file_realtime_v1_async/InputCurrentDriver/inc/ArrayInputCurrentDriver.h"
#include "../source_file_realtime_v1_async/communication/inc/ArrayOutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/communication/inc/FileOutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/communication/inc/FileOuterDynamicStateDriver.h"
#include "../source_file_realtime_v1_async/communication/inc/FileOutputWeightDriver.h"
#include "RealTimeRestriction.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
class OutputSpikeDriver;
class Spike;
class Neuron;
class CommunicationEvent;
class ZMQInputOutputSpikeDriver;
class ZMQAsyncInputOutputSpikeDriver;
class OuterDynamicModel;
class OuterDynamicSpikeBuffer;
class OuterDynamicStateDriver;

enum EventQueueType { EVENT_QUEUE_HEAP, EVENT_QUEUE_TIMING_WHEEL };
class Simulation {
public:
    Network* network;
    EventQueue* EventHeap;
    int totaltimesteps;
    float basetimesteps;
    int* currenttime;
    int NumberOfQueue;
    bool* SimulationEnd;
    ArrayInputSpikeDriver* inputSpikeDriver;
    ArrayInputCurrentDriver* inputCurrentDriver;
    ArrayOutputSpikeDriver* output_spike_driver;
    FileOutputSpikeDriver* file_output_spike_driver;
    FileOutputSpikeDriver* file_output_monitor_driver;
    FileOuterDynamicStateDriver* file_outer_dynamic_state_driver;
    std::list<InputSpikeDriver*> InputSpikeDriverList;
    std::list<InputCurrentDriver*> InputCurrentDriverList;
    bool* SyncThread;
    bool* PauseThread;
    int DelayMin;
    std::list<OutputSpikeDriver*> OutputSpikeDriverList;
    std::list<OutputSpikeDriver*> MonitorSpikeList;
    FileOutputWeightDriver* file_output_weight_driver;
    ZMQInputOutputSpikeDriver* zmq_input_output_spike_driver;
    ZMQAsyncInputOutputSpikeDriver* zmq_async_input_output_spike_driver;
    int WeightSaveInterval;
    int CommunivationInterval;
    std::list<OutputWeightDriver*> WeightDriverList;
    std::list<OuterDynamicStateDriver*> OuterDynamicStateDriverList;
    bool neuronMonitorExist;
    RealTimeRestriction* RealTimeRestrictionObject;
    bool RealtimeEnabled;
    int RealtimeSlotSteps;
    bool PrintSimulationTime;
    EventQueueType eventQueueType;
    int timingWheelSize;
    bool drivers_initialized;
    std::thread watchdogThread;
    std::atomic<bool> watchdogStarted{ false };
    std::list<OuterDynamicModel*> OuterDynamicModelList;
    OuterDynamicSpikeBuffer* outer_dynamic_spike_buffer;
    std::vector<int> outer_dynamic_input_times;
    std::vector<int> outer_dynamic_input_neurons;
    std::mutex outer_dynamic_input_mutex;
    void RunSimulationStep(int runsteps);
    void RunSimulationRealtime(int runsteps);
    Simulation(const std::list<NeuronLayerDescription>& neuron_layer_list, const std::list<ConnectionDescription>& connection_list, const std::list<LearningRuleDescription>& learning_rule_list, int simulationsteps, float timestep, int NumberOfQueue, EventQueueType eventQueueType = EVENT_QUEUE_HEAP, int timingWheelSize = 0);
    Simulation(const std::list<NeuronLayerDescription>& neuron_layer_list, const std::list<ConnectionDescription>& connection_list, const std::list<LearningRuleDescription>& learning_rule_list, const std::list<OuterDynamicDescription>& outer_dynamic_list, int simulationsteps, float timestep, int NumberOfQueue, EventQueueType eventQueueType = EVENT_QUEUE_HEAP, int timingWheelSize = 0);
    ~Simulation();
    void AddExternalSpikeActivity(const std::vector<int>& EventTime, const std::vector<int>& EventNeuronID);
    void AddExternalCurrentActivity(const std::vector<int>& EventTime, const std::vector<int>& EventNeuronID, const std::vector<float>& Current);
    void InitSimulation();
    void ResetForNextRound(bool preserve_weights = true);
    void EndSimulation(int QueueIndex);
    void SynchronizeThread();
    void SynchronizeThread(RealTimeRestrictionLevel restrictionLevel);
    void AddOutputSpikeDriver(OutputSpikeDriver* OutputSpikeDriver);
    void WriteSpike(Spike* spike);
    void WriteState(int time, Neuron* neuron);
    void SetOpenMPThreadToGPU(int openMpID);
    int GetWeightSaveStep();
    void AddInputSpikeDriver(InputSpikeDriver* newInputSpikeDriver);
    void AddInputCurrentDriver(InputCurrentDriver* newInputCurrentDriver);
    void AddOutputMonitorDriver(OutputSpikeDriver* OutputMonitorDriver);
    void AddOutputWeightDriver(OutputWeightDriver* WeightDriver);
    void AddOuterDynamicStateDriver(OuterDynamicStateDriver* driver);
    void AddFileOutputSpikeDriver(std::string FileName);
    void AddFileOutputMonitorDriver(std::string FileName);
    void AddFileOuterDynamicStateDriver(std::string FileName);
    void AddFileOutputWeightDriver(std::string FileName, int WeightSaveInterval);
    void AddZMQInputOutputSpikeDriver(enum DriverType Type, std::string address, unsigned short port, int CommunicationInterval);
    void AddZMQAsyncInputOutputSpikeDriver(std::string subscribeAddress, unsigned short publishPort, unsigned short subscribePort, std::string publishTopic, std::string subscribeTopic, int CommunicationInterval);
    void SaveWeight();
    void PublishOutput(CommunicationEvent* c_event);
    void LoadInput(CommunicationEvent* c_event);
    void LoadWeight(const char* filename);
    void SaveWeightToFile(const char* filename);
    void EnableRealtime(int slotSteps, double maxAdvanceSeconds, float firstSection, float secondSection, float thirdSection);
    void DisableRealtime();
    void SetPrintSimulationTime(bool enable);
    void CreateOuterDynamic(const std::list<OuterDynamicDescription>& outer_dynamic_list);
    void QueueOuterDynamicInputSpike(int event_time, int neuron_id);
    void FlushOuterDynamicInputSpikes(int current_time);
    void WriteOuterDynamicState(int time_step, const OuterDynamicJointState& state);
    void BindDriversOnce();
    void ScheduleInitialEvents();
    void RecreateEventQueue();
};

#endif
