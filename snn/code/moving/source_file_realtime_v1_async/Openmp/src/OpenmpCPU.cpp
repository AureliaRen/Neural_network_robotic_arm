#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include <iostream>

int NumberOfOpenMPQueues;
int NumberOfGPU;
int* GPUIndex;

void setNumberOfOpenMPQueues(int n) {
    if (n > omp_get_max_threads()) {
        n = omp_get_max_threads();
        std::cout << "Warning: Number of OpenMP queues is greater than the number of CPU cores. "
                     "Setting it to the number of CPU cores."
                  << n << std::endl;
    }
#ifdef _OPENMP
    std::cout << "OpenMP support is enabled" << std::endl;
#else
    std::cout << "OpenMP support is disabled" << std::endl;
#endif
    (void)omp_set_num_threads(n);
    NumberOfOpenMPQueues = n;
    NumberOfGPU = 0;
    GPUIndex = nullptr;
    std::cout << "CPU-only package: CUDA/GPU support disabled." << std::endl;
}
