#include "../source_file_realtime_v1_async/EventQueue/inc/TimingWheelEventQueue.h"
#include "../source_file_realtime_v1_async/mainprogram/inc/BenchProfiling.h"

namespace {
const int kInitialBucketReserve = 4;
const int kInitialBufferReserve = 64;

bool IsPowerOfTwoValue(int value) {
	return value > 0 && (value & (value - 1)) == 0;
}
}

TimingWheelEventQueue::TimingWheelEventQueue(int numberofQueue, int wheelSize)
	: EventQueue(numberofQueue, false),
	wheel_size(wheelSize > 0 ? wheelSize : 1024),
	wheel_mask(0),
	wheel_size_is_power_of_two(false),
	queue_buckets(numberofQueue),
	queue_far_events(numberofQueue),
	queue_bucket_counts(numberofQueue),
	queue_bucket_priority_masks(numberofQueue),
	queue_bucket_times(numberofQueue),
	queue_current_time(numberofQueue, 0),
	queue_current_slot(numberofQueue, 0),
	queue_size(numberofQueue, 0),
	queue_near_size(numberofQueue, 0),
	syn_buckets(wheelSize > 0 ? wheelSize : 1024),
	syn_far_events(),
	syn_bucket_counts(wheelSize > 0 ? wheelSize : 1024, 0),
	syn_bucket_priority_masks(wheelSize > 0 ? wheelSize : 1024, 0),
	syn_bucket_times(wheelSize > 0 ? wheelSize : 1024, -1),
	syn_current_time(0),
	syn_current_slot(0),
	syn_size(0),
	syn_near_size(0),
	buffer_events(numberofQueue)
{
	this->wheel_size_is_power_of_two = IsPowerOfTwoValue(this->wheel_size);
	this->wheel_mask = this->wheel_size_is_power_of_two ? (this->wheel_size - 1) : 0;

	for (int i = 0; i < numberofQueue; i++) {
		queue_buckets[i].resize(this->wheel_size);
		queue_bucket_counts[i].resize(this->wheel_size, 0);
		queue_bucket_priority_masks[i].resize(this->wheel_size, 0);
		queue_bucket_times[i].resize(this->wheel_size, -1);
		buffer_events[i].resize(numberofQueue);
		for (int slot = 0; slot < this->wheel_size; slot++) {
			for (int priority = 0; priority < PriorityCount; priority++) {
				queue_buckets[i][slot].events[priority].reserve(kInitialBucketReserve);
			}
		}
		for (int j = 0; j < numberofQueue; j++) {
			buffer_events[i][j].reserve(kInitialBufferReserve);
		}
	}

	for (int slot = 0; slot < this->wheel_size; slot++) {
		for (int priority = 0; priority < PriorityCount; priority++) {
			syn_buckets[slot].events[priority].reserve(kInitialBucketReserve);
		}
	}
}

TimingWheelEventQueue::~TimingWheelEventQueue() {
	for (int i = 0; i < this->NumberOfQueue; i++) {
		DeleteBucketEvents(queue_buckets[i]);
		DeleteFarEvents(queue_far_events[i]);
		for (int j = 0; j < this->NumberOfQueue; j++) {
			for (int k = 0; k < static_cast<int>(buffer_events[i][j].size()); k++) {
				delete buffer_events[i][j][k];
			}
			buffer_events[i][j].clear();
		}
	}
	DeleteBucketEvents(syn_buckets);
	DeleteFarEvents(syn_far_events);
}

bool TimingWheelEventQueue::EventComesAfter::operator()(Event* left, Event* right) const {
	if (left->getTime() != right->getTime()) {
		return left->getTime() > right->getTime();
	}
	return left->getPriority() < right->getPriority();
}

int TimingWheelEventQueue::SlotForTime(int time) const {
	if (this->wheel_size_is_power_of_two) {
		// 进行求与运算得到槽位索引
		return time & this->wheel_mask;
	}
	int slot = time % this->wheel_size;
	return slot < 0 ? slot + this->wheel_size : slot;
}

int TimingWheelEventQueue::ClampPriority(Event* event) const {
	int priority = static_cast<int>(event->getPriority());
	if (priority < 0) {
		return 0;
	}
	if (priority >= PriorityCount) {
		return PriorityCount - 1;
	}
	return priority;
}

bool TimingWheelEventQueue::IsNearEvent(int currentTime, int eventTime) const {
	return eventTime < currentTime + this->wheel_size;
}

