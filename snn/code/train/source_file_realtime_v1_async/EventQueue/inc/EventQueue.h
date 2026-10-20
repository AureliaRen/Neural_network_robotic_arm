/*
* 文件名：EventQueue.h
* 该文件用于定义事件队列类
*/


#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H
#include "../source_file_realtime_v1_async/Event/inc/Event.h"
#include <atomic>
#include <cstdint>

// 定义结构体，其存储在事件队列中，包含一个指向事件的指针和事件发生的时间
struct Event_in_Queue
{
	Event* EventPtr;

	int time;
    std::uint64_t insertion_sequence;
};

class EventQueue {
    public:
        // FIFO is an implementation assumption for equal time/priority.
        // Separate counters add no shared non-atomic multi-queue state.
        std::atomic<std::uint64_t>* next_queue_sequence;
        std::atomic<std::uint64_t> next_sync_sequence{0};
        static bool QueueBefore(const Event_in_Queue& a, const Event_in_Queue& b) {
            if (a.time != b.time) return a.time < b.time;
            if (a.EventPtr->getPriority() != b.EventPtr->getPriority())
                return a.EventPtr->getPriority() > b.EventPtr->getPriority();
            return a.insertion_sequence < b.insertion_sequence;
        }

		Event_in_Queue** Event_Queue_vector; // 一个二维数组，用于存储事件队列，行索引为队列号，列索引为具体的 Event_in_Queue 结构体

		unsigned int* Event_Queue_vector_size; // 一个一维数组，每个队列的实际大小

		unsigned int* Event_Queue_vector_capacity; // 一个一维数组，用于存储每个队列的容量

		int NumberOfQueue; // 队列的数量

		Event_in_Queue* Event_with_syn; //一个事件队列，用于存储需要全线程同步的事件

		int NumberOfEvent_with_syn; //需要全线程同步的事件的数量

		int Event_with_syn_capacity; //事件队列的容量

		Event**** Buffers; //一个四维数组，存放事件指针，NumberOfQueue行，NumberOfQueue列，事件数量(第i个队列传向第j个队列的事件)

		int ** Buffers_size; //一个二维数组，存放事件数量，NumberOfQueue行，NumberOfQueue列

		int ** Buffers_capacity; //一个二维数组，存放事件容量，NumberOfQueue行，NumberOfQueue列

		/*
		* 构造函数，用于初始化Event_Queue_vector，Event_Queue_vector_size和Event_Queue_vector_capacity
		* numberofQueue:队列的数量
		*/

		EventQueue(int numberofQueue, bool initializeStorage = true);

		/*
		* 析构函数，用于释放Event_Queue_vector，Event_Queue_vector_size和Event_Queue_vector_capacity
		*/

		virtual ~EventQueue();



		/*
		* 用于交换Event_Queue_vector中两个Event_in_Queue结构体的函数
		* index:对应的线程索引
		* location1:第一个Event_in_Queue结构体的位置
		* location2:第二个Event_in_Queue结构体的位置
		*/

		void swap_Event(int index, int location1, int location2);

		/*
		* 用于重新分配Event_Queue_vector中某个队列的容量
		* index:对应的线程索引
		* Newsize:新的容量
		*/

		void Resize_Event_Queue(int index, int Newsize);


		/*
		* 用于在Event_Queue_vector中插入一个Event_in_Queue结构体
		* event:要插入的Event_in_Queue结构体所指向的事件指针
		* 插入的队列索引
		*/

		virtual void Insert_a_Event(Event* event, int index);


		/*
		* 用于从Event_Queue_vector的顶部移除一个事件，并返回该事件的指针
		* index:对应的线程索引
		*/

		virtual Event* Remove_a_Event(int index);

		/*
		* 用于获取Event_Queue_vector中某个队列的大小
		* index:对应的线程索引
		*/

		virtual int Get_Event_Queue_vector_size(int index);

		/*
		* 获取对应线程的第一个事件的时间
		* index:对应的线程索引
		*/
		virtual int Get_First_Event_Time(int index);

		/*
		* 交换同步事件队列中的两个事件
		* location1:第一个事件的位置
		* location2:第二个事件的位置
		*/
        void swap_syn_Event(int location1, int location2);

		/*
		* 重新分配同步事件队列的容量
		* Newsize:新的容量
		*/
        void Resize_syn_Event_Queue(int Newsize);

		/*
		* 获取同步事件队列的大小
		*/
		virtual int Get_syn_Event_Queue_vector_size();

		/*
		* 向同步事件队列中插入一个事件
		* event:要插入的Event_in_Queue结构体所指向的事件指针
		*/
		virtual void Insert_a_syn_Event(Event* event);

		/*
		* 从同步事件队列的顶部移除一个事件，并返回该事件的指针
		*/
        virtual Event* Remove_a_syn_Event();

		/*
		* 获取同步事件队列中第一个事件的时间
		*/
		virtual int Get_First_syn_Event_Time();

		/*
		* 向事件缓冲队列index1，index2中插入一个事件
		* event:要插入的事件指针
		* index1:源队列索引
		* index2:目标队列索引
		*/
		virtual void Insert_a_Event_to_Buffer(Event* event, int index1, int index2);

		/*
		* 删除目标事件队列当中的全部缓冲事件
		* index:目标队列索引
		*/
        virtual void Reset_Buffer(int index);

		/*
		* 将缓冲队列中的跨线程事件插入到对应的事件队列中
		* index:目标队列索引
		*/
		virtual void Insert_Buffer_to_Event_Queue(int index);

		/*
		* 重新分配缓冲队列的内存
		* index1:源队列索引
		* index2:目标队列索引
		*/
		void Resize_Buffer(int index1, int index2);

		/*
		* 获取缓冲队列的大小
		* index1:源队列索引
		* index2:目标队列索引
		*/
		virtual int Get_Buffer_size(int index1, int index2);

		/*
		* 获取缓冲队列的容量
		* index1:源队列索引
		* index2:目标队列索引
		*/
        virtual int Get_Buffer_capacity(int index1, int index2);

		/*
		* 增加对应的缓冲队列计数器
		* index1:源队列索引
		* index2:目标队列索引
		*/
		virtual void Increase_Buffer_size(int index1, int index2);

		/*
		* 重置对应的缓冲队列计数器
		* index1:源队列索引
		* index2:目标队列索引
		*/
		virtual void ResetSizeBuffer(int index1, int index2);

		/*
		* 检查所有缓冲队列是否为空
		* 如果是空，返回true，否则返回false
		*/
        virtual bool is_Buffer_Empty();


};

#endif


