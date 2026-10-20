#include <motion_energy.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kDirs = MotionEnergy::NumDirs;
constexpr int kFilters = MotionEnergy::NumFilters;
constexpr int kScales = 3;
constexpr int kFramesPerDirection = 10;
constexpr int kTailFramesForCheck = 3;
constexpr double kSpeed = 1.5;
constexpr const char* kDefaultStimulusPath =
	"D:\\Programing\\My_Program\\Carsim\\CARLsim4\\CARLsim4\\doc\\source\\tutorial\\5_motion_energy\\input\\grating.dat";

struct StimulusHeader {
	int type = 0;
	int channels = 0;
	int width = 0;
	int height = 0;
	int length = 0;
};

bool ReadStimulusHeader(std::ifstream& input, StimulusHeader& header) {
	int32_t signature = 0;
	float version = 0.0f;
	int32_t type = 0;
	char channels = 0;
	int32_t width = 0;
	int32_t height = 0;
	int32_t length = 0;

	input.read(reinterpret_cast<char*>(&signature), sizeof(signature));
	input.read(reinterpret_cast<char*>(&version), sizeof(version));
	input.read(reinterpret_cast<char*>(&type), sizeof(type));
	input.read(reinterpret_cast<char*>(&channels), sizeof(channels));
	input.read(reinterpret_cast<char*>(&width), sizeof(width));
	input.read(reinterpret_cast<char*>(&height), sizeof(height));
	input.read(reinterpret_cast<char*>(&length), sizeof(length));
	if (!input.good()) {
		return false;
	}

	header.type = type;
	header.channels = static_cast<int>(channels);
	header.width = width;
	header.height = height;
	header.length = length;
	return signature == 293390619 && std::abs(version - 1.0f) < 1e-6f;
}

int WinnerIndex(const std::array<double, kDirs>& values) {
	return static_cast<int>(std::distance(values.begin(), std::max_element(values.begin(), values.end())));
}

std::array<double, kDirs> MeanDirectionResponse(const std::vector<float>& v1_complex) {
	std::array<double, kDirs> sums{};
	const int spatial_count = static_cast<int>(v1_complex.size()) / kDirs;
	for (int dir = 0; dir < kDirs; ++dir) {
		for (int spatial_index = 0; spatial_index < spatial_count; ++spatial_index) {
			sums[dir] += v1_complex[dir * spatial_count + spatial_index];
		}
	}
	for (double& value : sums) {
		value /= static_cast<double>(spatial_count);
	}
	return sums;
}

std::array<double, kDirs> PixelDirectionResponse(const std::vector<float>& v1_complex, int width, int height, int x, int y) {
	std::array<double, kDirs> values{};
	const int spatial_index = y * width + x;
	for (int dir = 0; dir < kDirs; ++dir) {
		values[dir] = v1_complex[dir * width * height + spatial_index];
	}
	return values;
}

void ProjectFiltersToDirections(
	const std::vector<float>& v1_filters,
	const std::vector<float>& motion_proj,
	int width,
	int height,
	std::vector<float>& projected_v1_complex)
{
	const int spatial_count = width * height;
	projected_v1_complex.assign(spatial_count * kDirs, 0.0f);
	for (int spatial_index = 0; spatial_index < spatial_count; ++spatial_index) {
		for (int dir = 0; dir < kDirs; ++dir) {
			float value = 0.0f;
			for (int scale = 0; scale < kScales; ++scale) {
				for (int filt = 0; filt < kFilters; ++filt) {
					const int src_index = (scale * kFilters + filt) * spatial_count + spatial_index;
					value += v1_filters[src_index] * motion_proj[filt * kDirs + dir];
				}
			}
			projected_v1_complex[dir * spatial_count + spatial_index] = value > 0.0f ? value * 10.0f : 1.0f;
		}
	}
}

double MeanAbsoluteDifference(const std::vector<float>& lhs, const std::vector<float>& rhs) {
	double sum = 0.0;
	for (size_t i = 0; i < lhs.size(); ++i) {
		sum += std::abs(static_cast<double>(lhs[i]) - static_cast<double>(rhs[i]));
	}
	return lhs.empty() ? 0.0 : sum / static_cast<double>(lhs.size());
}

