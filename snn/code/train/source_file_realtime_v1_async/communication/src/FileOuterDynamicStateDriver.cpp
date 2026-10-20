#include "../source_file_realtime_v1_async/communication/inc/FileOuterDynamicStateDriver.h"

#include <filesystem>
#include <stdexcept>

FileOuterDynamicStateDriver::FileOuterDynamicStateDriver(const char* file_name)
    : filename_(file_name), handler_(NULL) {
    std::filesystem::path file_path(file_name);
    std::filesystem::path parent_path = file_path.parent_path();
    if (!parent_path.empty()) {
        std::filesystem::create_directories(parent_path);
    }

    this->handler_ = fopen(file_name, "wt");
    if (this->handler_ == NULL) {
        throw std::runtime_error(std::string("Failed to open outer dynamic state file: ") + file_name);
    }

    fprintf(
        this->handler_,
        "time_s\tq1\tq2\tqv1\tqv2\tqdes1\tqdes2\tqvdes1\tqvdes2\ttau1\ttau2\n");
}

FileOuterDynamicStateDriver::~FileOuterDynamicStateDriver() {
    if (this->handler_ != NULL) {
        fclose(this->handler_);
        this->handler_ = NULL;
    }
}

void FileOuterDynamicStateDriver::WriteJointState(
    int time_step,
    float base_timestep_ms,
    const OuterDynamicJointState& state) {
#pragma omp critical (FileOuterDynamicStateDriver)
    {
        if (this->handler_ == NULL) {
            throw std::runtime_error(std::string("Outer dynamic state file handle is null: ") + this->filename_);
        }
        const double time_seconds = static_cast<double>(time_step) * static_cast<double>(base_timestep_ms) * 0.001;
        fprintf(
            this->handler_,
            "%.9f\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\n",
            time_seconds,
            state.q[0], state.q[1],
            state.qv[0], state.qv[1],
            state.q_des[0], state.q_des[1],
            state.qv_des[0], state.qv_des[1],
            state.tau_total[0], state.tau_total[1]);
    }
}

void FileOuterDynamicStateDriver::FlushBuffers() {
    if (this->handler_ != NULL) {
        fflush(this->handler_);
    }
}
