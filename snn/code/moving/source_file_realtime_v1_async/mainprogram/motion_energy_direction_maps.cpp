#include <motion_energy.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kFilters = MotionEnergy::NumFilters;
constexpr int kScales = 3;
constexpr int kFramesPerDirection = 10;
constexpr std::array<int, kFilters> kV1PopDirOrder = {
	0, 1, 2, 3, 4, 5, 6,
	7, 8, 9, 10, 11, 12, 13,
	14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27
};
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
	for (int order_index = 0; order_index < kFilters; ++order_index) {
		const int filt = kV1PopDirOrder[order_index];
		for (int spatial_index = 0; spatial_index < spatial_count; ++spatial_index) {
			const int src_index = (scale_index * kFilters + filt) * spatial_count + spatial_index;
			min_value = std::min(min_value, v1_filters[src_index]);
			max_value = std::max(max_value, v1_filters[src_index]);
		}
	}
	const float denom = std::max(max_value - min_value, 1e-6f);

	for (int order_index = 0; order_index < kFilters; ++order_index) {
		const int filt = kV1PopDirOrder[order_index];
		const int tile_x = order_index % tile_cols;
		const int tile_y = order_index / tile_cols;
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

} // namespace

int main(int argc, char** argv) {
#if SNN_CPU_ONLY
	std::cout << "motion_energy_direction_maps requires GPU backend." << std::endl;
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
		std::cerr << "Failed to read stimulus header." << std::endl;
		return 1;
	}
	if (header.channels != 1) {
		std::cerr << "Expected grayscale stimulus." << std::endl;
		return 1;
	}

	const int group_count = header.length / kFramesPerDirection;
	const int frame_size = header.width * header.height * header.channels;
	std::vector<unsigned char> frame(frame_size, 0);
	std::vector<float> v1_filters(header.width * header.height * kFilters * kScales, 0.0f);
	std::vector<bool> captured(group_count, false);
	std::vector<std::vector<float>> captured_filters(group_count);

	MotionEnergy motion_energy(header.width, header.height, header.channels);
	for (int frame_index = 0; frame_index < header.length; ++frame_index) {
		input.read(reinterpret_cast<char*>(frame.data()), frame_size);
		if (!input.good()) {
			std::cerr << "Failed to read frame " << frame_index << std::endl;
			return 1;
		}
		motion_energy.calcV1filters(frame.data(), v1_filters.data(), false);
		const int group = std::min(frame_index / kFramesPerDirection, group_count - 1);
		const int frame_in_group = frame_index % kFramesPerDirection;
		if (!captured[group] && frame_in_group == kFramesPerDirection - 1) {
			captured[group] = true;
			captured_filters[group] = v1_filters;
		}
	}

	const std::filesystem::path output_dir = std::filesystem::current_path() / "motion_energy_visualizations";
	std::filesystem::create_directories(output_dir);
	for (int group = 0; group < group_count; ++group) {
		if (!captured[group]) {
			continue;
		}
		for (int scale_index = 0; scale_index < kScales; ++scale_index) {
			std::ostringstream oss;
			oss << "grating_dir_" << (group % 8) * 45
				<< "_block_" << group
				<< "_scale_" << scale_index
				<< "_frame_" << (group * kFramesPerDirection + (kFramesPerDirection - 1))
				<< ".ppm";
			if (!SaveFilterMontage(captured_filters[group], header.width, header.height, scale_index, (output_dir / oss.str()).string())) {
				std::cerr << "Failed to save " << oss.str() << std::endl;
				return 1;
			}
		}
	}

	std::cout << "Saved grating filter heatmaps to: " << output_dir.string() << std::endl;
	return 0;
#endif
}