double MaxAbsoluteDifference(const std::vector<float>& lhs, const std::vector<float>& rhs) {
	double max_diff = 0.0;
	for (size_t i = 0; i < lhs.size(); ++i) {
		max_diff = std::max(max_diff, std::abs(static_cast<double>(lhs[i]) - static_cast<double>(rhs[i])));
	}
	return max_diff;
}

struct Rgb {
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;
};

Rgb HeatColor(float t) {
	t = std::max(0.0f, std::min(1.0f, t));
	const float r = std::max(0.0f, std::min(1.0f, 1.5f - std::abs(4.0f * t - 3.0f)));
	const float g = std::max(0.0f, std::min(1.0f, 1.5f - std::abs(4.0f * t - 2.0f)));
	const float b = std::max(0.0f, std::min(1.0f, 1.5f - std::abs(4.0f * t - 1.0f)));
	return Rgb{
		static_cast<unsigned char>(255.0f * r),
		static_cast<unsigned char>(255.0f * g),
		static_cast<unsigned char>(255.0f * b)
	};
}

bool WritePpm(const std::string& path, int width, int height, const std::vector<Rgb>& pixels) {
	std::ofstream output(path, std::ios::binary);
	if (!output.is_open()) {
		return false;
	}
	output << "P6\n" << width << " " << height << "\n255\n";
	output.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size() * sizeof(Rgb)));
	return output.good();
}

bool SaveFilterMontage(
	const std::vector<float>& v1_filters,
	int width,
	int height,
	int scale_index,
	const std::string& output_path)
{
	const int tile_cols = 7;
	const int tile_rows = 4;
	const int border = 2;
	const int spatial_count = width * height;
	const int canvas_width = tile_cols * width + (tile_cols + 1) * border;
	const int canvas_height = tile_rows * height + (tile_rows + 1) * border;
	std::vector<Rgb> image(canvas_width * canvas_height, Rgb{ 18, 18, 32 });

	float min_value = std::numeric_limits<float>::max();
	float max_value = std::numeric_limits<float>::lowest();
	for (int filt = 0; filt < kFilters; ++filt) {
		for (int spatial_index = 0; spatial_index < spatial_count; ++spatial_index) {
			const int src_index = (scale_index * kFilters + filt) * spatial_count + spatial_index;
			min_value = std::min(min_value, v1_filters[src_index]);
			max_value = std::max(max_value, v1_filters[src_index]);
		}
	}
	const float denom = std::max(max_value - min_value, 1e-6f);

	for (int filt = 0; filt < kFilters; ++filt) {
		const int tile_x = filt % tile_cols;
		const int tile_y = filt / tile_cols;
		const int x0 = border + tile_x * (width + border);
		const int y0 = border + tile_y * (height + border);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const int spatial_index = y * width + x;
				const int src_index = (scale_index * kFilters + filt) * spatial_count + spatial_index;
				const float normalized = (v1_filters[src_index] - min_value) / denom;
				image[(y0 + y) * canvas_width + (x0 + x)] = HeatColor(normalized);
			}
		}
	}

	return WritePpm(output_path, canvas_width, canvas_height, image);
}

void PrintDirectionVector(const std::array<double, kDirs>& values) {
	for (int dir = 0; dir < kDirs; ++dir) {
		std::cout << " [" << dir << ":" << dir * 45 << "deg=" << values[dir] << "]";
	}
	std::cout << std::endl;
}

} // namespace

