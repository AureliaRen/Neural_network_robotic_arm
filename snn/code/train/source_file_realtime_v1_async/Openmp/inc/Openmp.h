/*
* 文件名：Openmp.h
* 用于定义Openmp相关的多线程相关宏
*/
#ifndef OPENMP_H_
#define OPENMP_H_


extern int NumberOfOpenMPQueues;
extern int NumberOfGPU;
extern int* GPUIndex;


#include <omp.h>


void setNumberOfOpenMPQueues(int n);



#endif /*OPENMP_H_*/
