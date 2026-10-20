#include <motion_energy.h>

#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kDirs = 8;
constexpr int kFilters = MotionEnergy::NumFilters;
constexpr int kScales = 3;
constexpr int kFramesPerDirection = 10;
constexpr int kFrameSteps = 50;
constexpr int kTailSteps = 50;
constexpr int kTileCols = 4;
constexpr int kTileRows = 2;
constexpr float kSpeed = 1.5f;
constexpr float kBaseTimestep = 1.0f;
constexpr float kV1RateScale = 120.0f;
constexpr float kV1CdsExcTotalWeight = 0.24f;
constexpr float kV1CdsInhTotalWeight = 0.20f;
constexpr float kPdsToLipWeight = 0.060f;
constexpr float kLipInhibitionWeight = 0.30f;
constexpr float kCdsPdsAmpaWeight = 0.040f;
constexpr float kCdsPdsNmdaWeight = 0.028f;
constexpr float kPdsLateralInhibitionWeight = 0.35f;
constexpr float kCdsPdsExcSigma = 0.75f;
constexpr float kCdsPdsSpatialSigma = 0.85f;
constexpr float kPdsCrossBlockInhibitionGain = 0.06f;
constexpr int kPdsGridWidth = 10;
constexpr int kPdsGridHeight = 10;
constexpr int kPdsPoolSize = kPdsGridWidth * kPdsGridHeight;
constexpr int kLipGridWidth = 10;
constexpr int kLipGridHeight = 5;
constexpr int kLipPoolSize = 50;
constexpr int kLipInhibitionStride = 6;
constexpr const char* kDefaultStimulusPath =
	"D:\\Programing\\My_Program\\Carsim\\CARLsim4\\CARLsim4\\doc\\source\\tutorial\\5_motion_energy\\input\\grating.dat";

struct StimulusHeader {
	int type = 0;
	int channels = 0;
	int width = 0;
	int height = 0;
	int length = 0;
};

struct Rgb {
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;
};

ConnectionDescription MakeConnectionDescription(
	const std::vector<int>& source,
	const std::vector<int>& target,
	const std::vector<int>& type,
	const std::vector<float>& weight,
	const std::vector<float>& max_weight,
	const std::vector<int>& delay)
{
	ConnectionDescription desc;
	desc.SourceNeuron = source;
	desc.TargetNeuron = target;
	desc.Type = type;
	desc.Weight = weight;
	desc.MaxWeight = max_weight;
	desc.Delay = delay;
	desc.SynapseRule.assign(source.size(), -1);
	desc.TriggerRule.assign(source.size(), -1);
	desc.SynapseModel.assign(source.size(), ModelDescription{});
	return desc;
}

