#include "../source_file_realtime_v1_async/EventQueue/inc/EventQueue.h"
#include "../source_file_realtime_v1_async/Event/inc/Event.h"
#include "../source_file_realtime_v1_async/mainprogram/inc/BenchProfiling.h"


EventQueue::EventQueue(int numberofQueue, bool initializeStorage) : NumberOfQueue(numberofQueue) {
	this->next_queue_sequence = nullptr;
	this->Event_Queue_vector = NULL;
	this->Event_Queue_vector_size = NULL;
	this->Event_Queue_vector_capacity = NULL;
	this->Event_with_syn = NULL;
	this->NumberOfEvent_with_syn = 0;
	this->Event_with_syn_capacity = 0;
	this->Buffers = NULL;
	this->Buffers_size = NULL;
	this->Buffers_capacity = NULL;

	if (!initializeStorage) {
		return;
	}
	// 为成员变量分配内存
	// 给普通事件队列分配内存
	this->next_queue_sequence = new std::atomic<std::uint64_t>[NumberOfQueue]();
	this->Event_Queue_vector_size = new unsigned int[NumberOfQueue]();
	this->Event_Queue_vector_capacity = new unsigned int[NumberOfQueue]();
	this->Event_Queue_vector = new Event_in_Queue*[NumberOfQueue];

	for (int i = 0; i < NumberOfQueue; i++) {
		this->Event_Queue_vector_capacity[i] = 1024;
        this->Event_Queue_vector[i] = new Event_in_Queue[1024]();
		this->Event_Queue_vector_size[i] = 1;

	}

	//给同步事件队列分配内存
	this->Event_with_syn = new Event_in_Queue[1024]();
	this->NumberOfEvent_with_syn = 1;
	this->Event_with_syn_capacity = 1024;

	//给缓冲队列分配内存
	this->Buffers_capacity = new int*[NumberOfQueue];
	this->Buffers_size = new int*[NumberOfQueue]();
	this->Buffers = new Event***[NumberOfQueue];
	for (int i = 0; i < NumberOfQueue; i++) {
		this->Buffers_capacity[i] = new int[NumberOfQueue];
		this->Buffers_size[i] = new int[NumberOfQueue]();
		this->Buffers[i] = new Event**[NumberOfQueue];
		for (int j = 0; j < NumberOfQueue; j++) {
			this->Buffers_capacity[i][j] = 1024;
			this->Buffers[i][j] = new Event*[1024]();
		}
	}

}

EventQueue::~EventQueue() {
	delete[] this->next_queue_sequence;
	if (this->Event_Queue_vector != NULL && this->Event_Queue_vector_size != NULL) {
		for (int i = 0; i < this->NumberOfQueue; i++) {
			for (int j = 1; j < this->Event_Queue_vector_size[i]; j++) {
				delete (this->Event_Queue_vector[i] + j)->EventPtr;
			}
	        delete[] this->Event_Queue_vector[i];
		}
		delete[] this->Event_Queue_vector;
		delete[] this->Event_Queue_vector_size;
		delete[] this->Event_Queue_vector_capacity;
	}

	//释放同步事件队列内存
	if (this->Event_with_syn != NULL) {
		for (int i = 1; i < this->NumberOfEvent_with_syn; i++) {
			delete (this->Event_with_syn + i)->EventPtr;
		}
		delete[] this->Event_with_syn;
	}

	//闁插﹥鏂佺紓鎾冲暱闂冪喎鍨崘鍛摠
	if (this->Buffers != NULL && this->Buffers_size != NULL && this->Buffers_capacity != NULL) {
		for (int i = 0; i < this->NumberOfQueue; i++) {
			for (int j = 0; j < this->NumberOfQueue; j++) {
				for (int k = 0; k < this->Buffers_size[i][j]; k++) {
					delete this->Buffers[i][j][k];
				}
				delete[] this->Buffers[i][j];
			}
			delete[] this->Buffers[i];
			delete[] this->Buffers_capacity[i];
			delete[] this->Buffers_size[i];
		}
		delete[] this->Buffers;
	    delete[] this->Buffers_capacity;
		delete[] this->Buffers_size;
	}

	this->Event_Queue_vector = NULL;
    this->Event_Queue_vector_size = NULL;
    this->Event_Queue_vector_capacity = NULL;
}


