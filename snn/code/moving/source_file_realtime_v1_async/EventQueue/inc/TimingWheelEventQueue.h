#ifndef TIMING_WHEEL_EVENT_QUEUE_H
#define TIMING_WHEEL_EVENT_QUEUE_H

#include "../source_file_realtime_v1_async/EventQueue/inc/EventQueue.h"
#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

class TimingWheelEventQueue : public EventQueue {
public:
	/*
	* 构造函数
	* numberofQueue: 队列数量
	*/
	TimingWheelEventQueue(int numberofQueue, int wheelSize);
	/*
	* 析构函数
	*/
	virtual ~TimingWheelEventQueue();
	/*
	* 插入一个事件
	*/
	virtual void Insert_a_Event(Event* event, int index);
	/*
	* 移除一个事件
	*/
	virtual Event* Remove_a_Event(int index);
	/*
	* 获取index队列的大小
	*/
	virtual int Get_Event_Queue_vector_size(int index);
	/*
	* 获取第一个事件的时间
	*/
	virtual int Get_First_Event_Time(int index);
	/*
	* 获取同步队列的大小
	*/
	virtual int Get_syn_Event_Queue_vector_size();
	/*
	* 插入同步事件
	*/
	virtual void Insert_a_syn_Event(Event* event);
	/*
	* 移除同步事件
	*/
	virtual Event* Remove_a_syn_Event();
	/*
	* 获取同步队列中第一个事件的时间
	*/
	virtual int Get_First_syn_Event_Time();
	/*
	* 将跨线程时间插入缓冲堆
	*/
	virtual void Insert_a_Event_to_Buffer(Event* event, int index1, int index2);
	/*
	* 重置缓冲区
	*/
	virtual void Reset_Buffer(int index);
	/*
	* 将跨线程事件插入到事件队列中
	*/
	virtual void Insert_Buffer_to_Event_Queue(int index);
	/*
	* 获取缓冲区的大小
	*/
	virtual int Get_Buffer_size(int index1, int index2);
	/*
	* 获取缓冲区中事件的数量
	*/
	virtual int Get_Buffer_capacity(int index1, int index2);
	/*
	* 增加缓冲区计数
	*/
	virtual void Increase_Buffer_size(int index1, int index2);
	/*
	* 重置缓冲区
	*/
	virtual void ResetSizeBuffer(int index1, int index2);
	virtual bool is_Buffer_Empty();

private:
	static const int PriorityCount = PROPOGATEDCURRENT + 1;
	//定义了每个事件桶当中的事件向量结构体
	struct PriorityBucket {
		//数组有 PriorityCount 个元素，每个元素都是一个 std::vector<Event*>
		std::vector<Event*> events[PriorityCount];
	};
	//定义了优先级比较函数
	struct EventComesAfter {
		bool operator()(Event* left, Event* right) const;
	};
	// 定义了一种长期事件的存储Heap队列类型
	typedef std::priority_queue<Event*, std::vector<Event*>, EventComesAfter> FarEventQueue;
	// 时间桶的大小
	int wheel_size;
	int wheel_mask;
	bool wheel_size_is_power_of_two;
	// 定义了时间转筒的槽位
	std::vector<std::vector<PriorityBucket> > queue_buckets;
    // 定义了长期事件的存储Heap队列组
	std::vector<FarEventQueue> queue_far_events;
	// 每个时间桶，每个优先级的元素数量
	std::vector<std::vector<int> > queue_bucket_counts;
	// 每个时间桶当前非空优先级的位图
	std::vector<std::vector<std::uint32_t> > queue_bucket_priority_masks;
	// 每个时间桶当前对应的绝对时间，-1 表示空桶
	std::vector<std::vector<int> > queue_bucket_times;
	// 每个队列的当前时间
	std::vector<int> queue_current_time;
	std::vector<int> queue_current_slot;
	// 每个队列的大小
	std::vector<int> queue_size;
    // 每个队列的近期时间队列大小
	std::vector<int> queue_near_size;
	// 同步桶
	std::vector<PriorityBucket> syn_buckets;
	// 远期事件同步桶
	FarEventQueue syn_far_events;
	// 同步桶事件数量
	std::vector<int> syn_bucket_counts;
	std::vector<std::uint32_t> syn_bucket_priority_masks;
	std::vector<int> syn_bucket_times;
	int syn_current_time;
	int syn_current_slot;
	int syn_size;
	int syn_near_size;
	// 同步事件缓冲区
	std::vector<std::vector<std::vector<Event*> > > buffer_events;
	/*
	* 计算将事件插入到哪个近期时间桶槽位中
	*/
	int SlotForTime(int time) const;
	int ClampPriority(Event* event) const;
	/*
	* 判断是否为近期事件
	*/
	bool IsNearEvent(int currentTime, int eventTime) const;
	/*
	* 将近期事件插入到近期时间桶中
	*/
	void PushNearEvent(std::vector<PriorityBucket>& buckets, std::vector<std::uint32_t>& priorityMasks, Event* event);
	void InsertNearEventForQueue(Event* event, int index);
	void InsertNearSynEvent(Event* event);
	Event* RemoveReadyQueueEvent(int index, int slot, long long start_ns);
	Event* RemoveReadySynEvent(int slot, long long start_ns);
	/*
	* 提取一个当前时间槽位中优先级最高的事件
	*/
	Event* PopReadyEvent(PriorityBucket& bucket, int currentTime);
	/*
	* 查找第一个事件的时间
	*/
	int FindFirstTime(const std::vector<PriorityBucket>& buckets) const;
	/*
	* 删除一个时间桶中的所有事件
	*/
	void DeleteBucketEvents(std::vector<PriorityBucket>& buckets);
	/*
	* 删除长期事件队列中的事件
	*/
	void DeleteFarEvents(FarEventQueue& farEvents);
	/*
	* 将长期事件插入到短期并行事件桶中
	*/
	void MigrateNearEvents(int index);
	/*
	* 将长期同步事件插入到短期同步事件桶中
	*/
	void MigrateNearSynEvents();
	/*
	* 第index队列推进到下一个时间槽
	*/
	void AdvanceToNextQueueSlot(int index);
	void AdvanceToNextSynSlot();
	int FindNextNearEventTime(int index) const;
	int FindNextNearSynEventTime() const;
};

#endif