NeuronLayerDescription MakeIzhLayer(
	const std::string& model_name,
	int number_of_neurons,
	bool monitored,
	bool output,
	float a_value = 0.02f,
	float b_value = 0.2f,
	float c_value = -65.0f,
	float d_value = 8.0f,
	float v_th = 30.0f)
{
	NeuronLayerDescription layer;
	layer.ModelName = model_name;
	layer.numberofneuron = number_of_neurons;
	layer.isMonitored = monitored;
	layer.isOutput = output;
	layer.NeuronParameter["a"] = a_value;
	layer.NeuronParameter["b"] = b_value;
	layer.NeuronParameter["c"] = c_value;
	layer.NeuronParameter["d"] = d_value;
	layer.NeuronParameter["V_rest"] = c_value;
	layer.NeuronParameter["V_reset"] = c_value;
	layer.NeuronParameter["V_th"] = v_th;
	layer.NeuronParameter["R"] = 1.0f;
	layer.NeuronParameter["E_ampa"] = 0.0f;
	layer.NeuronParameter["ampa_tau"] = 5.0f;
	layer.NeuronParameter["E_gaba"] = -80.0f;
	layer.NeuronParameter["gaba_tau"] = 10.0f;
	layer.NeuronParameter["nmda_tau"] = 80.0f;
	layer.NeuronParameter["random_mu"] = std::array<float, 6>{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	layer.NeuronParameter["random_sigma"] = std::array<float, 6>{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	return layer;
}

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

bool WriteBmp(const std::string& path, int width, int height, const std::vector<Rgb>& pixels) {
	const int row_stride = ((width * 3 + 3) / 4) * 4;
	const int pixel_data_size = row_stride * height;
	const int file_size = 54 + pixel_data_size;

	std::ofstream output(path, std::ios::binary);
	if (!output.is_open()) {
		return false;
	}

	unsigned char file_header[14] = {
		'B', 'M',
		0, 0, 0, 0,
		0, 0, 0, 0,
		54, 0, 0, 0
	};
	file_header[2] = static_cast<unsigned char>(file_size);
	file_header[3] = static_cast<unsigned char>(file_size >> 8);
	file_header[4] = static_cast<unsigned char>(file_size >> 16);
	file_header[5] = static_cast<unsigned char>(file_size >> 24);
	output.write(reinterpret_cast<const char*>(file_header), sizeof(file_header));

	unsigned char info_header[40] = {
		40, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		1, 0,
		24, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		19, 11, 0, 0,
		19, 11, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	info_header[4] = static_cast<unsigned char>(width);
	info_header[5] = static_cast<unsigned char>(width >> 8);
	info_header[6] = static_cast<unsigned char>(width >> 16);
	info_header[7] = static_cast<unsigned char>(width >> 24);
	info_header[8] = static_cast<unsigned char>(height);
	info_header[9] = static_cast<unsigned char>(height >> 8);
	info_header[10] = static_cast<unsigned char>(height >> 16);
	info_header[11] = static_cast<unsigned char>(height >> 24);
	info_header[20] = static_cast<unsigned char>(pixel_data_size);
	info_header[21] = static_cast<unsigned char>(pixel_data_size >> 8);
	info_header[22] = static_cast<unsigned char>(pixel_data_size >> 16);
	info_header[23] = static_cast<unsigned char>(pixel_data_size >> 24);
	output.write(reinterpret_cast<const char*>(info_header), sizeof(info_header));

	std::vector<unsigned char> row(row_stride, 0);
	for (int y = height - 1; y >= 0; --y) {
		for (int x = 0; x < width; ++x) {
			const Rgb& pixel = pixels[y * width + x];
			row[x * 3 + 0] = pixel.b;
			row[x * 3 + 1] = pixel.g;
			row[x * 3 + 2] = pixel.r;
		}
		output.write(reinterpret_cast<const char*>(row.data()), row_stride);
	}
	return output.good();
}

bool SaveCdsDirectionMontage(
	const std::vector<int>& cds_counts,
	int width,
	int height,
	const std::string& output_path)
{
	const int border = 2;
	const int canvas_width = kTileCols * width + (kTileCols + 1) * border;
	const int canvas_height = kTileRows * height + (kTileRows + 1) * border;
	std::vector<Rgb> image(canvas_width * canvas_height, Rgb{ 18, 18, 32 });

	int min_value = std::numeric_limits<int>::max();
	int max_value = std::numeric_limits<int>::lowest();
	for (int dir = 0; dir < kDirs; ++dir) {
		for (int spatial_index = 0; spatial_index < width * height; ++spatial_index) {
			int value = cds_counts[dir * width * height + spatial_index];
			min_value = std::min(min_value, value);
			max_value = std::max(max_value, value);
		}
	}
	const float denom = std::max(static_cast<float>(max_value - min_value), 1e-6f);

	for (int dir = 0; dir < kDirs; ++dir) {
		const int tile_x = dir % kTileCols;
		const int tile_y = dir / kTileCols;
		const int x0 = border + tile_x * (width + border);
		const int y0 = border + tile_y * (height + border);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const int spatial_index = y * width + x;
				const int value = cds_counts[dir * width * height + spatial_index];
				const float normalized = (static_cast<float>(value) - static_cast<float>(min_value)) / denom;
				image[(y0 + y) * canvas_width + (x0 + x)] = HeatColor(normalized);
			}
		}
	}

	return WriteBmp(output_path, canvas_width, canvas_height, image);
}

bool SaveDirectionMontage(
	const std::vector<int>& counts,
	int map_width,
	int map_height,
	int pool_size,
	const std::string& output_path)
{
	if (counts.size() != static_cast<size_t>(kDirs * pool_size)) {
		return false;
	}

	const int border = 2;
	const int canvas_width = kTileCols * map_width + (kTileCols + 1) * border;
	const int canvas_height = kTileRows * map_height + (kTileRows + 1) * border;
	std::vector<Rgb> image(canvas_width * canvas_height, Rgb{ 18, 18, 32 });

	int min_value = std::numeric_limits<int>::max();
	int max_value = std::numeric_limits<int>::lowest();
	for (int value : counts) {
		min_value = std::min(min_value, value);
		max_value = std::max(max_value, value);
	}
	const float denom = std::max(static_cast<float>(max_value - min_value), 1e-6f);

	for (int dir = 0; dir < kDirs; ++dir) {
		const int tile_x = dir % kTileCols;
		const int tile_y = dir / kTileCols;
		const int x0 = border + tile_x * (map_width + border);
		const int y0 = border + tile_y * (map_height + border);
		for (int y = 0; y < map_height; ++y) {
			for (int x = 0; x < map_width; ++x) {
				const int local_index = y * map_width + x;
				const int value = counts[dir * pool_size + local_index];
				const float normalized = (static_cast<float>(value) - static_cast<float>(min_value)) / denom;
				image[(y0 + y) * canvas_width + (x0 + x)] = HeatColor(normalized);
			}
		}
	}

	return WriteBmp(output_path, canvas_width, canvas_height, image);
}

float CircularDistance(int a, int b)
{
	int diff = std::abs(a - b);
	diff = std::min(diff, kDirs - diff);
	return static_cast<float>(diff);
}

float DirectionSimilarity(int src_dir, int dst_dir, float sigma)
{
	float d = CircularDistance(src_dir, dst_dir);
	return std::exp(-(d * d) / (2.0f * sigma * sigma));
}

bool SparseConnect(int src, int dst, int stride = 10)
{
	unsigned int hash = static_cast<unsigned int>(src * 1103515245u + dst * 12345u + 0x9e3779b9u);
	return (hash % static_cast<unsigned int>(stride)) == 0u;
}

std::vector<int> ReadCdsCounts(
	const std::string& spike_file,
	int cds_base,
	int cds_count,
	int width,
	int height)
{
	std::ifstream input(spike_file);
	if (!input.is_open()) {
		return {};
	}

	std::vector<int> counts(kDirs * width * height, 0);
	std::string line;
	while (std::getline(input, line)) {
		std::istringstream iss(line);
		int time = 0;
		long neuron = -1;
		if (!(iss >> time >> neuron)) {
			continue;
		}
		if (neuron < cds_base || neuron >= cds_base + cds_count) {
			continue;
		}
		const int local_index = static_cast<int>(neuron) - cds_base;
		const int dir = local_index % kDirs;
		const int spatial_index = local_index / kDirs;
		counts[dir * width * height + spatial_index]++;
	}
	return counts;
}

std::vector<int> ReadDirectionalSpatialCounts(
	const std::string& spike_file,
	int base_index,
	int neuron_count,
	int pool_size)
{
	std::ifstream input(spike_file);
	if (!input.is_open()) {
		return {};
	}

	std::vector<int> counts(neuron_count, 0);
	std::string line;
	while (std::getline(input, line)) {
		std::istringstream iss(line);
		int time = 0;
		long neuron = -1;
		if (!(iss >> time >> neuron)) {
			continue;
		}
		if (neuron >= base_index && neuron < base_index + neuron_count) {
			int local_index = static_cast<int>(neuron) - base_index;
			int dir = local_index / pool_size;
			if (dir >= 0 && dir < kDirs) {
				counts[local_index]++;
			}
		}
	}
	return counts;
}

std::vector<std::vector<unsigned char>> ReadAllFrames(
	const std::string& stimulus_path,
	StimulusHeader& header)
{
	std::ifstream input(stimulus_path, std::ios::binary);
	if (!input.is_open()) {
		return {};
	}
	if (!ReadStimulusHeader(input, header)) {
		return {};
	}
	if (header.channels != 1) {
		return {};
	}

	const int frame_size = header.width * header.height * header.channels;
	std::vector<std::vector<unsigned char>> frames(header.length, std::vector<unsigned char>(frame_size, 0));
	for (int frame_index = 0; frame_index < header.length; ++frame_index) {
		input.read(reinterpret_cast<char*>(frames[frame_index].data()), frame_size);
		if (!input.good()) {
			return {};
		}
	}
	return frames;
}

std::vector<std::vector<float>> ComputeAllV1Complex(
	const std::vector<std::vector<unsigned char>>& frames,
	int width,
	int height)
{
	const int v1_count = width * height * kDirs;
	std::vector<std::vector<float>> all_v1_complex(frames.size(), std::vector<float>(v1_count, 0.0f));
	std::vector<float> v1_complex(v1_count, 0.0f);
	MotionEnergy motion_energy(width, height, 1);
	for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index) {
		motion_energy.calcV1complex(const_cast<unsigned char*>(frames[frame_index].data()), v1_complex.data(), kSpeed, false);
		all_v1_complex[frame_index] = v1_complex;
	}
	return all_v1_complex;
}

