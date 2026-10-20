// paperpsp_selftest.cpp
// Validates the PaperPSP synapse kernel against Wei, Bu & Zhu (2020) Eq.(4):
//   PSP(x) = w * (x/tau) * exp(1 - x/tau)   for x >= 0,  else 0
// Checks:
//   1) output strictly 0 for x < 0 (before arrival)
//   2) signal starts at arrival t_pre + D (x = 0)
//   3) peak located at t_pre + D + tau
//   4) numeric peak equals w (=1 here)
// A synthetic impulse is delivered at step D (arrival), not at step 0, so the
// delay behaviour is exercised too.

#include "../source_file_realtime_v1_async/SynModel/inc/int/PaperPSP.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include <boost/any.hpp>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main() {
    const float tau = 5.0f;      // ms
    const float dt = 0.1f;       // ms
    const float w = 1.0f;        // weight
    const int D = 10;            // arrival step = t_pre + delay (10 steps = 1 ms)
    const int N = 300;           // 30 ms

    PaperPSP psp(1);
    std::map<std::string, boost::any> params;
    params["tau"] = tau;
    ModelDescription intm;
    intm.ModelName = "ForwardEulerMethod";
    intm.ModelParameter["step"] = dt;
    params["int_method"] = intm;
    psp.setParameters(params);
    psp.InitSynState(1);

    Interconnections inter(nullptr, nullptr, nullptr, 1, w, 4, 0);
    inter.TargetNeuronModelIndex = 0;
    std::vector<float> state(1, -65.0f);

    std::vector<float> cur(N, 0.0f);
    for (int k = 0; k < N; ++k) {
        if (k == D) {
            psp.ProcessInput(&inter, state.data(), w);   // spike arrives here
        }
        cur[k] = psp.GetCurrent(state.data(), 0);        // value at x=(k-D)*dt
        psp.UpdateSynState(dt);
    }

    // ---- checks ----
    float before_max = 0.0f;
    for (int k = 0; k < D; ++k) before_max = std::max(before_max, std::fabs(cur[k]));
    float at_arrival = cur[D];

    int peak_k = 0;
    for (int k = 0; k < N; ++k) if (cur[k] > cur[peak_k]) peak_k = k;
    float peak_val = cur[peak_k];
    float peak_t = (peak_k - D) * dt;      // time since arrival

    int first_nonzero = -1;
    for (int k = 0; k < N; ++k) if (std::fabs(cur[k]) > 1e-9f) { first_nonzero = k; break; }

    // two-neuron symmetry: identical impulses must yield identical current
    PaperPSP psp2(1);
    psp2.setParameters(params);
    psp2.InitSynState(2);
    Interconnections i0(nullptr, nullptr, nullptr, 1, w, 4, 0);
    i0.TargetNeuronModelIndex = 0;
    Interconnections i1(nullptr, nullptr, nullptr, 1, w, 4, 0);
    i1.TargetNeuronModelIndex = 1;
    float max_diff = 0.0f;
    for (int k = 0; k < N; ++k) {
        if (k == D) {
            psp2.ProcessInput(&i0, state.data(), w);
            psp2.ProcessInput(&i1, state.data(), w);
        }
        float c0 = psp2.GetCurrent(state.data(), 0);
        float c1 = psp2.GetCurrent(state.data(), 1);
        max_diff = std::max(max_diff, std::fabs(c0 - c1));
        psp2.UpdateSynState(dt);
    }

    bool ok = true;
    auto check = [&ok](const std::string& name, bool pass, const std::string& detail) {
        std::cout << "  [" << (pass ? "PASS" : "FAIL") << "] " << name << " : " << detail << "\n";
        ok = ok && pass;
    };

    std::cout << "PaperPSP self-test (tau=" << tau << " ms, w=" << w << ", arrival step D=" << D << ")\n";
    check("1) PSP == 0 for x<0 (before arrival)", before_max < 1e-9f,
          "max=" + std::to_string(before_max));
    check("2) PSP == 0 at arrival (x=0), first nonzero after", std::fabs(at_arrival) < 1e-9f && first_nonzero > D,
          "at_arrival=" + std::to_string(at_arrival) + ", first_nonzero_step=" + std::to_string(first_nonzero));
    check("3) peak at arrival + tau", std::fabs(peak_t - tau) < 0.2f,
          "peak_t=" + std::to_string(peak_t) + " ms (expect " + std::to_string(tau) + ")");
    check("4) peak value == w (=1)", std::fabs(peak_val - w) < 0.02f,
          "peak=" + std::to_string(peak_val));
    check("5) two-neuron symmetry", max_diff < 1e-9f,
          "max|c0-c1|=" + std::to_string(max_diff));

    std::cout << (ok ? "ALL CHECKS PASSED\n" : "SOME CHECKS FAILED\n");
    return ok ? 0 : 1;
}
