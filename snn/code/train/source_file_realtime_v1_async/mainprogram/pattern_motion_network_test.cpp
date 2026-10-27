#include <motion_energy.h>

#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kDirs = 8;
constexpr int kFilters = MotionEnergy::NumFilters;
constexpr int kScales = 3;
constexpr int kSpeeds = 3;
constexpr int kTileCols = 4;
constexpr int kTileRows = 2;
constexpr float kPi = 3.14159265358979323846f;
constexpr std::array<double, kSpeeds> kSpeedValues{ 0.125, 1.5, 9.0 };

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

void AddPlaidFrame(
	std::vector<unsigned char>& frame,
	int width,
	int height,
	int frame_index,
	float temporal_speed,
	float direction_a_deg,
	float direction_b_deg,
	float spatial_frequency)
{
	const float phase_t = temporal_speed * static_cast<float>(frame_index);
	const float theta_a = direction_a_deg * kPi / 180.0f;
	const float theta_b = direction_b_deg * kPi / 180.0f;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float xf = (static_cast<float>(x) - width * 0.5f) / static_cast<float>(width);
			float yf = (static_cast<float>(y) - height * 0.5f) / static_cast<float>(height);

			float proj_a = std::cos(theta_a) * xf + std::sin(theta_a) * yf;
			float proj_b = std::cos(theta_b) * xf + std::sin(theta_b) * yf;

			float g1 = std::sin(2.0f * kPi * (spatial_frequency * proj_a - phase_t));
			float g2 = std::sin(2.0f * kPi * (spatial_frequency * proj_b - phase_t));
			float value = 127.5f + 63.75f * (g1 + g2);
			value = std::max(0.0f, std::min(255.0f, value));
			frame[y * width + x] = static_cast<unsigned char>(value);
		}
	}
}

void AddGratingFrame(
	std::vector<unsigned char>& frame,
	int width,
	int height,
	int frame_index,
	float temporal_speed,
	float direction_deg,
	float spatial_frequency)
{
	const float phase_t = temporal_speed * static_cast<float>(frame_index);
	const float theta = direction_deg * kPi / 180.0f;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float xf = (static_cast<float>(x) - width * 0.5f) / static_cast<float>(width);
			float yf = (static_cast<float>(y) - height * 0.5f) / static_cast<float>(height);

			float proj = std::cos(theta) * xf + std::sin(theta) * yf;
			float g = std::sin(2.0f * kPi * (spatial_frequency * proj - phase_t));
			float value = 127.5f + 127.5f * g;
			value = std::max(0.0f, std::min(255.0f, value));
			frame[y * width + x] = static_cast<unsigned char>(value);
		}
	}
}

std::vector<int> ReadLipPoolCounts(const std::string& spike_file, int lip_base, int lip_pool_size)
{
	std::ifstream input(spike_file);
	if (!input.is_open()) {
		return {};
	}

	std::vector<int> counts(kDirs, 0);
	std::string line;
	while (std::getline(input, line)) {
		std::istringstream iss(line);
		int time = 0;
		long neuron = -1;
		if (!(iss >> time >> neuron)) {
			continue;
		}
		if (neuron >= lip_base) {
			int lip_index = static_cast<int>(neuron) - lip_base;
			int dir = lip_index / lip_pool_size;
			if (dir >= 0 && dir < kDirs) {
				counts[dir]++;
			}
		}
	}

	return counts;
}

std::vector<int> ReadDirectionPoolCounts(const std::string& spike_file, int base_index, int neuron_count, int pool_size)
{
	std::ifstream input(spike_file);
	if (!input.is_open()) {
		return {};
	}

	std::vector<int> counts(kDirs, 0);
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
				counts[dir]++;
			}
		}
	}

	return counts;
}

int SelectWinnerDirection(const std::vector<int>& counts)
{
	if (counts.empty()) {
		return -1;
	}
	int best_dir = 0;
	for (int dir = 1; dir < static_cast<int>(counts.size()); ++dir) {
		if (counts[dir] > counts[best_dir]) {
			best_dir = dir;
		}
	}
	return best_dir;
}

float DirectionIndexToDegrees(int dir)
{
	return 360.0f * static_cast<float>(dir) / static_cast<float>(kDirs);
}