int main(int argc, char** argv) {
#if SNN_CPU_ONLY
	std::cout << "motion_energy_source_check requires GPU backend." << std::endl;
	return 0;
#else
	const std::string stimulus_path = argc > 1 ? argv[1] : kDefaultStimulusPath;

	std::ifstream input(stimulus_path, std::ios::binary);
	if (!input.is_open()) {
		std::cerr << "Failed to open stimulus file: " << stimulus_path << std::endl;
		return 1;
	}

	StimulusHeader header;
	if (!ReadStimulusHeader(input, header)) {
		std::cerr << "Failed to read VisualStimulus header from: " << stimulus_path << std::endl;
		return 1;
	}

	if (header.channels != 1) {
		std::cerr << "Expected grayscale stimulus, found channels=" << header.channels << std::endl;
		return 1;
	}

	const int frame_size = header.width * header.height * header.channels;
	std::vector<unsigned char> frame(frame_size, 0);
	std::vector<float> v1_complex(header.width * header.height * kDirs, 0.0f);
	std::vector<float> v1_filters(header.width * header.height * kFilters * kScales, 0.0f);
	std::vector<float> projected_v1_complex;
	MotionEnergy motion_energy_complex(header.width, header.height, header.channels);
	MotionEnergy motion_energy_filters(header.width, header.height, header.channels);
	std::vector<float> motion_proj(kFilters * kDirs, 0.0f);
	motion_energy_complex.getMotionProj(kSpeed, motion_proj.data());

	const int group_count = header.length / kFramesPerDirection;
	std::vector<std::array<double, kDirs>> grouped_direction_sums(group_count);
	std::vector<std::array<double, kDirs>> grouped_tail_sums(group_count);
	std::vector<std::array<double, kDirs>> grouped_center_sums(group_count);
	std::vector<std::array<double, kDirs>> grouped_center_tail_sums(group_count);
	std::vector<int> grouped_counts(group_count, 0);
	std::vector<int> grouped_tail_counts(group_count, 0);
	const int center_x = header.width / 2;
	const int center_y = header.height / 2;
	double mean_abs_diff_sum = 0.0;
	double max_abs_diff = 0.0;
	std::vector<bool> captured_montage_frame(group_count, false);
	std::vector<std::vector<float>> montage_filters(group_count);

	for (int frame_index = 0; frame_index < header.length; ++frame_index) {
		input.read(reinterpret_cast<char*>(frame.data()), frame_size);
		if (!input.good()) {
			std::cerr << "Failed to read frame " << frame_index << " from stimulus file." << std::endl;
			return 1;
		}

		motion_energy_complex.calcV1complex(frame.data(), v1_complex.data(), kSpeed, false);
		motion_energy_filters.calcV1filters(frame.data(), v1_filters.data(), false);
		ProjectFiltersToDirections(v1_filters, motion_proj, header.width, header.height, projected_v1_complex);
		mean_abs_diff_sum += MeanAbsoluteDifference(v1_complex, projected_v1_complex);
		max_abs_diff = std::max(max_abs_diff, MaxAbsoluteDifference(v1_complex, projected_v1_complex));
		const int stimulus_group = std::min(frame_index / kFramesPerDirection, group_count - 1);
		const int frame_in_group = frame_index % kFramesPerDirection;
		if (!captured_montage_frame[stimulus_group] && frame_in_group == kFramesPerDirection - 1) {
			montage_filters[stimulus_group] = v1_filters;
			captured_montage_frame[stimulus_group] = true;
		}
		const std::array<double, kDirs> dir_response = MeanDirectionResponse(v1_complex);
		const std::array<double, kDirs> center_response = PixelDirectionResponse(v1_complex, header.width, header.height, center_x, center_y);
		for (int dir = 0; dir < kDirs; ++dir) {
			grouped_direction_sums[stimulus_group][dir] += dir_response[dir];
			grouped_center_sums[stimulus_group][dir] += center_response[dir];
			if (frame_in_group >= kFramesPerDirection - kTailFramesForCheck) {
				grouped_tail_sums[stimulus_group][dir] += dir_response[dir];
				grouped_center_tail_sums[stimulus_group][dir] += center_response[dir];
			}
		}
		grouped_counts[stimulus_group]++;
		if (frame_in_group >= kFramesPerDirection - kTailFramesForCheck) {
			grouped_tail_counts[stimulus_group]++;
		}
	}

	std::cout << "Stimulus path: " << stimulus_path << std::endl;
	std::cout << "Stimulus size: " << header.width << "x" << header.height << "x" << header.channels
		<< ", frames=" << header.length << ", V1 speed=" << kSpeed << std::endl;
	bool wrote_any_montage = false;
	const std::string output_dir = "motion_energy_visualizations";
	std::filesystem::create_directories(output_dir);
	for (int group = 0; group < group_count; ++group) {
		if (!captured_montage_frame[group] || montage_filters[group].empty()) {
			continue;
		}
		for (int scale_index = 0; scale_index < kScales; ++scale_index) {
			std::ostringstream oss;
			oss << output_dir << "\\grating_dir_" << (group % kDirs) * 45
				<< "_block_" << group
				<< "_scale_" << scale_index
				<< "_frame_" << (group * kFramesPerDirection + (kFramesPerDirection - 1))
				<< ".ppm";
			if (!SaveFilterMontage(montage_filters[group], header.width, header.height, scale_index, oss.str())) {
				std::cerr << "Failed to write filter montage: " << oss.str() << std::endl;
				return 1;
			}
			wrote_any_montage = true;
		}
	}
	if (wrote_any_montage) {
		std::cout << "Saved 28-filter heatmaps to: " << output_dir << std::endl;
	}
	std::cout << "calcV1complex vs calcV1filters+motionProj mean abs diff per frame: "
		<< (header.length > 0 ? mean_abs_diff_sum / static_cast<double>(header.length) : 0.0) << std::endl;
	std::cout << "calcV1complex vs calcV1filters+motionProj max abs diff: " << max_abs_diff << std::endl;

	bool all_group_means_match = true;
	bool all_group_tails_match = true;
	bool all_center_tails_match = true;
	for (int group = 0; group < group_count; ++group) {
		if (grouped_counts[group] == 0) {
			continue;
		}
		std::array<double, kDirs> mean_response{};
		std::array<double, kDirs> tail_response{};
		std::array<double, kDirs> center_response{};
		std::array<double, kDirs> center_tail_response{};
		for (int dir = 0; dir < kDirs; ++dir) {
			mean_response[dir] = grouped_direction_sums[group][dir] / static_cast<double>(grouped_counts[group]);
			tail_response[dir] = grouped_tail_counts[group] > 0
				? grouped_tail_sums[group][dir] / static_cast<double>(grouped_tail_counts[group])
				: 0.0;
			center_response[dir] = grouped_center_sums[group][dir] / static_cast<double>(grouped_counts[group]);
			center_tail_response[dir] = grouped_tail_counts[group] > 0
				? grouped_center_tail_sums[group][dir] / static_cast<double>(grouped_tail_counts[group])
				: 0.0;
		}
		const int winner = WinnerIndex(mean_response);
		const int tail_winner = WinnerIndex(tail_response);
		const int center_winner = WinnerIndex(center_response);
		const int center_tail_winner = WinnerIndex(center_tail_response);
		const int expected = group % kDirs;
		all_group_means_match &= (winner == expected);
		all_group_tails_match &= (tail_winner == expected);
		all_center_tails_match &= (center_tail_winner == expected);

		std::cout << "Stimulus direction block " << group
			<< " (" << expected * 45 << "deg)"
			<< " -> V1 winner " << winner
			<< " (" << winner * 45 << "deg)" << std::endl;
		PrintDirectionVector(mean_response);
		std::cout << "Tail frames winner " << tail_winner
			<< " (" << tail_winner * 45 << "deg)" << std::endl;
		PrintDirectionVector(tail_response);
		std::cout << "Center pixel winner " << center_winner
			<< " (" << center_winner * 45 << "deg)" << std::endl;
		PrintDirectionVector(center_response);
		std::cout << "Center pixel tail winner " << center_tail_winner
			<< " (" << center_tail_winner * 45 << "deg)" << std::endl;
		PrintDirectionVector(center_tail_response);
	}

	std::cout << "V1 direction check (group mean): " << (all_group_means_match ? "PASS" : "MISMATCH") << std::endl;
	std::cout << "V1 direction check (tail frames): " << (all_group_tails_match ? "PASS" : "MISMATCH") << std::endl;
	std::cout << "V1 direction check (center tail): " << (all_center_tails_match ? "PASS" : "MISMATCH") << std::endl;
	return all_center_tails_match ? 0 : 2;
#endif
}