std::vector<std::vector<float>> ComputeAllV1Filters(
	const std::vector<std::vector<unsigned char>>& frames,
	int width,
	int height)
{
	const int v1_count = width * height * kFilters * kScales;
	std::vector<std::vector<float>> all_v1_filters(frames.size(), std::vector<float>(v1_count, 0.0f));
	std::vector<float> v1_filters(v1_count, 0.0f);
	MotionEnergy motion_energy(width, height, 1);
	for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index) {
		motion_energy.calcV1filters(const_cast<unsigned char*>(frames[frame_index].data()), v1_filters.data(), false);
		all_v1_filters[frame_index] = v1_filters;
	}
	return all_v1_filters;
}

bool RunCdsPdsLipBlockSimulation(
	const std::vector<std::vector<float>>& all_v1_filters,
	const std::vector<float>& motion_proj,
	int block_index,
	int width,
	int height,
	const std::filesystem::path& output_dir)
{
	const int v1_count = width * height * kFilters * kScales;
	const int cds_count = width * height * kDirs;
	const int pds_count = kDirs * kPdsPoolSize;
	const int lip_count = kDirs * kLipPoolSize;
	const int simulation_steps = kFramesPerDirection * kFrameSteps + kTailSteps;
	const int v1_base = 0;
	const int cds_base = v1_base + v1_count;
	const int pds_base = cds_base + cds_count;
	const int lip_base = pds_base + pds_count;
	const std::string model_name = "TimeDrivenIzhikevic_I_Exponential_Decay_GPU";

	std::list<NeuronLayerDescription> neu_description;
	std::list<ConnectionDescription> syn_description;
	std::list<LearningRuleDescription> learn_description;

	NeuronLayerDescription v1_input_layer;
	v1_input_layer.ModelName = "InputSpikeNeuronModel";
	v1_input_layer.numberofneuron = v1_count;
	v1_input_layer.isMonitored = false;
	v1_input_layer.isOutput = false;
	neu_description.push_back(v1_input_layer);
	neu_description.push_back(MakeIzhLayer(model_name, cds_count, true, true));
	neu_description.push_back(MakeIzhLayer(model_name, pds_count, false, true));
	neu_description.push_back(MakeIzhLayer(model_name, lip_count, false, true));

	{
		std::vector<int> source;
		std::vector<int> target;
		std::vector<int> type;
		std::vector<float> weight;
		std::vector<float> max_weight;
		std::vector<int> delay;
		source.reserve(cds_count * kFilters * kScales);
		target.reserve(cds_count * kFilters * kScales);
		type.reserve(cds_count * kFilters * kScales);
		weight.reserve(cds_count * kFilters * kScales);
		max_weight.reserve(cds_count * kFilters * kScales);
		delay.reserve(cds_count * kFilters * kScales);

		std::array<float, kDirs> positive_sums{};
		std::array<float, kDirs> negative_sums{};
		for (int dir = 0; dir < kDirs; ++dir) {
			float positive_sum = 0.0f;
			float negative_sum = 0.0f;
			for (int filter = 0; filter < kFilters; ++filter) {
				float coeff = motion_proj[filter * kDirs + dir];
				if (coeff > 0.0f) {
					positive_sum += coeff;
				}
				else {
					negative_sum += -coeff;
				}
			}
			positive_sums[dir] = positive_sum * static_cast<float>(kScales);
			negative_sums[dir] = negative_sum * static_cast<float>(kScales);
		}

		for (int spatial_index = 0; spatial_index < width * height; ++spatial_index) {
			for (int dst_dir = 0; dst_dir < kDirs; ++dst_dir) {
				int dst_neuron = cds_base + spatial_index * kDirs + dst_dir;
				for (int scale = 0; scale < kScales; ++scale) {
					for (int filter = 0; filter < kFilters; ++filter) {
						float coeff = motion_proj[filter * kDirs + dst_dir];
						if (coeff == 0.0f) {
							continue;
						}
						int src_index = (scale * kFilters + filter) * width * height + spatial_index;
						source.push_back(v1_base + src_index);
						target.push_back(dst_neuron);
						if (coeff > 0.0f) {
							float normalized = coeff / std::max(positive_sums[dst_dir], 1e-6f);
							type.push_back(0);
							weight.push_back(kV1CdsExcTotalWeight * normalized);
							max_weight.push_back(kV1CdsExcTotalWeight * normalized);
						}
						else {
							float normalized = (-coeff) / std::max(negative_sums[dst_dir], 1e-6f);
							type.push_back(1);
							weight.push_back(kV1CdsInhTotalWeight * normalized);
							max_weight.push_back(kV1CdsInhTotalWeight * normalized);
						}
						delay.push_back(1);
					}
				}
			}
		}
		syn_description.push_back(MakeConnectionDescription(source, target, type, weight, max_weight, delay));
	}

	{
		std::vector<int> source;
		std::vector<int> target;
		std::vector<int> type;
		std::vector<float> weight;
		std::vector<float> max_weight;
		std::vector<int> delay;
		source.reserve(cds_count * kDirs * 18);
		target.reserve(cds_count * kDirs * 18);
		type.reserve(cds_count * kDirs * 18);
		weight.reserve(cds_count * kDirs * 18);
		max_weight.reserve(cds_count * kDirs * 18);
		delay.reserve(cds_count * kDirs * 18);

		for (int cds = 0; cds < cds_count; ++cds) {
			int src_dir = cds % kDirs;
			int spatial_index = cds / kDirs;
			int src_x = spatial_index % width;
			int src_y = spatial_index / width;
			float block_xf = ((static_cast<float>(src_x) + 0.5f) * static_cast<float>(kPdsGridWidth) / static_cast<float>(width)) - 0.5f;
			float block_yf = ((static_cast<float>(src_y) + 0.5f) * static_cast<float>(kPdsGridHeight) / static_cast<float>(height)) - 0.5f;
			int center_block_x = static_cast<int>(std::round(block_xf));
			int center_block_y = static_cast<int>(std::round(block_yf));

			struct BlockCandidate {
				int local_index;
				float gain;
			};
			std::vector<BlockCandidate> block_candidates;
			float block_gain_sum = 0.0f;
			for (int dy = -1; dy <= 1; ++dy) {
				for (int dx = -1; dx <= 1; ++dx) {
					int block_x = center_block_x + dx;
					int block_y = center_block_y + dy;
					if (block_x < 0 || block_x >= kPdsGridWidth || block_y < 0 || block_y >= kPdsGridHeight) {
						continue;
					}
					float delta_x = static_cast<float>(block_x) - block_xf;
					float delta_y = static_cast<float>(block_y) - block_yf;
					float spatial_gain = std::exp(-(delta_x * delta_x + delta_y * delta_y) / (2.0f * kCdsPdsSpatialSigma * kCdsPdsSpatialSigma));
					if (spatial_gain <= 0.0f) {
						continue;
					}
					block_candidates.push_back(BlockCandidate{ block_y * kPdsGridWidth + block_x, spatial_gain });
					block_gain_sum += spatial_gain;
				}
			}
			if (block_candidates.empty()) {
				continue;
			}

			for (int pds_dir = 0; pds_dir < kDirs; ++pds_dir) {
				float exc_similarity = DirectionSimilarity(src_dir, pds_dir, kCdsPdsExcSigma);
				for (const BlockCandidate& candidate : block_candidates) {
					float spatial_gain = candidate.gain / std::max(block_gain_sum, 1e-6f);
					int pds = pds_base + pds_dir * kPdsPoolSize + candidate.local_index;

					source.push_back(cds_base + cds);
					target.push_back(pds);
					type.push_back(0);
					weight.push_back(kCdsPdsAmpaWeight * exc_similarity * spatial_gain);
					max_weight.push_back(kCdsPdsAmpaWeight * exc_similarity * spatial_gain);
					delay.push_back(1);

					source.push_back(cds_base + cds);
					target.push_back(pds);
					type.push_back(2);
					weight.push_back(kCdsPdsNmdaWeight * exc_similarity * spatial_gain);
					max_weight.push_back(kCdsPdsNmdaWeight * exc_similarity * spatial_gain);
					delay.push_back(2);
				}
			}
		}
		syn_description.push_back(MakeConnectionDescription(source, target, type, weight, max_weight, delay));
	}

	{
		std::vector<int> source;
		std::vector<int> target;
		std::vector<int> type;
		std::vector<float> weight;
		std::vector<float> max_weight;
		std::vector<int> delay;
		for (int src = 0; src < pds_count; ++src) {
			int src_dir = src / kPdsPoolSize;
			int src_local_index = src % kPdsPoolSize;
			for (int dst = 0; dst < pds_count; ++dst) {
				int dst_dir = dst / kPdsPoolSize;
				int dst_local_index = dst % kPdsPoolSize;
				if (src == dst || src_dir == dst_dir) {
					continue;
				}
				float inhibition_gain = src_local_index == dst_local_index ? 1.0f : kPdsCrossBlockInhibitionGain;
				source.push_back(pds_base + src);
				target.push_back(pds_base + dst);
				type.push_back(1);
				weight.push_back(0.35f * kPdsLateralInhibitionWeight * inhibition_gain);
				max_weight.push_back(0.35f * kPdsLateralInhibitionWeight * inhibition_gain);
				delay.push_back(1);
			}
		}
		syn_description.push_back(MakeConnectionDescription(source, target, type, weight, max_weight, delay));
	}

	{
		std::vector<int> source;
		std::vector<int> target;
		std::vector<int> type;
		std::vector<float> weight;
		std::vector<float> max_weight;
		std::vector<int> delay;
		source.reserve(pds_count * 8);
		target.reserve(pds_count * 8);
		type.reserve(pds_count * 8);
		weight.reserve(pds_count * 8);
		max_weight.reserve(pds_count * 8);
		delay.reserve(pds_count * 8);
		for (int pds = 0; pds < pds_count; ++pds) {
			int src_dir = pds / kPdsPoolSize;
			int src_local_index = pds % kPdsPoolSize;
			int src_block_x = src_local_index % kPdsGridWidth;
			int src_block_y = src_local_index / kPdsGridWidth;
			for (int lip = 0; lip < kLipPoolSize; ++lip) {
				int rf_x = lip % kPdsGridWidth;
				int rf_y = lip / kPdsGridWidth;
				int center_y = rf_y * 2;
				if (std::abs(src_block_x - rf_x) > 1 || std::abs(src_block_y - center_y) > 1) {
					continue;
				}
				int lip_neuron = lip_base + src_dir * kLipPoolSize + lip;
				source.push_back(pds_base + pds);
				target.push_back(lip_neuron);
				type.push_back(0);
				weight.push_back(kPdsToLipWeight);
				max_weight.push_back(kPdsToLipWeight);
				delay.push_back(1);
			}
		}
		syn_description.push_back(MakeConnectionDescription(source, target, type, weight, max_weight, delay));
	}

	{
		std::vector<int> source;
		std::vector<int> target;
		std::vector<int> type;
		std::vector<float> weight;
		std::vector<float> max_weight;
		std::vector<int> delay;
		for (int src_dir = 0; src_dir < kDirs; ++src_dir) {
			for (int dst_dir = 0; dst_dir < kDirs; ++dst_dir) {
				if (src_dir == dst_dir) {
					continue;
				}
				float delta = 2.0f * 3.14159265358979323846f * static_cast<float>(src_dir - dst_dir) / static_cast<float>(kDirs);
				float inh_strength = std::max(0.0f, -std::cos(delta));
				if (inh_strength <= 0.0f) {
					continue;
				}
				for (int src_idx = 0; src_idx < kLipPoolSize; ++src_idx) {
					int src_neuron = lip_base + src_dir * kLipPoolSize + src_idx;
					for (int dst_idx = 0; dst_idx < kLipPoolSize; ++dst_idx) {
						int dst_neuron = lip_base + dst_dir * kLipPoolSize + dst_idx;
						if (!SparseConnect(src_neuron, dst_neuron, kLipInhibitionStride)) {
							continue;
						}
						source.push_back(src_neuron);
						target.push_back(dst_neuron);
						type.push_back(1);
						weight.push_back(kLipInhibitionWeight * inh_strength);
						max_weight.push_back(kLipInhibitionWeight * inh_strength);
						delay.push_back(1);
					}
				}
			}
		}
		syn_description.push_back(MakeConnectionDescription(source, target, type, weight, max_weight, delay));
	}

	Simulation* simulation = new Simulation(
		neu_description,
		syn_description,
		learn_description,
		simulation_steps,
		kBaseTimestep,
		1,
		EVENT_QUEUE_TIMING_WHEEL,
		64
	);

	std::vector<int> spike_times;
	std::vector<int> spike_neurons;
	spike_times.reserve(kFramesPerDirection * v1_count / 4);
	spike_neurons.reserve(kFramesPerDirection * v1_count / 4);
	std::mt19937 poisson_rng(123456u + static_cast<unsigned int>(block_index));
	std::uniform_real_distribution<float> uniform01(0.0f, 1.0f);

	const int start_frame = block_index * kFramesPerDirection;
	const int end_frame = std::min(start_frame + kFramesPerDirection, static_cast<int>(all_v1_filters.size()));
	for (int frame_index = start_frame; frame_index < end_frame; ++frame_index) {
		const std::vector<float>& v1_filters = all_v1_filters[frame_index];
		const int event_time = (frame_index - start_frame) * kFrameSteps;
		for (int hold_step = 0; hold_step < kFrameSteps; ++hold_step) {
			const int scheduled_time = event_time + hold_step;
			for (int neuron = 0; neuron < v1_count; ++neuron) {
				const float rate_hz = std::max(0.0f, v1_filters[neuron]) * kV1RateScale;
				const float spike_probability = std::min(rate_hz * 0.001f, 1.0f);
				if (spike_probability > 0.0f && uniform01(poisson_rng) < spike_probability) {
					spike_times.push_back(scheduled_time);
					spike_neurons.push_back(v1_base + neuron);
				}
			}
		}
	}

	std::ostringstream spike_name;
	spike_name << "mt_cds_spikes_block_" << block_index << ".txt";
	const std::filesystem::path spike_path = output_dir / spike_name.str();

	simulation->AddExternalSpikeActivity(spike_times, spike_neurons);
	simulation->AddFileOutputSpikeDriver(spike_path.string());
	simulation->SetPrintSimulationTime(false);
	simulation->InitSimulation();
	simulation->RunSimulationStep(simulation_steps);
	delete simulation;

	std::vector<int> cds_counts = ReadCdsCounts(spike_path.string(), cds_base, cds_count, width, height);
	std::vector<int> pds_counts = ReadDirectionalSpatialCounts(spike_path.string(), pds_base, pds_count, kPdsPoolSize);
	std::vector<int> lip_counts = ReadDirectionalSpatialCounts(spike_path.string(), lip_base, lip_count, kLipPoolSize);
	if (cds_counts.empty() || pds_counts.empty() || lip_counts.empty()) {
		return false;
	}

	std::ostringstream image_name;
	image_name << "mt_cds_dir_maps_grating_" << (block_index % 8) * 45
		<< "_block_" << block_index << ".bmp";
	std::ostringstream pds_name;
	pds_name << "mt_pds_dir_maps_grating_" << (block_index % 8) * 45
		<< "_block_" << block_index << ".bmp";
	std::ostringstream lip_name;
	lip_name << "mt_lip_dir_maps_grating_" << (block_index % 8) * 45
		<< "_block_" << block_index << ".bmp";
	return SaveCdsDirectionMontage(cds_counts, width, height, (output_dir / image_name.str()).string()) &&
		SaveDirectionMontage(pds_counts, kPdsGridWidth, kPdsGridHeight, kPdsPoolSize, (output_dir / pds_name.str()).string()) &&
		SaveDirectionMontage(lip_counts, kLipGridWidth, kLipGridHeight, kLipPoolSize, (output_dir / lip_name.str()).string());
}

} // namespace