void EventQueue::swap_Event(int index, int location1, int location2) {
    Event_in_Queue temp;
	temp = *(this->Event_Queue_vector[index] + location1);
	*(this->Event_Queue_vector[index] + location1) = *(this->Event_Queue_vector[index] + location2);
    *(this->Event_Queue_vector[index]+location2) = temp;
}

void EventQueue::Resize_Event_Queue(int index, int Newsize) {
	Event_in_Queue* temp;
	temp = this->Event_Queue_vector[index];
    this->Event_Queue_vector[index] = new Event_in_Queue[Newsize]();
	this->Event_Queue_vector_capacity[index] = Newsize;
	//将原来的数据复制到新的数组中
	for (int i = 0; i < Event_Queue_vector_size[index]; i++) {
        *(this->Event_Queue_vector[index] + i) = *(temp + i);
	}
	delete[] temp;
	temp = NULL;
}


void EventQueue::Insert_a_Event(Event* event, int index) {
	//int index = event->getIndex();
	//检查是否需要扩容
	if (this->Event_Queue_vector_size[index] == this->Event_Queue_vector_capacity[index]) {
        Resize_Event_Queue(index, 2 * this->Event_Queue_vector_capacity[index]);
	}
	//插入事件到堆底
	(this->Event_Queue_vector[index] + this->Event_Queue_vector_size[index])->EventPtr = event;
	(this->Event_Queue_vector[index] + this->Event_Queue_vector_size[index])->time = event->getTime();
    (this->Event_Queue_vector[index] + this->Event_Queue_vector_size[index])->insertion_sequence =
        this->next_queue_sequence[index].fetch_add(1, std::memory_order_relaxed);
	this->Event_Queue_vector_size[index]++;

	//根据事件时间调整堆顺序
	for (int i = Event_Queue_vector_size[index] - 1;
		(i > 1) && QueueBefore(this->Event_Queue_vector[index][i], this->Event_Queue_vector[index][i / 2]);
		i = i / 2) {
		swap_Event(index, i, i / 2);
	}
	return;


}

Event* EventQueue::Remove_a_Event(int index) {
    const long long start_ns = bench_profile::now_ns();
	Event* first_event = NULL;

	if (this->Event_Queue_vector_size[index] > 2) {
		first_event = (this->Event_Queue_vector[index] + 1)->EventPtr;
		*(this->Event_Queue_vector[index] + 1) = *(this->Event_Queue_vector[index] + this->Event_Queue_vector_size[index] - 1);
		this->Event_Queue_vector_size[index]--;
		if (this->Event_Queue_vector_size[index] < this->Event_Queue_vector_capacity[index] / 4 && this->Event_Queue_vector_size[index] > 1024) {
			Resize_Event_Queue(index, this->Event_Queue_vector_capacity[index] / 2);
		}
		int tree_father_count = 1;
		int tree_son_count = 1;
		for (tree_son_count = 2 * tree_father_count; tree_son_count < this->Event_Queue_vector_size[index] - 1; tree_father_count = tree_son_count, tree_son_count = tree_son_count * 2) {
			if (QueueBefore(this->Event_Queue_vector[index][tree_son_count + 1], this->Event_Queue_vector[index][tree_son_count])) {
				tree_son_count++;
			}
			if (QueueBefore(this->Event_Queue_vector[index][tree_son_count], this->Event_Queue_vector[index][tree_father_count])) {
				swap_Event(index, tree_father_count, tree_son_count);
			}
			else {
				break;
			}
		}
		if (tree_son_count == this->Event_Queue_vector_size[index] - 1) {
			if (QueueBefore(this->Event_Queue_vector[index][tree_son_count], this->Event_Queue_vector[index][tree_father_count])) {
				swap_Event(index, tree_father_count, tree_son_count);
			}
		}
	}
	else if (this->Event_Queue_vector_size[index] == 2) {
		first_event = (this->Event_Queue_vector[index] + 1)->EventPtr;
		this->Event_Queue_vector_size[index]--;
	}
    bench_profile::event_remove_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
    bench_profile::remove_count.fetch_add(1, std::memory_order_relaxed);
	return first_event;
}


