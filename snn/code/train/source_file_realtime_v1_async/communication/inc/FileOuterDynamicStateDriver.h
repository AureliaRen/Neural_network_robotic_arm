/*
* 文件名: FileOuterDynamicStateDriver.h
* 将2DOF机械臂关节状态写入文件
*/

#ifndef FILEOUTERDYNAMICSTATEDRIVER_H
#define FILEOUTERDYNAMICSTATEDRIVER_H

#include "../source_file_realtime_v1_async/communication/inc/OuterDynamicStateDriver.h"

#include <cstdio>
#include <string>

class FileOuterDynamicStateDriver : public OuterDynamicStateDriver {
public:
    explicit FileOuterDynamicStateDriver(const char* file_name);
    virtual ~FileOuterDynamicStateDriver();

    virtual void WriteJointState(
        int time_step,
        float base_timestep_ms,
        const OuterDynamicJointState& state) override;

    virtual void FlushBuffers() override;

private:
    std::string filename_;
    FILE* handler_;
};

#endif
