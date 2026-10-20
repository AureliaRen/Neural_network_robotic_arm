#include "../source_file_realtime_v1_async/OuterDynamic/inc/OuterDynamicModel.h"

OuterDynamicModel::OuterDynamicModel() : timestep_size_(1), queue_index_(0), communication_interval_(1) {}

OuterDynamicModel::OuterDynamicModel(int timestep_size)
    : timestep_size_(timestep_size), queue_index_(0), communication_interval_(1) {}

OuterDynamicModel::~OuterDynamicModel() {}

int OuterDynamicModel::getTimestepSize() const {
    return this->timestep_size_;
}

void OuterDynamicModel::setTimestepSize(int timestep_size) {
    this->timestep_size_ = timestep_size;
}

int OuterDynamicModel::getQueueIndex() const {
    return this->queue_index_;
}

void OuterDynamicModel::setQueueIndex(int queue_index) {
    this->queue_index_ = queue_index;
}

int OuterDynamicModel::getCommunicationInterval() const {
    return this->communication_interval_;
}

void OuterDynamicModel::setCommunicationInterval(int interval) {
    this->communication_interval_ = interval;
}
