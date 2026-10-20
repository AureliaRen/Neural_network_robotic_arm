#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include <cuda_runtime.h>
#include <iostream>

int NumberOfOpenMPQueues;//CPU线程数
int NumberOfGPU;//GPU个数
int* GPUIndex;//GPU索引列表指针

/*
* 设置 OpenMP 线程数
*/
void setNumberOfOpenMPQueues(int n) {
	// 设置 CPU 线程数
	// 判断是否超过 CPU 核心数
	if (n > omp_get_max_threads()) {
		n = omp_get_max_threads();
		std::cout << "Warning: Number of OpenMP queues is greater than the number of CPU cores. Setting it to the number of CPU cores." << n << std::endl;
	}
    #ifdef _OPENMP
	    std::cout << "OpenMP support is enabled" << std::endl;
    #else
	    std::cout << "OpenMP support is disabled" << std::endl;
    #endif
	(void) omp_set_num_threads(n);
	NumberOfOpenMPQueues = n;

	
	// 获取 GPU 数量
	int n_Device = 0; //GPU个数
	int validDevice = 0; //有效GPU个数(要求计算能力>=2.0，即CUDA版本大于6.5)
	// 读取 GPU 数量
    cudaGetDeviceCount(&n_Device);
	if (n_Device == 0) {
        std::cout << "Error: No GPU devices found." << std::endl;
		NumberOfGPU = 0;
	}
	else {
		GPUIndex = new int[n_Device]();
		for (int i = 0; i < n_Device; i++) {
			cudaDeviceProp deviceProp;
			cudaGetDeviceProperties(&deviceProp, i);
			if (deviceProp.major >= 2) {
				GPUIndex[validDevice] = i;
				validDevice++;
			}
		}
		if (validDevice == 0) {
			std::cout << "Error: No valid GPU devices found." << std::endl;
		}

		if (validDevice > NumberOfOpenMPQueues) {
			NumberOfGPU = NumberOfOpenMPQueues;
		}
		else {
			NumberOfGPU = validDevice;
		}
	}

}