void TimingWheelEventQueue::PushNearEvent(std::vector<PriorityBucket>& buckets, std::vector<std::uint32_t>& priorityMasks, Event* event) {
	const int priority = ClampPriority(event);
	const int slot = SlotForTime(event->getTime());
	buckets[slot].events[priority].push_back(event);
	priorityMasks[slot] |= (static_cast<std::uint32_t>(1) << priority);
}

void TimingWheelEventQueue::InsertNearEventForQueue(Event* event, int index) {
	PushNearEvent(queue_buckets[index], queue_bucket_priority_masks[index], event);
	const int slot = SlotForTime(event->getTime());
	const int event_time = event->getTime();
	if (queue_bucket_counts[index][slot] == 0) {
		queue_bucket_times[index][slot] = event_time;
	}
	queue_bucket_counts[index][slot]++;
	queue_near_size[index]++;
}

void TimingWheelEventQueue::InsertNearSynEvent(Event* event) {
	PushNearEvent(syn_buckets, syn_bucket_priority_masks, event);
	const int slot = SlotForTime(event->getTime());
	const int event_time = event->getTime();
	if (syn_bucket_counts[slot] == 0) {
		syn_bucket_times[slot] = event_time;
	}
	syn_bucket_counts[slot]++;
	syn_near_size++;
}

Event* TimingWheelEventQueue::PopReadyEvent(PriorityBucket& bucket, int currentTime) {
	(void)currentTime;
	for (int priority = PriorityCount - 1; priority >= 0; priority--) {
		std::vector<Event*>& events = bucket.events[priority];
		if (!events.empty()) {
			Event* event = events.back();
			events.pop_back();
			return event;
		}
	}
	return NULL;
}

int TimingWheelEventQueue::FindFirstTime(const std::vector<PriorityBucket>& buckets) const {
	bool found = false;
	int firstTime = 0;
	for (int slot = 0; slot < static_cast<int>(buckets.size()); slot++) {
		for (int priority = 0; priority < PriorityCount; priority++) {
			const std::vector<Event*>& events = buckets[slot].events[priority];
			for (int i = 0; i < static_cast<int>(events.size()); i++) {
				int eventTime = events[i]->getTime();
				if (!found || eventTime < firstTime) {
					found = true;
					firstTime = eventTime;
				}
			}
		}
	}
	return found ? firstTime : -1;
}

void TimingWheelEventQueue::DeleteBucketEvents(std::vector<PriorityBucket>& buckets) {
	for (int slot = 0; slot < static_cast<int>(buckets.size()); slot++) {
		for (int priority = 0; priority < PriorityCount; priority++) {
			std::vector<Event*>& events = buckets[slot].events[priority];
			for (int i = 0; i < static_cast<int>(events.size()); i++) {
				delete events[i];
			}
			events.clear();
		}
	}
}

void TimingWheelEventQueue::DeleteFarEvents(FarEventQueue& farEvents) {
	while (!farEvents.empty()) {
		delete farEvents.top();
		farEvents.pop();
	}
}

