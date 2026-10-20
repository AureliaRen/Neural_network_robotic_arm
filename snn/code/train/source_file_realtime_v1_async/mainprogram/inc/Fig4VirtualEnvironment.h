#ifndef FIG4_VIRTUAL_ENVIRONMENT_H
#define FIG4_VIRTUAL_ENVIRONMENT_H
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace fig4 {
enum Cue { LEFT=0, RIGHT=1, CENTER=2 };
enum Action { TURN_LEFT=0, TURN_RIGHT=1, NO_DECISION=2 };

struct Movement {
    double before=0, requested=0, after=0;
    bool entered=false, toward=false, boundary=false;
};

// Coordinates are normalized angular units, not paper-specified degrees/pixels.
// Observation occurs at the end of an action. No automatic target snapping.
class VirtualEnvironment {
public:
    double position, target, step, tolerance, limit;
    VirtualEnvironment(double initial, double goal, double increment, double band, double bound)
        : position(initial),target(goal),step(increment),tolerance(band),limit(bound) {
        if (!std::isfinite(initial) || !std::isfinite(goal) || !std::isfinite(increment) ||
            !std::isfinite(band) || !std::isfinite(bound) || increment<=0 || band<0 ||
            bound<=0 || std::fabs(initial)>bound || std::fabs(goal)>bound)
            throw std::runtime_error("Invalid virtual environment configuration");
    }
    int cue() const {
        if (std::fabs(target-position)<=tolerance) return CENTER;
        return target<position ? LEFT : RIGHT;
    }
    Movement move(int action) {
        if (action<0 || action>2) throw std::runtime_error("Invalid action");
        Movement m; m.before=position;
        m.requested=position+(action==TURN_LEFT?-step:action==TURN_RIGHT?step:0);
        m.after=std::clamp(m.requested,-limit,limit);
        m.boundary=(m.after!=m.requested);
        const bool wasInside=(cue()==CENTER);
        m.toward=std::fabs(target-m.after)<std::fabs(target-position);
        position=m.after;
        m.entered=!wasInside && cue()==CENTER;
        return m;
    }
};
}
#endif
