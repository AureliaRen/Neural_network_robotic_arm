#include "../source_file_realtime_v1_async/communication/inc/FileOutputWeightDriver.h"
#include "../source_file_realtime_v1_async/Network/inc/Network.h"
#include <string>

FileOutputWeightDriver::FileOutputWeightDriver(const char* newfilename): OutputWeightDriver() {
	this->filename = newfilename;
}

FileOutputWeightDriver::~FileOutputWeightDriver() {}


void FileOutputWeightDriver::WriteWeight(Network* newnetwork, int time) {
	//鏍规嵁鏃堕棿鎴崇敓鎴愭枃浠跺悕
	std::string name = this->filename;
    size_t dot_pos = name.find_last_of('.');

    // 2. 将整数 time 转换为字符串
    std::string time_str = std::to_string(time);

    // 3. 插入时间字符串
    // 如果没找到点，就直接拼接到末尾
    if (dot_pos != std::string::npos) {
        name.insert(dot_pos, time_str);
    }
    else {
        name += time_str;
    }
	newnetwork->SaveWeightsToFile(name.c_str());
}