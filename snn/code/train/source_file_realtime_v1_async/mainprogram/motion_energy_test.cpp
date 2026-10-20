#include <motion_energy.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int main() {
#if SNN_CPU_ONLY
	std::printf("motion_energy_test requires GPU backend.\n");
	return 0;
#else
	const int num_px = 16;
	const int num_channels = 1;
	const int num_dirs = 8;
	const double speed = 1.5;
	const int iterations = 10;

	MotionEnergy me(num_px, num_px, num_channels);

	std::vector<float> v1_comp(num_px * num_px * num_dirs, 0.0f);
	std::vector<unsigned char> stim(num_px * num_px, 0);

	std::srand(1);
	for (int iter = 0; iter < iterations; ++iter) {
		std::memset(stim.data(), 0, stim.size() * sizeof(unsigned char));
		for (int x = 0; x < num_px; ++x) {
			for (int y = 0; y < num_px; ++y) {
				stim[x * num_px + y] = static_cast<unsigned char>(std::rand() % 256);
			}
		}

		me.calcV1complex(stim.data(), v1_comp.data(), speed, false);

		float sum_activity = 0.0f;
		for (float value : v1_comp) {
			sum_activity += value;
		}
		std::printf("%d: mean activity = %f\n", iter, sum_activity / static_cast<float>(v1_comp.size()));
	}

	return 0;
#endif
}