void TimingWheelEventQueue::MigrateNearEvents(int index) {
	const long long start_ns = bench_profile::now_ns();
	PriorityBucket* active_bucket = NULL;
	std::vector<int>* active_counts = NULL;
	std::vector<std::uint32_t>* active_masks = NULL;
	std::vector<Event*>* active_events = NULL;
	int active_slot = -1;
	int active_priority = -1;
	int migrated_count = 0;
	while (!queue_far_events[index].empty()) {
		Event* event = queue_far_events[index].top();
		if (!IsNearEvent(queue_current_time[index], event->getTime())) {
			break;
		}
		queue_far_events[index].pop();
		const int event_time = event->getTime();
		const int slot = SlotForTime(event_time);
		const int priority = ClampPriority(event);
		if (slot != active_slot) {
			active_slot = slot;
			active_priority = -1;
			active_bucket = &queue_buckets[index][slot];
			active_counts = &queue_bucket_counts[index];
			active_masks = &queue_bucket_priority_masks[index];
			if ((*active_counts)[slot] == 0) {
				queue_bucket_times[index][slot] = event_time;
			}
		}
		if (priority != active_priority) {
			active_priority = priority;
			active_events = &active_bucket->events[priority];
		}
		active_events->push_back(event);
		(*active_masks)[slot] |= (static_cast<std::uint32_t>(1) << priority);
		(*active_counts)[slot]++;
		migrated_count++;
	}
	queue_near_size[index] += migrated_count;
	bench_profile::wheel_migrate_queue_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

void TimingWheelEventQueue::MigrateNearSynEvents() {
	const long long start_ns = bench_profile::now_ns();
	PriorityBucket* active_bucket = NULL;
	std::vector<Event*>* active_events = NULL;
	int active_slot = -1;
	int active_priority = -1;
	int migrated_count = 0;
	while (!syn_far_events.empty()) {
		Event* event = syn_far_events.top();
		if (!IsNearEvent(syn_current_time, event->getTime())) {
			break;
		}
		syn_far_events.pop();
		const int event_time = event->getTime();
		const int slot = SlotForTime(event_time);
		const int priority = ClampPriority(event);
		if (slot != active_slot) {
			active_slot = slot;
			active_priority = -1;
			active_bucket = &syn_buckets[slot];
			if (syn_bucket_counts[slot] == 0) {
				syn_bucket_times[slot] = event_time;
			}
		}
		if (priority != active_priority) {
			active_priority = priority;
			active_events = &active_bucket->events[priority];
		}
		active_events->push_back(event);
		syn_bucket_priority_masks[slot] |= (static_cast<std::uint32_t>(1) << priority);
		syn_bucket_counts[slot]++;
		migrated_count++;
	}
	syn_near_size += migrated_count;
	bench_profile::wheel_migrate_syn_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

void TimingWheelEventQueue::AdvanceToNextQueueSlot(int index) {
	const long long start_ns = bench_profile::now_ns();
	if (queue_near_size[index] <= 0) {
		bench_profile::wheel_advance_queue_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
		return;
	}
	const int next_time = FindNextNearEventTime(index);
	if (next_time >= 0 && next_time > queue_current_time[index]) {
		queue_current_time[index] = next_time;
		queue_current_slot[index] = SlotForTime(next_time);
	}
	bench_profile::wheel_advance_queue_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

void TimingWheelEventQueue::AdvanceToNextSynSlot() {
	const long long start_ns = bench_profile::now_ns();
	if (syn_near_size <= 0) {
		bench_profile::wheel_advance_syn_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
		return;
	}
	const int next_time = FindNextNearSynEventTime();
	if (next_time >= 0 && next_time > syn_current_time) {
		syn_current_time = next_time;
		syn_current_slot = SlotForTime(next_time);
	}
	bench_profile::wheel_advance_syn_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

void TimingWheelEventQueue::Insert_a_Event(Event* event, int index) {
	const long long start_ns = bench_profile::now_ns();
	if (IsNearEvent(queue_current_time[index], event->getTime())) {
		InsertNearEventForQueue(event, index);
	}
	else {
		queue_far_events[index].push(event);
	}
	queue_size[index]++;
	bench_profile::wheel_insert_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

Event* TimingWheelEventQueue::RemoveReadyQueueEvent(int index, int slot, long long start_ns) {
	const long long pop_start_ns = bench_profile::now_ns();
	Event* event = PopReadyEvent(queue_buckets[index][slot], queue_current_time[index]);
	bench_profile::wheel_pop_ready_ns.fetch_add(bench_profile::now_ns() - pop_start_ns, std::memory_order_relaxed);
	if (event == NULL) {
		return NULL;
	}

	const int priority = ClampPriority(event);
	queue_size[index]--;
	queue_near_size[index]--;
	const int new_count = --queue_bucket_counts[index][slot];
	if (queue_buckets[index][slot].events[priority].empty()) {
		queue_bucket_priority_masks[index][slot] &= ~(static_cast<std::uint32_t>(1) << priority);
	}
	if (new_count == 0) {
		queue_bucket_times[index][slot] = -1;
		queue_bucket_priority_masks[index][slot] = 0;
	}

	const long long elapsed_ns = bench_profile::now_ns() - start_ns;
	bench_profile::wheel_remove_ns.fetch_add(elapsed_ns, std::memory_order_relaxed);
	bench_profile::event_remove_ns.fetch_add(elapsed_ns, std::memory_order_relaxed);
	bench_profile::remove_count.fetch_add(1, std::memory_order_relaxed);
	return event;
}

Event* TimingWheelEventQueue::Remove_a_Event(int index) {
	const long long start_ns = bench_profile::now_ns();
	while (queue_size[index] > 0) {
		int slot = queue_current_slot[index];
		if (queue_near_size[index] > 0 &&
			queue_bucket_counts[index][slot] > 0 &&
			queue_bucket_times[index][slot] == queue_current_time[index] &&
			queue_bucket_priority_masks[index][slot] != 0) {
			Event* event = RemoveReadyQueueEvent(index, slot, start_ns);
			if (event != NULL) {
				return event;
			}
		}

		auto& far_queue = queue_far_events[index];
		if (queue_near_size[index] == 0) {
			if (far_queue.empty()) {
				break;
			}
			const int far_event_time = far_queue.top()->getTime();
			queue_current_time[index] = far_event_time;
			queue_current_slot[index] = SlotForTime(far_event_time);
			MigrateNearEvents(index);
		}
		else if (!far_queue.empty()) {
			const int far_event_time = far_queue.top()->getTime();
			if (IsNearEvent(queue_current_time[index], far_event_time)) {
				MigrateNearEvents(index);
			}
		}
		AdvanceToNextQueueSlot(index);
		if (queue_near_size[index] == 0) {
			continue;
		}
		slot = queue_current_slot[index];
		if (queue_bucket_priority_masks[index][slot] == 0) {
			queue_current_time[index]++;
			queue_current_slot[index] = SlotForTime(queue_current_time[index]);
			continue;
		}
		Event* event = RemoveReadyQueueEvent(index, slot, start_ns);
		if (event != NULL) {
			return event;
		}
		queue_current_time[index]++;
		queue_current_slot[index] = SlotForTime(queue_current_time[index]);
	}
	const long long elapsed_ns = bench_profile::now_ns() - start_ns;
	bench_profile::wheel_remove_ns.fetch_add(elapsed_ns, std::memory_order_relaxed);
	bench_profile::event_remove_ns.fetch_add(elapsed_ns, std::memory_order_relaxed);
	bench_profile::remove_count.fetch_add(1, std::memory_order_relaxed);
	return NULL;
}

int TimingWheelEventQueue::Get_Event_Queue_vector_size(int index) {
	return queue_size[index];
}

int TimingWheelEventQueue::Get_First_Event_Time(int index) {
	const long long start_ns = bench_profile::now_ns();
	int firstTime = FindNextNearEventTime(index);
	if (!queue_far_events[index].empty() && (firstTime < 0 || queue_far_events[index].top()->getTime() < firstTime)) {
		firstTime = queue_far_events[index].top()->getTime();
	}
	bench_profile::wheel_first_event_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
	return firstTime;
}

int TimingWheelEventQueue::Get_syn_Event_Queue_vector_size() {
	return syn_size;
}

void TimingWheelEventQueue::Insert_a_syn_Event(Event* event) {
	const long long start_ns = bench_profile::now_ns();
	if (IsNearEvent(syn_current_time, event->getTime())) {
		InsertNearSynEvent(event);
	}
	else {
		syn_far_events.push(event);
	}
	syn_size++;
	bench_profile::wheel_syn_insert_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

Event* TimingWheelEventQueue::RemoveReadySynEvent(int slot, long long start_ns) {
	const long long pop_start_ns = bench_profile::now_ns();
	Event* event = PopReadyEvent(syn_buckets[slot], syn_current_time);
	bench_profile::wheel_pop_ready_ns.fetch_add(bench_profile::now_ns() - pop_start_ns, std::memory_order_relaxed);
	if (event == NULL) {
		return NULL;
	}

	const int priority = ClampPriority(event);
	syn_size--;
	syn_near_size--;
	const int new_count = --syn_bucket_counts[slot];
	if (syn_buckets[slot].events[priority].empty()) {
		syn_bucket_priority_masks[slot] &= ~(static_cast<std::uint32_t>(1) << priority);
	}
	if (new_count == 0) {
		syn_bucket_times[slot] = -1;
		syn_bucket_priority_masks[slot] = 0;
	}

	bench_profile::wheel_syn_remove_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
	return event;
}

Event* TimingWheelEventQueue::Remove_a_syn_Event() {
	const long long start_ns = bench_profile::now_ns();
	while (syn_size > 0) {
		int slot = syn_current_slot;
		if (syn_near_size > 0 &&
			syn_bucket_counts[slot] > 0 &&
			syn_bucket_times[slot] == syn_current_time &&
			syn_bucket_priority_masks[slot] != 0) {
			Event* event = RemoveReadySynEvent(slot, start_ns);
			if (event != NULL) {
				return event;
			}
		}

		if (syn_near_size == 0) {
			if (syn_far_events.empty()) {
				break;
			}
			syn_current_time = syn_far_events.top()->getTime();
			syn_current_slot = SlotForTime(syn_current_time);
			MigrateNearSynEvents();
		}
		else if (!syn_far_events.empty() &&
			IsNearEvent(syn_current_time, syn_far_events.top()->getTime())) {
			MigrateNearSynEvents();
		}
		AdvanceToNextSynSlot();
		if (syn_near_size == 0) {
			continue;
		}
		slot = syn_current_slot;
		if (syn_bucket_priority_masks[slot] == 0) {
			syn_current_time++;
			syn_current_slot = SlotForTime(syn_current_time);
			continue;
		}
		Event* event = RemoveReadySynEvent(slot, start_ns);
		if (event != NULL) {
			return event;
		}
		syn_current_time++;
		syn_current_slot = SlotForTime(syn_current_time);
	}
	bench_profile::wheel_syn_remove_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
	return NULL;
}

int TimingWheelEventQueue::Get_First_syn_Event_Time() {
	const long long start_ns = bench_profile::now_ns();
	int firstTime = FindNextNearSynEventTime();
	if (!syn_far_events.empty() && (firstTime < 0 || syn_far_events.top()->getTime() < firstTime)) {
		firstTime = syn_far_events.top()->getTime();
	}
	bench_profile::wheel_first_syn_event_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
	return firstTime;
}

int TimingWheelEventQueue::FindNextNearEventTime(int index) const {
	if (queue_near_size[index] <= 0) {
		return -1;
	}
	for (int offset = 0; offset < this->wheel_size; offset++) {
		const int candidate_time = queue_current_time[index] + offset;
		const int slot = SlotForTime(candidate_time);
		if (queue_bucket_counts[index][slot] > 0 && queue_bucket_times[index][slot] == candidate_time) {
			return candidate_time;
		}
	}
	return -1;
}

int TimingWheelEventQueue::FindNextNearSynEventTime() const {
	if (syn_near_size <= 0) {
		return -1;
	}
	for (int offset = 0; offset < this->wheel_size; offset++) {
		const int candidate_time = syn_current_time + offset;
		const int slot = SlotForTime(candidate_time);
		if (syn_bucket_counts[slot] > 0 && syn_bucket_times[slot] == candidate_time) {
			return candidate_time;
		}
	}
	return -1;
}

void TimingWheelEventQueue::Insert_a_Event_to_Buffer(Event* event, int index1, int index2) {
	const long long start_ns = bench_profile::now_ns();
	buffer_events[index1][index2].push_back(event);
	bench_profile::event_buffer_insert_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

void TimingWheelEventQueue::Reset_Buffer(int index) {
	for (int i = 0; i < this->NumberOfQueue; i++) {
		for (int j = 0; j < static_cast<int>(buffer_events[i][index].size()); j++) {
			delete buffer_events[i][index][j];
		}
		buffer_events[i][index].clear();
	}
}

void TimingWheelEventQueue::Insert_Buffer_to_Event_Queue(int index) {
	const long long start_ns = bench_profile::now_ns();
	for (int i = 0; i < this->NumberOfQueue; i++) {
		std::vector<Event*>& pending_events = buffer_events[i][index];
		for (int j = 0; j < static_cast<int>(pending_events.size()); j++) {
			Insert_a_Event(pending_events[j], index);
		}
		pending_events.clear();
	}
	bench_profile::event_buffer_flush_ns.fetch_add(bench_profile::now_ns() - start_ns, std::memory_order_relaxed);
}

int TimingWheelEventQueue::Get_Buffer_size(int index1, int index2) {
	return static_cast<int>(buffer_events[index1][index2].size());
}

int TimingWheelEventQueue::Get_Buffer_capacity(int index1, int index2) {
	return static_cast<int>(buffer_events[index1][index2].capacity());
}

void TimingWheelEventQueue::Increase_Buffer_size(int index1, int index2) {
	(void)index1;
	(void)index2;
}

void TimingWheelEventQueue::ResetSizeBuffer(int index1, int index2) {
	buffer_events[index1][index2].clear();
}

bool TimingWheelEventQueue::is_Buffer_Empty() {
	for (int i = 0; i < this->NumberOfQueue; i++) {
		for (int j = 0; j < this->NumberOfQueue; j++) {
			if (!buffer_events[i][j].empty()) {
				return false;
			}
		}
	}
	return true;
}