int EventQueue::Get_Event_Queue_vector_size(int index) {
	return this->Event_Queue_vector_size[index] - 1;
}

int EventQueue::Get_First_Event_Time(int index) {
	//检查是否存在有效事件
	int time;
	if (this->Event_Queue_vector_size[index] > 1) {
		time = (this->Event_Queue_vector[index] + 1)->time;
	}
	else {
		time = -1;
	}
	return time;

}


void EventQueue::swap_syn_Event(int index1, int index2) {
	//中间交换变量
	Event_in_Queue exchange;
	//交换
	exchange = *(this->Event_with_syn + index1);

	*(this->Event_with_syn + index1) = *(this->Event_with_syn + index2);

	* (this->Event_with_syn + index2) = exchange;

}

void EventQueue::Resize_syn_Event_Queue(int new_capacity) {
	//中间变量
	Event_in_Queue* Temp;
	Temp = this->Event_with_syn;
	//内存扩容
	this->Event_with_syn = new Event_in_Queue[new_capacity]();
	this->Event_with_syn_capacity = new_capacity;
	//内存转移
	for (int i = 0; i < this->NumberOfEvent_with_syn; i++) {
		*(this->Event_with_syn + i) = *(Temp + i);
	}

	delete[] Temp;
}

int EventQueue::Get_syn_Event_Queue_vector_size() {
	return this->NumberOfEvent_with_syn - 1;
}

void EventQueue::Insert_a_syn_Event(Event* event) {
	//检查是否需要扩容
	if (this->NumberOfEvent_with_syn == this->Event_with_syn_capacity) {
		this->Resize_syn_Event_Queue(this->Event_with_syn_capacity * 2);
	}

	//閹绘帒鍙嗘禍瀣╂
	//将event插入到堆底
	(this->Event_with_syn + this->NumberOfEvent_with_syn)->EventPtr = event;
	(this->Event_with_syn + this->NumberOfEvent_with_syn)->time = event->getTime();
    (this->Event_with_syn + this->NumberOfEvent_with_syn)->insertion_sequence =
        this->next_sync_sequence.fetch_add(1, std::memory_order_relaxed);
	this->NumberOfEvent_with_syn++;
	//閸氭垳绗傞崘鎺撳満
	for (int c = this->Get_syn_Event_Queue_vector_size(); c > 1 && QueueBefore(this->Event_with_syn[c], this->Event_with_syn[c / 2]); c /= 2) {
		swap_syn_Event(c, c / 2);
	}

}

