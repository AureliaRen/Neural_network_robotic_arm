#ifndef OUTER_DYNAMIC_MODEL_H
#define OUTER_DYNAMIC_MODEL_H

#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

class Simulation;

class OuterDynamicModel {
public:
    OuterDynamicModel();
    explicit OuterDynamicModel(int timestep_size);
    virtual ~OuterDynamicModel();

    virtual void Initialize(const OuterDynamicDescription& description, Simulation* simulation) = 0;
    virtual void Update(int time, Simulation* simulation) = 0;

    int getTimestepSize() const;
    void setTimestepSize(int timestep_size);

    int getQueueIndex() const;
    void setQueueIndex(int queue_index);

    int getCommunicationInterval() const;
    void setCommunicationInterval(int interval);

protected:
    int timestep_size_;
    int queue_index_;
    int communication_interval_;
};

#endif
