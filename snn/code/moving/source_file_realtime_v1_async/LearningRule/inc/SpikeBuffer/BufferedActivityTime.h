/*
* 文件名：BufferedActivityTime.h
* 该类用于存储时间窗口当中的放电
*/

#ifndef BUFFERED_ACTIVITY_TIME_H
#define BUFFERED_ACTIVITY_TIME_H

#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"

struct SpikeData {
	float time;
	int synapticID;
};

struct BufferedActivityTimesData {
	int size;
	int N_elements; //有效放电的个数
	int first_element; //缓冲区中第一个元素的位置(最新加入的元素位置)
	int last_element;  //缓冲区中最后一个元素的位置(最早加入的元素位置 - 1)
	SpikeData* SpikeData; //神经元的放电缓冲区
};

class BufferedActivityTime {
	public:
		int BufferSize; //缓冲区大小

		BufferedActivityTimesData* structure;

		int* size_output_array;

		SpikeData** output_spike_data;

		/*
		* 构造函数
		*/
		BufferedActivityTime(int newsize);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~BufferedActivityTime();

		/*
		* 内联函数：向缓冲区中添加数据
		* neuronID: 神经元ID
		* time: 放电时间
		* thresholdtime: 时间窗口
		* synapticID: 放电突触ID
		*/
		inline void InsetElement(int neuronID, float time, float thresholdtime, int synapticID) {
			//检查扩容
			DuplicateData(neuronID, thresholdtime);
			//更新相应神经元的放电元素计数器
			structure[neuronID].N_elements++;
			// 更新最新元素的位置
			structure[neuronID].first_element++;
			//如果指向列表末尾，则指向列表开头
			if (structure[neuronID].first_element == structure[neuronID].size) {
				structure[neuronID].first_element = 0;
			}
			// 更新最新元素的数据
			structure[neuronID].SpikeData[structure[neuronID].first_element].time = time;
			structure[neuronID].SpikeData[structure[neuronID].first_element].synapticID = synapticID;

		}

		inline int ProcessElement(int neuronID, float thresholdtime) {
			if (structure[neuronID].N_elements > 0) {
				int OpenmpID = omp_get_thread_num();
				//检查输出数组是否需要扩容
				if (structure[neuronID].N_elements > size_output_array[OpenmpID]) {
					this->size_output_array[OpenmpID] = structure[neuronID].N_elements;
					delete [] output_spike_data[OpenmpID];
					output_spike_data[OpenmpID] = new SpikeData[structure[neuronID].N_elements];
				}
				int i = structure[neuronID].first_element;
				int counter = 0;
				//从缓冲区中提取有效放电
				while (i != structure[neuronID].last_element) {
					if (structure[neuronID].SpikeData[i].time > thresholdtime) {
						//判断时间是否越过时间窗口阈值
						output_spike_data[OpenmpID][counter].time = structure[neuronID].SpikeData[i].time;
						output_spike_data[OpenmpID][counter].synapticID = structure[neuronID].SpikeData[i].synapticID;
						counter++;
						//如果指向列表开始，则指向列表末尾
						if (i == 0) {
							i = structure[neuronID].size;
						}
						i--;
					}
					else {
						//如果指向了最早加入的元素，则退出循环
						structure[neuronID].last_element = i;
						structure[neuronID].N_elements = counter;
						break;
					}
				}
			}
			return structure[neuronID].N_elements;
		}

		/*
		* 内联函数：缓冲区扩容
		*/
		inline void DuplicateData(int index, float ThrethholdTime) {
			//检查扩容
			if (structure[index].size == (structure[index].N_elements + 1)) {
				//濡傛灉last_element鍦ㄩ槦灏撅紝鍒欏皢last_element鎸囧悜闃熼
				int last_element = structure[index].last_element + 1;
				if (last_element == structure[index].size) {
					last_element = 0;
				}
				//如果last_element是可丢弃的
				if (structure[index].SpikeData[last_element].time < ThrethholdTime) {
					int i = structure[index].first_element - structure[index].size / 2;
					if (i < 0) {
						i += structure[index].size;
					}
					int counter = structure[index].size / 2;
					while (i != structure[index].last_element) {
						if (structure[index].SpikeData[i].time > ThrethholdTime) {
							counter++;
							if (i == 0) {
								i = structure[index].size;
							}
							i--;
						}
						else {
							structure[index].last_element = i;
							structure[index].N_elements = counter;
							break;
						}
					}
				}
				else {//如果last_element不可丢弃，则进行扩容
					SpikeData* aux_data = structure[index].SpikeData;
					//容量翻倍
					structure[index].SpikeData = new SpikeData[structure[index].size * 2];
					int i = structure[index].last_element;
					int counter = 0;
					// 从最早的一个元素开始，将数据复制到新的缓冲区中
					do {
						i++;
						if (i == structure[index].size) {
							i = 0;
						}
						structure[index].SpikeData[counter].time = aux_data[i].time;
						structure[index].SpikeData[counter].synapticID = aux_data[i].synapticID;
						counter++;
					}while (i != structure[index].first_element);
					structure[index].size *= 2;
					structure[index].first_element = counter - 1;
					structure[index].last_element = structure[index].size - 1;
					delete [] aux_data;
				}
			}
		};

		/*
		* 鍐呰仈鍑芥暟锛氳幏鍙栬緭鍑虹紦鍐插尯
		*/
		inline SpikeData* GetOutputSpikeData() {
			return output_spike_data[omp_get_thread_num()];
		}

};



#endif // BUFFERED_ACTIVITY_TIME_H