int main(int argc, char** argv) {
#if SNN_CPU_ONLY
	std::cout << "mt_cds_direction_maps requires GPU backend." << std::endl;
	return 0;
#else
	const std::string stimulus_path = argc > 1 ? argv[1] : kDefaultStimulusPath;
	StimulusHeader header;
	std::vector<std::vector<unsigned char>> frames = ReadAllFrames(stimulus_path, header);
	if (frames.empty()) {
		std::cerr << "Failed to read stimulus frames from: " << stimulus_path << std::endl;
		return 1;
	}

	MotionEnergy motion_energy(header.width, header.height, header.channels);
	std::vector<float> motion_proj(kFilters * kDirs, 0.0f);
	motion_energy.getMotionProj(kSpeed, motion_proj.data());
	std::vector<std::vector<float>> all_v1_filters = ComputeAllV1Filters(frames, header.width, header.height);
	const std::filesystem::path output_dir = std::filesystem::current_path() / "mt_cds_visualizations";
	std::filesystem::create_directories(output_dir);

	const int block_count = header.length / kFramesPerDirection;
	auto start = std::chrono::high_resolution_clock::now();
	for (int block_index = 0; block_index < block_count; ++block_index) {
		if (!RunCdsPdsLipBlockSimulation(all_v1_filters, motion_proj, block_index, header.width, header.height, output_dir)) {
			std::cerr << "Failed to generate CDS/PDS/LIP maps for block " << block_index << std::endl;
			return 1;
		}
	}
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli> elapsed = end - start;

	std::cout << "Saved MT CDS/PDS/LIP direction heatmaps to: " << output_dir.string() << std::endl;
	std::cout << "Processed blocks: " << block_count << ", elapsed: " << elapsed.count() << " ms." << std::endl;
	return 0;
#endif
}