Event* EventQueue::Remove_a_syn_Event() {
	int c, p;
	//閸棝銆婃禍瀣╂
	Event* first_event = NULL;
	//如果存在大于一个
	if (this->NumberOfEvent_with_syn > 2) {
		// 维护堆的有序性
		first_event = (this->Event_with_syn + 1)->EventPtr;
		//将堆底事件放到堆顶
		*(this->Event_with_syn + 1) = *(this->Event_with_syn + this->Get_syn_Event_Queue_vector_size());
		//更新事件计数
		this->NumberOfEvent_with_syn--;
		//检查是否需要缩容
		if (this->NumberOfEvent_with_syn < this->Event_with_syn_capacity / 4 && this->NumberOfEvent_with_syn > 1024) {
			this->Resize_syn_Event_Queue(this->Event_with_syn_capacity / 2);
		}
		p = 1;
		for (c = p * 2; c < this->NumberOfEvent_with_syn; p = c, c = p * 2) {
			//比较左右子节点的时间与优先级
			if (QueueBefore(this->Event_with_syn[c + 1], this->Event_with_syn[c])) {
				//移动到右节点
				c++;
			}
			//比较父节点和子节点的时间与优先级
			if (QueueBefore(this->Event_with_syn[c], this->Event_with_syn[p])) {
				//交换
				swap_syn_Event(p, c);
			}
			else {
				break;
			}
		}
		//如果最终子节点只有一个
		if (c == this->Get_syn_Event_Queue_vector_size()) {
			//比较父节点和子节点的时间与优先级
			if (QueueBefore(this->Event_with_syn[c], this->Event_with_syn[p])) {
				swap_syn_Event(p, c);
			}
		}
	}
	else if (this->NumberOfEvent_with_syn == 2) {    //此时队列中拥有一个有效事件
		first_event = (this->Event_with_syn + 1)->EventPtr;
		this->NumberOfEvent_with_syn--;
	}
	return first_event;
}

int EventQueue::Get_First_syn_Event_Time() {
	int time;
	if (this->NumberOfEvent_with_syn > 1) {
		time = (this->Event_with_syn + 1)->time;
	}
	else {
		time = -1;
	}
	return time;
}

void EventQueue::Insert_a_Event_to_Buffer(Event* event, int index1, int index2) {
    const long long start_ns = bench_profile::now_ns();
	//检查是否需要扩容
	if (this->Buffers_size[index1][index2] == this->Buffers_capacity[index1][index2]) {
		this->Resize_Buffer(index1, index2);
	}
	this->Buffers[index1][index2][this->Buffers_size[index1][index2]] = event;
	this->Increase_Buffer_size(index1, index2);
    bench_profile::event_buffer_insert_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

void EventQueue::Reset_Buffer(int index) {
	for (int i = 0; i < NumberOfQueue; i++) {
		for (int j = 0; j < this->Buffers_size[i][index]; j++) {
			//将目标缓冲区中的事件全部删除
			delete this->Buffers[i][index][j];
		}
		this->ResetSizeBuffer(i, index);
	}
}

void EventQueue::ResetSizeBuffer(int index1, int index2) {
	this->Buffers_size[index1][index2] = 0;
}

void EventQueue::Resize_Buffer(int index1, int index2) {
	//创建中间变量
	Event** Temp = this->Buffers[index1][index2];
	//扩容
	this->Buffers[index1][index2] = new Event*[this->Buffers_capacity[index1][index2] * 2];
	//复制中间变量
	for (int i = 0; i < this->Buffers_size[index1][index2]; i++) {
		this->Buffers[index1][index2][i] = Temp[i];
	}

	delete[] Temp;
	this->Buffers_capacity[index1][index2] *= 2;
}

void EventQueue::Insert_Buffer_to_Event_Queue(int index) {
    const long long start_ns = bench_profile::now_ns();
	for (int i = 0; i < NumberOfQueue; i++) {
		for (int j = 0; j < this->Buffers_size[i][index]; j++) {
			//将目标缓冲区中的事件全部插入到事件队列
			this->Insert_a_Event(this->Buffers[i][index][j], index);
		}
		//重置缓冲区计数
		this->ResetSizeBuffer(i, index);
	}
    bench_profile::event_buffer_flush_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

int EventQueue::Get_Buffer_size(int index1, int index2) {
	return this->Buffers_size[index1][index2];
}

int EventQueue::Get_Buffer_capacity(int index1, int index2) {
	return this->Buffers_capacity[index1][index2];
}

void EventQueue::Increase_Buffer_size(int index1, int index2) {
	this->Buffers_size[index1][index2]++;
}

bool EventQueue::is_Buffer_Empty() {
	for (int i = 0; i < NumberOfQueue; i++) {
		for (int j = 0; j < NumberOfQueue; j++) {
			if (this->Buffers_size[i][j] > 0) {
				return false;
			}
		}
	}
	return true;
}



