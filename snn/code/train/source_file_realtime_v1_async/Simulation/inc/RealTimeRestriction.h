#ifndef REAL_TIME_RESTRICTION_H
#define REAL_TIME_RESTRICTION_H

#include <atomic>
#include <chrono>

enum RealTimeRestrictionLevel {
    SIMULATION_TOO_FAST = 0,
    ALL_EVENTS_ENABLED = 1,
    LEARNING_RULES_DISABLED = 2,
    SPIKES_DISABLED = 3,
    ALL_UNESSENTIAL_EVENTS_DISABLED = 4
};

class RealTimeRestriction {
public:
    RealTimeRestriction();

    void SetParameterWatchDog(double new_simulation_step_size, double new_max_simulation_time_in_advance,
        float new_first_section, float new_second_section, float new_third_section);
    void NextStepWatchDog();
    void StartWatchDog();
    void StopWatchDog();
    void Watchdog();
    void SetSleepPeriod(double new_sleep_period);

    RealTimeRestrictionLevel GetRestrictionLevel() const;

private:
    void ResetClock();
    void UpdateRestrictionLevel(double current_time);

    double simulation_step_size;
    double max_simulation_time_in_advance;
    double simulation_time;
    double first_section;
    double second_section;
    double third_section;
    double first_gap_time;
    double second_gap_time;
    double third_gap_time;
    double sleep_period;

    std::atomic<bool> stop_requested;
    std::atomic<bool> started;
    std::atomic<bool> primed;
    std::atomic<RealTimeRestrictionLevel> restriction_level;
    std::chrono::steady_clock::time_point start_time;
};

#endif