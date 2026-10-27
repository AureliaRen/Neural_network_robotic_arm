#pragma once

#include <atomic>
#include <chrono>
#include <iostream>

namespace bench_profile {

inline long long now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

inline std::atomic<long long> event_remove_ns{0};
inline std::atomic<long long> event_buffer_insert_ns{0};
inline std::atomic<long long> event_buffer_flush_ns{0};
inline std::atomic<long long> wheel_insert_ns{0};
inline std::atomic<long long> wheel_remove_ns{0};
inline std::atomic<long long> wheel_syn_insert_ns{0};
inline std::atomic<long long> wheel_syn_remove_ns{0};
inline std::atomic<long long> wheel_first_event_ns{0};
inline std::atomic<long long> wheel_first_syn_event_ns{0};
inline std::atomic<long long> wheel_advance_queue_ns{0};
inline std::atomic<long long> wheel_advance_syn_ns{0};
inline std::atomic<long long> wheel_migrate_queue_ns{0};
inline std::atomic<long long> wheel_migrate_syn_ns{0};
inline std::atomic<long long> wheel_pop_ready_ns{0};
inline std::atomic<long long> sync_ns{0};
inline std::atomic<long long> time_event_total_ns{0};
inline std::atomic<long long> time_event_update_state_ns{0};
inline std::atomic<long long> time_event_internal_spike_ns{0};
inline std::atomic<long long> time_event_reschedule_ns{0};
inline std::atomic<long long> gpu_update_total_ns{0};
inline std::atomic<long long> gpu_update_memcpy_h2d_ns{0};
inline std::atomic<long long> gpu_update_kernel_ns{0};
inline std::atomic<long long> gpu_update_d2h_ns{0};
inline std::atomic<long long> gpu_update_sync_ns{0};
inline std::atomic<long long> gpu_update_memset_ns{0};
inline std::atomic<long long> gpu_update_event_record_ns{0};
inline std::atomic<long long> remove_count{0};
inline std::atomic<long long> time_event_count{0};
inline std::atomic<long long> gpu_update_count{0};
inline std::atomic<long long> sync_count{0};

struct Snapshot {
    long long event_remove_ns{};
    long long event_buffer_insert_ns{};
    long long event_buffer_flush_ns{};
    long long wheel_insert_ns{};
    long long wheel_remove_ns{};
    long long wheel_syn_insert_ns{};
    long long wheel_syn_remove_ns{};
    long long wheel_first_event_ns{};
    long long wheel_first_syn_event_ns{};
    long long wheel_advance_queue_ns{};
    long long wheel_advance_syn_ns{};
    long long wheel_migrate_queue_ns{};
    long long wheel_migrate_syn_ns{};
    long long wheel_pop_ready_ns{};
    long long sync_ns{};
    long long time_event_total_ns{};
    long long time_event_update_state_ns{};
    long long time_event_internal_spike_ns{};
    long long time_event_reschedule_ns{};
    long long gpu_update_total_ns{};
    long long gpu_update_memcpy_h2d_ns{};
    long long gpu_update_kernel_ns{};
    long long gpu_update_d2h_ns{};
    long long gpu_update_sync_ns{};
    long long gpu_update_memset_ns{};
    long long gpu_update_event_record_ns{};
    long long remove_count{};
    long long time_event_count{};
    long long gpu_update_count{};
    long long sync_count{};
};

inline void reset() {
    event_remove_ns = 0;
    event_buffer_insert_ns = 0;
    event_buffer_flush_ns = 0;
    wheel_insert_ns = 0;
    wheel_remove_ns = 0;
    wheel_syn_insert_ns = 0;
    wheel_syn_remove_ns = 0;
    wheel_first_event_ns = 0;
    wheel_first_syn_event_ns = 0;
    wheel_advance_queue_ns = 0;
    wheel_advance_syn_ns = 0;
    wheel_migrate_queue_ns = 0;
    wheel_migrate_syn_ns = 0;
    wheel_pop_ready_ns = 0;
    sync_ns = 0;
    time_event_total_ns = 0;
    time_event_update_state_ns = 0;
    time_event_internal_spike_ns = 0;
    time_event_reschedule_ns = 0;
    gpu_update_total_ns = 0;
    gpu_update_memcpy_h2d_ns = 0;
    gpu_update_kernel_ns = 0;
    gpu_update_d2h_ns = 0;
    gpu_update_sync_ns = 0;
    gpu_update_memset_ns = 0;
    gpu_update_event_record_ns = 0;
    remove_count = 0;
    time_event_count = 0;
    gpu_update_count = 0;
    sync_count = 0;
}

inline Snapshot snapshot() {
    Snapshot s;
    s.event_remove_ns = event_remove_ns.load(std::memory_order_relaxed);
    s.event_buffer_insert_ns = event_buffer_insert_ns.load(std::memory_order_relaxed);
    s.event_buffer_flush_ns = event_buffer_flush_ns.load(std::memory_order_relaxed);
    s.wheel_insert_ns = wheel_insert_ns.load(std::memory_order_relaxed);
    s.wheel_remove_ns = wheel_remove_ns.load(std::memory_order_relaxed);
    s.wheel_syn_insert_ns = wheel_syn_insert_ns.load(std::memory_order_relaxed);
    s.wheel_syn_remove_ns = wheel_syn_remove_ns.load(std::memory_order_relaxed);
    s.wheel_first_event_ns = wheel_first_event_ns.load(std::memory_order_relaxed);
    s.wheel_first_syn_event_ns = wheel_first_syn_event_ns.load(std::memory_order_relaxed);
    s.wheel_advance_queue_ns = wheel_advance_queue_ns.load(std::memory_order_relaxed);
    s.wheel_advance_syn_ns = wheel_advance_syn_ns.load(std::memory_order_relaxed);
    s.wheel_migrate_queue_ns = wheel_migrate_queue_ns.load(std::memory_order_relaxed);
    s.wheel_migrate_syn_ns = wheel_migrate_syn_ns.load(std::memory_order_relaxed);
    s.wheel_pop_ready_ns = wheel_pop_ready_ns.load(std::memory_order_relaxed);
    s.sync_ns = sync_ns.load(std::memory_order_relaxed);
    s.time_event_total_ns = time_event_total_ns.load(std::memory_order_relaxed);
    s.time_event_update_state_ns = time_event_update_state_ns.load(std::memory_order_relaxed);
    s.time_event_internal_spike_ns = time_event_internal_spike_ns.load(std::memory_order_relaxed);
    s.time_event_reschedule_ns = time_event_reschedule_ns.load(std::memory_order_relaxed);
    s.gpu_update_total_ns = gpu_update_total_ns.load(std::memory_order_relaxed);
    s.gpu_update_memcpy_h2d_ns = gpu_update_memcpy_h2d_ns.load(std::memory_order_relaxed);
    s.gpu_update_kernel_ns = gpu_update_kernel_ns.load(std::memory_order_relaxed);
    s.gpu_update_d2h_ns = gpu_update_d2h_ns.load(std::memory_order_relaxed);
    s.gpu_update_sync_ns = gpu_update_sync_ns.load(std::memory_order_relaxed);
    s.gpu_update_memset_ns = gpu_update_memset_ns.load(std::memory_order_relaxed);
    s.gpu_update_event_record_ns = gpu_update_event_record_ns.load(std::memory_order_relaxed);
    s.remove_count = remove_count.load(std::memory_order_relaxed);
    s.time_event_count = time_event_count.load(std::memory_order_relaxed);
    s.gpu_update_count = gpu_update_count.load(std::memory_order_relaxed);
    s.sync_count = sync_count.load(std::memory_order_relaxed);
    return s;
}

inline double ns_to_ms(long long ns) {
    return static_cast<double>(ns) / 1000000.0;
}

inline void print_line(const char* name, long long ns, double run_ms) {
    const double ms = ns_to_ms(ns);
    const double pct = run_ms > 0.0 ? (ms * 100.0 / run_ms) : 0.0;
    std::cout << name << ": " << ms << " ms (" << pct << "%)" << std::endl;
}

inline void print_breakdown(const Snapshot& s, double run_ms) {
    std::cout << "profiling_breakdown_begin" << std::endl;
    print_line("event_remove", s.event_remove_ns, run_ms);
    print_line("event_buffer_insert", s.event_buffer_insert_ns, run_ms);
    print_line("event_buffer_flush", s.event_buffer_flush_ns, run_ms);
    print_line("wheel_insert", s.wheel_insert_ns, run_ms);
    print_line("wheel_remove", s.wheel_remove_ns, run_ms);
    print_line("wheel_syn_insert", s.wheel_syn_insert_ns, run_ms);
    print_line("wheel_syn_remove", s.wheel_syn_remove_ns, run_ms);
    print_line("wheel_first_event", s.wheel_first_event_ns, run_ms);
    print_line("wheel_first_syn_event", s.wheel_first_syn_event_ns, run_ms);
    print_line("wheel_advance_queue", s.wheel_advance_queue_ns, run_ms);
    print_line("wheel_advance_syn", s.wheel_advance_syn_ns, run_ms);
    print_line("wheel_migrate_queue", s.wheel_migrate_queue_ns, run_ms);
    print_line("wheel_migrate_syn", s.wheel_migrate_syn_ns, run_ms);
    print_line("wheel_pop_ready", s.wheel_pop_ready_ns, run_ms);
    print_line("synchronize_thread", s.sync_ns, run_ms);
    print_line("time_event_total", s.time_event_total_ns, run_ms);
    print_line("time_event_update_state", s.time_event_update_state_ns, run_ms);
    print_line("time_event_internal_spike", s.time_event_internal_spike_ns, run_ms);
    print_line("time_event_reschedule", s.time_event_reschedule_ns, run_ms);
    print_line("gpu_update_total", s.gpu_update_total_ns, run_ms);
    print_line("gpu_update_memcpy_h2d", s.gpu_update_memcpy_h2d_ns, run_ms);
    print_line("gpu_update_kernel", s.gpu_update_kernel_ns, run_ms);
    print_line("gpu_update_d2h", s.gpu_update_d2h_ns, run_ms);
    print_line("gpu_update_sync", s.gpu_update_sync_ns, run_ms);
    print_line("gpu_update_memset", s.gpu_update_memset_ns, run_ms);
    print_line("gpu_update_event_record", s.gpu_update_event_record_ns, run_ms);
    std::cout << "counts: remove=" << s.remove_count
              << ", time_event=" << s.time_event_count
              << ", gpu_update=" << s.gpu_update_count
              << ", sync=" << s.sync_count << std::endl;
    std::cout << "profiling_breakdown_end" << std::endl;
}

}  // namespace bench_profile