Rgb HeatColor(float t)
{
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

bool WriteBmp(const std::string& path, int width, int height, const std::vector<Rgb>& pixels)
{
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

std::vector<int> ReadSpatialDirectionCounts(
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

std::vector<int> CollapseSpeedPlanes(
	const std::vector<int>& counts_with_speed,
	int pool_size,
	int speed_count)
{
	if (counts_with_speed.size() != static_cast<size_t>(kDirs * pool_size * speed_count)) {
		return {};
	}
	std::vector<int> collapsed(kDirs * pool_size, 0);
	for (int speed_index = 0; speed_index < speed_count; ++speed_index) {
		const int speed_offset = speed_index * kDirs * pool_size;
		for (int dir = 0; dir < kDirs; ++dir) {
			for (int local_index = 0; local_index < pool_size; ++local_index) {
				collapsed[dir * pool_size + local_index] += counts_with_speed[speed_offset + dir * pool_size + local_index];
			}
		}
	}
	return collapsed;
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

} // namespace

int main(int argc, char** argv) {
#if SNN_CPU_ONLY
	std::cout << "pattern_motion_network_test requires GPU backend." << std::endl;
	return 0;
#else
	const int width = 40;
	const int height = 40;
	const int frame_count = 8;
	const int frame_steps = 50;
	const int simulation_steps = frame_count * frame_steps + 50;
	const float base_timestep = 0.1f;
	const int queue_count = 1;
	const float v1_rate_scale = 120.0f;
	const float pds_to_lip_weight = 0.060f;
	const float lip_inhibition_weight = 0.18f;
	const float cds_pds_ampa_weight = 0.040f;
	const float cds_pds_nmda_weight = 0.028f;
	const float cds_pds_opp_weight = 0.050f;
	const float cds_pds_norm_weight = 0.012f;
	const float v1_cds_exc_total_weight = 0.24f;
	const float v1_cds_inh_total_weight = 0.20f;
	const float cds_fs_ampa_weight = 0.022f;
	const float pds_fs_to_pds_norm_weight = 0.055f;
	const float pds_fs_to_pds_opp_weight = 0.040f;
	const float pds_lateral_inhibition_weight = 0.35f;
	const float cds_pds_exc_sigma = 0.75f;
	const float cds_pds_opp_sigma = 0.90f;
	const float cds_pds_norm_sigma = 1.5f;
	const float pds_fs_to_pds_norm_sigma = 1.15f;
	const float pds_fs_to_pds_opp_sigma = 0.75f;
	const float pds_cross_block_inhibition_gain = 0.06f;
	const float cds_pds_spatial_sigma = 0.85f;
	const bool enable_pds_fs_path = false;
	const std::string model_name = "TimeDrivenIzhikevic_I_Exponential_Decay_GPU";
	const std::string stimulus_mode = argc > 1 ? argv[1] : "plaid";
	const float grating_direction_deg = argc > 2 ? std::stof(argv[2]) : 0.0f;
	const float plaid_direction_a_deg = argc > 2 ? std::stof(argv[2]) : 0.0f;
	const float plaid_direction_b_deg = argc > 3 ? std::stof(argv[3]) : 120.0f;

	const int v1_channels_per_speed = width * height * kFilters * kScales;
	const int v1_count = v1_channels_per_speed * kSpeeds;
	const int v1_input_count = v1_count;
	const int cds_count = width * height * kDirs * kSpeeds;
	const int pds_grid_width = 10;
	const int pds_grid_height = 10;
	const int pds_pool_size = pds_grid_width * pds_grid_height;
	const int pds_count = kDirs * pds_pool_size;
	const int pds_fs_count = pds_count;
	const int lip_grid_width = 10;
	const int lip_grid_height = 5;
	const int lip_pool_size = 50;
	const int lip_count = kDirs * lip_pool_size;
	const int v1_base = 0;
	const int cds_base = v1_base + v1_input_count;
	const int pds_base = cds_base + cds_count;
	const int pds_fs_base = pds_base + pds_count;
	const int lip_base = pds_fs_base + pds_fs_count;
	const int cds_plane_count = width * height * kDirs;

	std::list<NeuronLayerDescription> neu_description;
	std::list<ConnectionDescription> syn_description;
	std::list<LearningRuleDescription> learn_description;
	MotionEnergy motion_energy(width, height, 1);

	NeuronLayerDescription v1_input_layer;
	v1_input_layer.ModelName = "InputSpikeNeuronModel";
	v1_input_layer.numberofneuron = v1_input_count;
	v1_input_layer.isMonitored = false;
	v1_input_layer.isOutput = false;
	neu_description.push_back(v1_input_layer);

	neu_description.push_back(MakeIzhLayer(model_name, cds_count, false, false));
	neu_description.push_back(MakeIzhLayer(model_name, pds_count, false, true));
	neu_description.push_back(MakeIzhLayer(model_name, pds_fs_count, false, false, 0.1f, 0.2f, -65.0f, 2.0f));
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
		for (int speed_index = 0; speed_index < kSpeeds; ++speed_index) {
			std::vector<float> motion_proj(kFilters * kDirs, 0.0f);
			motion_energy.getMotionProj(kSpeedValues[speed_index], motion_proj.data());
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
					int dst_neuron = cds_base + speed_index * cds_plane_count + spatial_index * kDirs + dst_dir;
					for (int scale = 0; scale < kScales; ++scale) {
						for (int filter = 0; filter < kFilters; ++filter) {
							float coeff = motion_proj[filter * kDirs + dst_dir];
							if (coeff == 0.0f) {
								continue;
							}
							int src_local = (scale * kFilters + filter) * width * height + spatial_index;
							int src_neuron = v1_base + speed_index * v1_channels_per_speed + src_local;
							source.push_back(src_neuron);
							target.push_back(dst_neuron);
							if (coeff > 0.0f) {
								float normalized = coeff / std::max(positive_sums[dst_dir], 1e-6f);
								type.push_back(0);
								weight.push_back(v1_cds_exc_total_weight * normalized);
								max_weight.push_back(v1_cds_exc_total_weight * normalized);
							}
							else {
								float normalized = (-coeff) / std::max(negative_sums[dst_dir], 1e-6f);
								type.push_back(1);
								weight.push_back(v1_cds_inh_total_weight * normalized);
								max_weight.push_back(v1_cds_inh_total_weight * normalized);
							}
							delay.push_back(1);
						}
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
		source.reserve(pds_count * 8);
		target.reserve(pds_count * 8);
		type.reserve(pds_count * 8);
		weight.reserve(pds_count * 8);
		max_weight.reserve(pds_count * 8);
		delay.reserve(pds_count * 8);
		for (int pds = 0; pds < pds_count; ++pds) {
			int src_dir = pds / pds_pool_size;
			int src_local_index = pds % pds_pool_size;
			int src_block_x = src_local_index % pds_grid_width;
			int src_block_y = src_local_index / pds_grid_width;
			for (int lip = 0; lip < lip_pool_size; ++lip) {
				int rf_x = lip % pds_grid_width;
				int rf_y = lip / pds_grid_width;
				int center_y = rf_y * 2;
				if (std::abs(src_block_x - rf_x) > 1 || std::abs(src_block_y - center_y) > 1) {
					continue;
				}
				int lip_neuron = lip_base + src_dir * lip_pool_size + lip;
				source.push_back(pds_base + pds);
				target.push_back(lip_neuron);
				type.push_back(0);
				weight.push_back(pds_to_lip_weight);
				max_weight.push_back(pds_to_lip_weight);
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
				float delta = 2.0f * kPi * static_cast<float>(src_dir - dst_dir) / static_cast<float>(kDirs);
				float inh_strength = std::max(0.0f, -std::cos(delta));
				if (inh_strength <= 0.0f) {
					continue;
				}
				for (int src_idx = 0; src_idx < lip_pool_size; ++src_idx) {
					int src_neuron = lip_base + src_dir * lip_pool_size + src_idx;
					for (int dst_idx = 0; dst_idx < lip_pool_size; ++dst_idx) {
						int dst_neuron = lip_base + dst_dir * lip_pool_size + dst_idx;
						if (!SparseConnect(src_neuron, dst_neuron, 10)) {
							continue;
						}
						source.push_back(src_neuron);
						target.push_back(dst_neuron);
						type.push_back(1);
						weight.push_back(lip_inhibition_weight * inh_strength);
						max_weight.push_back(lip_inhibition_weight * inh_strength);
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
		source.reserve(cds_count * kDirs * 3);
		target.reserve(cds_count * kDirs * 3);
		type.reserve(cds_count * kDirs * 3);
		weight.reserve(cds_count * kDirs * 3);
		max_weight.reserve(cds_count * kDirs * 3);
		delay.reserve(cds_count * kDirs * 3);
		for (int cds = 0; cds < cds_count; ++cds) {
			int speed_index = cds / cds_plane_count;
			int cds_local_index = cds % cds_plane_count;
			int src_dir = cds_local_index % kDirs;
			int spatial_index = cds_local_index / kDirs;
			int src_x = spatial_index % width;
			int src_y = spatial_index / width;
			float speed_gain = speed_index == 1 ? 1.0f : 0.65f;
			float block_xf = ((static_cast<float>(src_x) + 0.5f) * static_cast<float>(pds_grid_width) / static_cast<float>(width)) - 0.5f;
			float block_yf = ((static_cast<float>(src_y) + 0.5f) * static_cast<float>(pds_grid_height) / static_cast<float>(height)) - 0.5f;
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
					if (block_x < 0 || block_x >= pds_grid_width || block_y < 0 || block_y >= pds_grid_height) {
						continue;
					}
					float delta_x = static_cast<float>(block_x) - block_xf;
					float delta_y = static_cast<float>(block_y) - block_yf;
					float spatial_gain = std::exp(-(delta_x * delta_x + delta_y * delta_y) / (2.0f * cds_pds_spatial_sigma * cds_pds_spatial_sigma));
					if (spatial_gain <= 0.0f) {
						continue;
					}
					block_candidates.push_back(BlockCandidate{ block_y * pds_grid_width + block_x, spatial_gain });
					block_gain_sum += spatial_gain;
				}
			}
			if (block_candidates.empty()) {
				continue;
			}
			for (int pds_dir = 0; pds_dir < kDirs; ++pds_dir) {
				float exc_similarity = DirectionSimilarity(src_dir, pds_dir, cds_pds_exc_sigma);
				for (const BlockCandidate& candidate : block_candidates) {
					int pds = pds_dir * pds_pool_size + candidate.local_index;
					float spatial_gain = candidate.gain / std::max(block_gain_sum, 1e-6f);

					source.push_back(cds_base + cds);
					target.push_back(pds_base + pds);
					type.push_back(0);
					weight.push_back(cds_pds_ampa_weight * exc_similarity * spatial_gain * speed_gain);
					max_weight.push_back(cds_pds_ampa_weight * exc_similarity * spatial_gain * speed_gain);
					delay.push_back(1);

					source.push_back(cds_base + cds);
					target.push_back(pds_base + pds);
					type.push_back(2);
					weight.push_back(cds_pds_nmda_weight * exc_similarity * spatial_gain * speed_gain);
					max_weight.push_back(cds_pds_nmda_weight * exc_similarity * spatial_gain * speed_gain);
					delay.push_back(2);
				}
			}
		}
		syn_description.push_back(MakeConnectionDescription(source, target, type, weight, max_weight, delay));
	}

	if (enable_pds_fs_path) {
		std::vector<int> source;
		std::vector<int> target;
		std::vector<int> type;
		std::vector<float> weight;
		std::vector<float> max_weight;
		std::vector<int> delay;
		source.reserve(cds_count * kDirs);
		target.reserve(cds_count * kDirs);
		type.reserve(cds_count * kDirs);
		weight.reserve(cds_count * kDirs);
		max_weight.reserve(cds_count * kDirs);
		delay.reserve(cds_count * kDirs);
		for (int cds = 0; cds < cds_count; ++cds) {
			int speed_index = cds / cds_plane_count;
			int cds_local_index = cds % cds_plane_count;
			int src_dir = cds_local_index % kDirs;
			int spatial_index = cds_local_index / kDirs;
			int src_x = spatial_index % width;
			int src_y = spatial_index / width;
			float speed_gain = speed_index == 1 ? 1.0f : 0.65f;
			int block_x = (src_x * pds_grid_width) / width;
			int block_y = (src_y * pds_grid_height) / height;
			int pds_local_index = block_y * pds_grid_width + block_x;
			for (int fs_dir = 0; fs_dir < kDirs; ++fs_dir) {
				int pds_fs = fs_dir * pds_pool_size + pds_local_index;
				float norm_similarity = DirectionSimilarity(src_dir, fs_dir, cds_pds_norm_sigma);
				float opp_similarity = DirectionSimilarity(src_dir, (fs_dir + 4) % kDirs, cds_pds_opp_sigma);
				float drive_gain = (cds_pds_norm_weight * (1.1f - norm_similarity) + cds_pds_opp_weight * opp_similarity) * speed_gain;
				if (drive_gain <= 0.0f) {
					continue;
				}
				source.push_back(cds_base + cds);
				target.push_back(pds_fs_base + pds_fs);
				type.push_back(0);
				weight.push_back(cds_fs_ampa_weight * drive_gain);
				max_weight.push_back(cds_fs_ampa_weight * drive_gain);
				delay.push_back(1);
			}
		}
		syn_description.push_back(MakeConnectionDescription(source, target, type, weight, max_weight, delay));
	}

	if (enable_pds_fs_path) {
		std::vector<int> source;
		std::vector<int> target;
		std::vector<int> type;
		std::vector<float> weight;
		std::vector<float> max_weight;
		std::vector<int> delay;
		source.reserve(pds_fs_count * kDirs);
		target.reserve(pds_fs_count * kDirs);
		type.reserve(pds_fs_count * kDirs);
		weight.reserve(pds_fs_count * kDirs);
		max_weight.reserve(pds_fs_count * kDirs);
		delay.reserve(pds_fs_count * kDirs);
		for (int pds_fs = 0; pds_fs < pds_fs_count; ++pds_fs) {
			int src_dir = pds_fs / pds_pool_size;
			int src_local_index = pds_fs % pds_pool_size;
			for (int dst_dir = 0; dst_dir < kDirs; ++dst_dir) {
				int pds = dst_dir * pds_pool_size + src_local_index;
				float norm_similarity = DirectionSimilarity(src_dir, dst_dir, pds_fs_to_pds_norm_sigma);
				float opp_similarity = DirectionSimilarity(src_dir, (dst_dir + 4) % kDirs, pds_fs_to_pds_opp_sigma);
				float inhibition = pds_fs_to_pds_norm_weight * (1.05f - norm_similarity) + pds_fs_to_pds_opp_weight * opp_similarity;
				if (inhibition <= 0.0f) {
					continue;
				}
				source.push_back(pds_fs_base + pds_fs);
				target.push_back(pds_base + pds);
				type.push_back(1);
				weight.push_back(inhibition);
				max_weight.push_back(inhibition);
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
		for (int src = 0; src < pds_count; ++src) {
			int src_dir = src / pds_pool_size;
			int src_local_index = src % pds_pool_size;
			for (int dst = 0; dst < pds_count; ++dst) {
				int dst_dir = dst / pds_pool_size;
				int dst_local_index = dst % pds_pool_size;
				if (src == dst || src_dir == dst_dir) {
					continue;
				}
				float inhibition_gain = src_local_index == dst_local_index ? 1.0f : pds_cross_block_inhibition_gain;
				source.push_back(pds_base + src);
				target.push_back(pds_base + dst);
				type.push_back(1);
				weight.push_back(0.35f * pds_lateral_inhibition_weight * inhibition_gain);
				max_weight.push_back(0.35f * pds_lateral_inhibition_weight * inhibition_gain);
				delay.push_back(1);
			}
		}
		syn_description.push_back(MakeConnectionDescription(source, target, type, weight, max_weight, delay));
	}

	Simulation* simulation = new Simulation(
		neu_description,
		syn_description,
		learn_description,
		simulation_steps,
		base_timestep,
		queue_count,
		EVENT_QUEUE_TIMING_WHEEL,
		64
	);

	std::vector<unsigned char> frame(width * height, 0);
	std::vector<float> v1_filters(v1_channels_per_speed, 0.0f);
	std::array<std::vector<float>, kSpeeds> v1_filters_per_speed;
	for (int speed_index = 0; speed_index < kSpeeds; ++speed_index) {
		v1_filters_per_speed[speed_index].assign(v1_channels_per_speed, 0.0f);
	}
	std::vector<int> spike_times;
	std::vector<int> spike_neurons;
	spike_times.reserve(frame_count * cds_count / 4);
	spike_neurons.reserve(frame_count * cds_count / 4);
	std::mt19937 poisson_rng(123456u);
	std::uniform_real_distribution<float> uniform01(0.0f, 1.0f);

	for (int frame_index = 0; frame_index < frame_count; ++frame_index) {
		if (stimulus_mode == "grating") {
			AddGratingFrame(frame, width, height, frame_index, 0.08f, grating_direction_deg, 2.2f);
		}
		else {
			AddPlaidFrame(frame, width, height, frame_index, 0.08f, plaid_direction_a_deg, plaid_direction_b_deg, 2.2f);
		}
		for (int speed_index = 0; speed_index < kSpeeds; ++speed_index) {
			motion_energy.calcV1filters(frame.data(), v1_filters.data(), false);
			v1_filters_per_speed[speed_index] = v1_filters;
		}

		int event_time = frame_index * frame_steps;
		for (int hold_step = 0; hold_step < frame_steps; ++hold_step) {
			int scheduled_time = event_time + hold_step;
			for (int speed_index = 0; speed_index < kSpeeds; ++speed_index) {
				for (int i = 0; i < v1_channels_per_speed; ++i) {
					float rate_hz = std::max(0.0f, v1_filters_per_speed[speed_index][i]) * v1_rate_scale;
					float spike_probability = std::min(rate_hz * 0.001f, 1.0f);
					if (spike_probability > 0.0f && uniform01(poisson_rng) < spike_probability) {
						spike_times.push_back(scheduled_time);
						spike_neurons.push_back(v1_base + speed_index * v1_channels_per_speed + i);
					}
				}
			}
		}
	}

	simulation->AddExternalSpikeActivity(spike_times, spike_neurons);
	simulation->AddFileOutputSpikeDriver("pattern_motion_network_spikes.txt");
	simulation->AddFileOutputMonitorDriver("pattern_motion_network_monitor.txt");
	simulation->SetPrintSimulationTime(false);
	simulation->InitSimulation();

	auto start = std::chrono::high_resolution_clock::now();
	simulation->RunSimulationStep(simulation_steps);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli> elapsed = end - start;

	std::cout << "Pattern motion network test finished in " << elapsed.count() << " ms." << std::endl;
	std::cout << "V1 channels per speed: " << v1_channels_per_speed << ", V1 input neurons total: " << v1_input_count << ", CDS neurons: " << cds_count << ", PDS neurons: " << pds_count << " (" << pds_grid_width << "x" << pds_grid_height << " blocks per direction), PDS FS neurons: " << pds_fs_count << ", LIP neurons: " << lip_count << std::endl;
	std::cout << "Stimulus mode: " << stimulus_mode << std::endl;
	std::cout << "Speed channels: " << kSpeeds << " (";
	for (int i = 0; i < kSpeeds; ++i) {
		std::cout << kSpeedValues[i];
		if (i + 1 < kSpeeds) {
			std::cout << ", ";
		}
	}
	std::cout << ")" << std::endl;

	delete simulation;

	std::vector<int> lip_counts = ReadLipPoolCounts("pattern_motion_network_spikes.txt", lip_base, lip_pool_size);
	std::vector<int> pds_counts = ReadDirectionPoolCounts("pattern_motion_network_spikes.txt", pds_base, pds_count, pds_pool_size);
	std::vector<int> cds_spatial_counts_all_speeds = ReadSpatialDirectionCounts("pattern_motion_network_spikes.txt", cds_base, cds_count, width * height);
	std::vector<int> pds_spatial_counts = ReadSpatialDirectionCounts("pattern_motion_network_spikes.txt", pds_base, pds_count, pds_pool_size);
	std::vector<int> lip_spatial_counts = ReadSpatialDirectionCounts("pattern_motion_network_spikes.txt", lip_base, lip_count, lip_pool_size);
	std::vector<int> cds_spatial_counts = CollapseSpeedPlanes(cds_spatial_counts_all_speeds, width * height, kSpeeds);
	int lip_winner_dir = SelectWinnerDirection(lip_counts);
	int pds_winner_dir = SelectWinnerDirection(pds_counts);

	const std::filesystem::path output_dir = std::filesystem::current_path() / "pattern_motion_visualizations";
	std::filesystem::create_directories(output_dir);
	std::ostringstream cds_map_name;
	cds_map_name << "cds_dir_maps_" << stimulus_mode;
	if (stimulus_mode == "grating") {
		cds_map_name << "_" << static_cast<int>(std::round(grating_direction_deg));
	}
	else {
		cds_map_name << "_" << static_cast<int>(std::round(plaid_direction_a_deg)) << "_" << static_cast<int>(std::round(plaid_direction_b_deg));
	}
	cds_map_name << ".bmp";
	const std::filesystem::path cds_map_path = output_dir / cds_map_name.str();
	if (SaveDirectionMontage(cds_spatial_counts, width, height, width * height, cds_map_path.string())) {
		std::cout << "Saved CDS direction heatmap to: " << cds_map_path.string() << std::endl;
	}
	std::ostringstream pds_map_name;
	pds_map_name << "pds_dir_maps_" << stimulus_mode;
	if (stimulus_mode == "grating") {
		pds_map_name << "_" << static_cast<int>(std::round(grating_direction_deg));
	}
	else {
		pds_map_name << "_" << static_cast<int>(std::round(plaid_direction_a_deg)) << "_" << static_cast<int>(std::round(plaid_direction_b_deg));
	}
	pds_map_name << ".bmp";
	const std::filesystem::path pds_map_path = output_dir / pds_map_name.str();
	if (SaveDirectionMontage(pds_spatial_counts, pds_grid_width, pds_grid_height, pds_pool_size, pds_map_path.string())) {
		std::cout << "Saved PDS direction heatmap to: " << pds_map_path.string() << std::endl;
	}
	std::ostringstream lip_map_name;
	lip_map_name << "lip_dir_maps_" << stimulus_mode;
	if (stimulus_mode == "grating") {
		lip_map_name << "_" << static_cast<int>(std::round(grating_direction_deg));
	}
	else {
		lip_map_name << "_" << static_cast<int>(std::round(plaid_direction_a_deg)) << "_" << static_cast<int>(std::round(plaid_direction_b_deg));
	}
	lip_map_name << ".bmp";
	const std::filesystem::path lip_map_path = output_dir / lip_map_name.str();
	if (SaveDirectionMontage(lip_spatial_counts, lip_grid_width, lip_grid_height, lip_pool_size, lip_map_path.string())) {
		std::cout << "Saved LIP direction heatmap to: " << lip_map_path.string() << std::endl;
	}

	if (pds_winner_dir >= 0) {
		std::cout << "PDS winner direction index: " << pds_winner_dir << std::endl;
		std::cout << "PDS winner direction degrees: " << DirectionIndexToDegrees(pds_winner_dir) << std::endl;
		std::cout << "PDS pool spike counts:";
		for (int dir = 0; dir < static_cast<int>(pds_counts.size()); ++dir) {
			std::cout << " [" << dir << ":" << DirectionIndexToDegrees(dir) << "deg=" << pds_counts[dir] << "]";
		}
		std::cout << std::endl;
	}
	if (lip_winner_dir >= 0) {
		std::cout << "LIP winner direction index: " << lip_winner_dir << std::endl;
		std::cout << "LIP winner direction degrees: " << DirectionIndexToDegrees(lip_winner_dir) << std::endl;
		std::cout << "LIP pool spike counts:";
		for (int dir = 0; dir < static_cast<int>(lip_counts.size()); ++dir) {
			std::cout << " [" << dir << ":" << DirectionIndexToDegrees(dir) << "deg=" << lip_counts[dir] << "]";
		}
		std::cout << std::endl;
	}
	return 0;
#endif
}
