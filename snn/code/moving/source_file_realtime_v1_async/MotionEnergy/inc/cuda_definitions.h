#ifndef _CUDA_DEFINITIONS_H
#define _CUDA_DEFINITIONS_H

#include <cstdio>
#include <cstdlib>

#include <cuda.h>
#include <cuda_runtime.h>

inline void motion_energy_check_cuda(cudaError_t result, const char* expr, const char* file, int line) {
	if (result != cudaSuccess) {
		std::fprintf(stderr, "CUDA error at %s:%d for %s: %s\n", file, line, expr, cudaGetErrorString(result));
		std::fflush(stderr);
		std::abort();
	}
}

inline void motion_energy_check_last_error(const char* message, const char* file, int line) {
	cudaError_t result = cudaGetLastError();
	if (result != cudaSuccess) {
		std::fprintf(stderr, "CUDA kernel error at %s:%d: %s: %s\n", file, line, message, cudaGetErrorString(result));
		std::fflush(stderr);
		std::abort();
	}
}

#define CUDA_CHECK_ERRORS(x) motion_energy_check_cuda((x), #x, __FILE__, __LINE__)
#define CUDA_CHECK_ERRORS_MACRO(x) motion_energy_check_cuda((x), #x, __FILE__, __LINE__)
#define CUDA_GET_LAST_ERROR(x) motion_energy_check_last_error((x), __FILE__, __LINE__)
#define CUDA_GET_LAST_ERROR_MACRO(x) motion_energy_check_last_error((x), __FILE__, __LINE__)

#endif
