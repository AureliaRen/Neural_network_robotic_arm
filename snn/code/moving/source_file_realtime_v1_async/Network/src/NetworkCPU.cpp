#include "../source_file_realtime_v1_async/Network/inc/Network.h"
#include "../source_file_realtime_v1_async/ModelFactory/NeuronModelFactory.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/ModelFactory/LearningRuleModelFactory.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/LearningRule.h"
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"




Network::Network() :
	inters(0), intersNum(0), neurontypes(0), neurontypesNum(0), neurons(0), neuronsNum(0), TimeDrivenNeurons(0), TimeDrivenNeuronsNum(0), TimeDrivenNeuronsGPU(0), TimeDrivenNeuronsNumGPU(0), NumberOfQueue(0), LearningRuleNum(0), LearningRules(0), isMonitor(false), wordination(0), timesteps(1), basetemestepsize(0.1) {}

Network::Network(const std::list<NeuronLayerDescription>& neuron_layer_list, const std::list<ConnectionDescription>& connection_list, const std::list<LearningRuleDescription>& learning_rule_list, int NumberOfQueue, float basetimestepsize, Simulation* simulation) :
	inters(0), intersNum(0), neurontypes(0), neurontypesNum(0), neurons(0), neuronsNum(0), TimeDrivenNeurons(0), TimeDrivenNeuronsNum(0), TimeDrivenNeuronsGPU(0), TimeDrivenNeuronsNumGPU(0), NumberOfQueue(NumberOfQueue), LearningRuleNum(0), LearningRules(0), isMonitor(false), wordination(0), timesteps(1), basetemestepsize(basetimestepsize){
	this->CompileNetwork(neuron_layer_list, connection_list, learning_rule_list, simulation);
}

Network::~Network() {
	if (this->inters != 0) {
		//释放连接的内存
		delete[] this->inters;
	}
	if (this->neurontypes != 0) {
		for (int i = 0; i < this->neurontypesNum; i++) {
			if (this->neurontypes[i] != 0) {
				for (int j = 0; j < this->NumberOfQueue; j++) {
					if (this->neurontypes[i][j] != 0) {
						delete this->neurontypes[i][j];
					}
				}
				delete[] this->neurontypes[i];
			}
		}
		delete[] this->neurontypes;
	}
	if (this->neurons != 0) {
		delete[] this->neurons;
	}

	if (this->TimeDrivenNeurons != 0) {
		for (int i = 0; i < this->neurontypesNum; i++) {
			if (this->TimeDrivenNeurons[i] != 0) {
				for (int j = 0; j < this->NumberOfQueue; j++) {
					delete[] this->TimeDrivenNeurons[i][j];
				}
				delete[] this->TimeDrivenNeurons[i];
			}
		}
		delete[] this->TimeDrivenNeurons;
	}

	if (this->TimeDrivenNeuronsNum != 0) {
		for (int i = 0; i < this->neurontypesNum; i++) {
			if (this->TimeDrivenNeuronsNum[i] != 0) {
				delete[] this->TimeDrivenNeuronsNum[i];
			}
		}
		delete[] this->TimeDrivenNeuronsNum;
	}

	if (this->TimeDrivenNeuronsGPU != 0) {
		for (int i = 0; i < this->neurontypesNum; i++) {
			if (this->TimeDrivenNeuronsGPU[i] != 0) {
				for (int j = 0; j < this->NumberOfQueue; j++) {
					delete[] this->TimeDrivenNeuronsGPU[i][j];
				}
				delete[] this->TimeDrivenNeuronsGPU[i];
			}
		}
		delete [] this->TimeDrivenNeuronsGPU;
	}

	if (this->TimeDrivenNeuronsNumGPU != 0) {
		for (int i = 0; i < this->neurontypesNum; i++) {
			if (this->TimeDrivenNeuronsNumGPU[i] != 0) {
				delete[] this->TimeDrivenNeuronsNumGPU[i];
			}
		}
		delete[] this->TimeDrivenNeuronsNumGPU;
	}

	if (this->LearningRules != 0) {
		for (int i = 0; i < this->LearningRuleNum; i++) {
			delete this->LearningRules[i];
		}
		delete[] this->LearningRules;
	}

	if (this->wordination != 0) {
		delete[] this->wordination;
	}

}






void Network::CreateNeuronModel(const std::list<NeuronLayerDescription>& neuron_layer_list, int timesteps, float basetimestepsize, Simulation* simulation) {
	//定义一个常量迭代器用于遍历neuron_layer_list
	std::list<NeuronLayerDescription>::const_iterator it_NeuronLayer;
	//瀹氫箟浜嗕竴涓猅empModel锛岀敤浜庡瓨鍌ㄤ复鏃剁殑NeuronModel
	std::vector<NeuronModel*> TempModel;
	//瀹氫箟浜嗕竴涓父閲忚凯浠ｅ櫒鐢ㄤ簬閬嶅巻 TempModel
	std::vector<NeuronModel*>::const_iterator it_TempModel;
	//瀹氫箟浜嗕竴涓悜閲忓瓨鍌∟euronModel鐨処ndex
	std::vector<int> NeuronModelIndex;
	//棣栧厛鍦═emp_Model涓瓨鍌ㄦ墍鏈変笉鍚岀被鐨凬euronModel
	for (it_NeuronLayer = neuron_layer_list.begin(); it_NeuronLayer != neuron_layer_list.end(); it_NeuronLayer++) {
		bool found = false;
		// 创建一个临时的 NeuronModel 对象 temp_type
		NeuronModel* temp_type = NeuronModelFactory::createNeuronModel(it_NeuronLayer->ModelName, it_NeuronLayer->NeuronParameter, timesteps, basetimestepsize, 0);
		// 遍历 TempModel，检查是否已有重复的 NeuronModel
		int type_index = 0;
		for (it_TempModel = TempModel.begin(); it_TempModel != TempModel.end() && !found; it_TempModel++) {
			found = temp_type->compare(*it_TempModel);
			if (!found) {
				type_index++;
			}
		}
		//如果没找到，说明是新的NeuronModel，将其添加到TempModel中
		if (!found) {
			TempModel.push_back(temp_type);
		}
		else {
			delete temp_type;
			temp_type = NULL;
		}

		NeuronModelIndex.push_back(type_index);

	}
	//设置NeuronModel的个数
	this->neurontypesNum = TempModel.size();
	//涓簄eurontypes鍒嗛厤鍐呭瓨
	this->neurontypes = new NeuronModel * *[this->neurontypesNum];
	for (int i = 0; i < this->neurontypesNum; i++) {
		this->neurontypes[i] = new NeuronModel * [this->NumberOfQueue];
		for (int j = 0; j < this->NumberOfQueue; j++) {
			this->neurontypes[i][j] = NeuronModelFactory::createNeuronModel(TempModel[i]->getModelName(), TempModel[i]->getParameters(), timesteps, basetimestepsize, j);
		}
		delete TempModel[i];
		TempModel[i] = NULL;
	}
	//缁熻绁炵粡鍏冪殑鎬绘暟
	this->neuronsNum = 0;
	for (it_NeuronLayer = neuron_layer_list.begin(); it_NeuronLayer != neuron_layer_list.end(); it_NeuronLayer++) {
		this->neuronsNum += it_NeuronLayer->numberofneuron;
	}

	// 定义神经元模型的计数器
	int tind, nind;
	NeuronModel** temp_type;
	this->neurons = new Neuron[this->neuronsNum];

	//统计时间驱动神经元
	this->TimeDrivenNeuronsNum = new int* [this->neurontypesNum]();
	//记录所有时间驱动神经元的索引
	int*** time_driven_index = new int** [this->neurontypesNum];
	//统计GPU时间驱动神经元
	this->TimeDrivenNeuronsNumGPU = new int* [this->neurontypesNum]();
	// 记录所有 GPU 时间驱动神经元的索引
	int*** time_driven_indexGPU = new int** [this->neurontypesNum];
	//记录每个模型中神经元的个数
	int** N_neurons = new int* [this->neurontypesNum]();

	for (int i = 0; i < this->neurontypesNum; i++) {
		this->TimeDrivenNeuronsNum[i] = new int[this->NumberOfQueue]();
		this->TimeDrivenNeuronsNumGPU[i] = new int[this->NumberOfQueue]();
		time_driven_index[i] = new int* [this->NumberOfQueue];
		time_driven_indexGPU[i] = new int* [this->NumberOfQueue];
		for (int j = 0; j < this->NumberOfQueue; j++) {
			time_driven_index[i][j] = new int[this->neuronsNum]();
            time_driven_indexGPU[i][j] = new int[this->neuronsNum]();
		}
		N_neurons[i] = new int[this->NumberOfQueue]();
	}

	//涓簄eurons鍒嗛厤鍒板搴旂殑闃熷垪涓庣缁忓厓妯″瀷
	std::vector<int>::const_iterator it_NeuronTypeIndex;
	it_NeuronTypeIndex = NeuronModelIndex.begin();
	tind = 0;
	for (it_NeuronLayer = neuron_layer_list.begin();
		it_NeuronLayer != neuron_layer_list.end();
		++it_NeuronLayer) {

		temp_type = this->neurontypes[*it_NeuronTypeIndex];
		int blockSize = (it_NeuronLayer->numberofneuron + this->NumberOfQueue - 1) / this->NumberOfQueue;
		int blockIndex;
		for (nind = 0; nind < it_NeuronLayer->numberofneuron; nind++) {
			//为每个神经元分配相应的队列
			blockIndex = nind / blockSize;
			//为每个神经元分配相应的模型
			this->neurons[nind + tind].InitNeuron(nind + tind, temp_type[blockIndex], N_neurons[*it_NeuronTypeIndex][blockIndex], blockIndex, it_NeuronLayer->isMonitored, it_NeuronLayer->isOutput);
			//鏇存柊N_neurons璁℃暟
			N_neurons[*it_NeuronTypeIndex][blockIndex]++;

			//涓洪潪杈撳叆绁炵粡鍏冨垽鏂坊鍔犵洃瑙嗗櫒
			if (it_NeuronLayer->isMonitored && temp_type[0]->getNeuronModelType() == NEURAL_LAYER) {
				for (int n = 0; n < this->NumberOfQueue; n++) {
					temp_type[n]->StateVector->IsMonitored = true; //添加监视器
				}
				this->isMonitor = true;
			}

			if (temp_type[blockIndex]->TimeDriven && !temp_type[blockIndex]->IsGPU) {
				//璁板綍鏃堕棿椹卞姩绁炵粡鍏冪殑绱㈠紩
				time_driven_index[*it_NeuronTypeIndex][blockIndex][this->TimeDrivenNeuronsNum[*it_NeuronTypeIndex][blockIndex]] = nind + tind;
				//鏇存柊鏃堕棿椹卞姩绁炵粡鍏冪殑璁℃暟
				this->TimeDrivenNeuronsNum[*it_NeuronTypeIndex][blockIndex]++;
			}
			else if (temp_type[blockIndex]->TimeDriven && temp_type[blockIndex]->IsGPU) {
				//璁板綍GPU鏃堕棿椹卞姩绁炵粡鍏冪殑绱㈠紩
				time_driven_indexGPU[*it_NeuronTypeIndex][blockIndex][this->TimeDrivenNeuronsNumGPU[*it_NeuronTypeIndex][blockIndex]] = nind + tind;
				//鏇存柊GPU鏃堕棿椹卞姩绁炵粡鍏冪殑璁℃暟
				this->TimeDrivenNeuronsNumGPU[*it_NeuronTypeIndex][blockIndex]++;
			}
		}
		tind += it_NeuronLayer->numberofneuron;
		++it_NeuronTypeIndex;
	}
	//缁熻鏃堕棿椹卞姩绁炵粡鍏冪殑淇℃伅
	this->TimeDrivenNeurons = new Neuron ***[this->neurontypesNum]();
	for (int i = 0; i < this->neurontypesNum; i++) {
		bool hasTimeDrivenNeurons = false;
		for (int j = 0; j < this->NumberOfQueue; j++) {
			if (this->TimeDrivenNeuronsNum[i][j] > 0) {
				hasTimeDrivenNeurons = true;
				break;
			}
		}
		if (hasTimeDrivenNeurons) {
			this->TimeDrivenNeurons[i] = new Neuron * *[this->NumberOfQueue]();
			for (int j = 0; j < this->NumberOfQueue; j++) {
				this->TimeDrivenNeurons[i][j] = new Neuron * [this->TimeDrivenNeuronsNum[i][j]]();
				for (int k = 0; k < this->TimeDrivenNeuronsNum[i][j]; k++) {
					this->TimeDrivenNeurons[i][j][k] = &this->neurons[time_driven_index[i][j][k]];
				}
			}

		}
	}

	//缁熻GPU鏃堕棿椹卞姩绁炵粡鍏冪殑淇℃伅
	this->TimeDrivenNeuronsGPU = new Neuron ***[this->neurontypesNum]();
	for (int i = 0; i < this->neurontypesNum; i++) {
		bool hasGPUTimeDrivenNeurons = false;
		for (int j = 0; j < this->NumberOfQueue; j++) {
			if (this->TimeDrivenNeuronsNumGPU[i][j] > 0) {
				hasGPUTimeDrivenNeurons = true;
				break;
			}
		}
		if (hasGPUTimeDrivenNeurons) {
			this->TimeDrivenNeuronsGPU[i] = new Neuron * *[this->NumberOfQueue]();
			for (int j = 0; j < this->NumberOfQueue; j++) {
				this->TimeDrivenNeuronsGPU[i][j] = new Neuron * [this->TimeDrivenNeuronsNumGPU[i][j]]();
				for (int k = 0; k < this->TimeDrivenNeuronsNumGPU[i][j]; k++) {
					this->TimeDrivenNeuronsGPU[i][j][k] = &this->neurons[time_driven_indexGPU[i][j][k]];
				}
			}
		}
	}

	//初始化神经元状态向量
	this->InitializeStates(N_neurons);

	//閲婃斁鍐呭瓨
	for (int z = 0; z < neurontypesNum; z++) {
		for (int j = 0; j < this->NumberOfQueue; j++) {
			delete[] time_driven_index[z][j];
			delete[] time_driven_indexGPU[z][j];
		}
		delete[] time_driven_index[z];
		delete[] time_driven_indexGPU[z];
		delete[] N_neurons[z];
	}
	delete[] time_driven_index;
	delete[] time_driven_indexGPU;
	delete[] N_neurons;
}

void Network::InitializeStates(int** N_neurons) {
	for (int z = 0; z < this->neurontypesNum; z++) {
		for (int j = 0; j < this->NumberOfQueue; j++) {
			if (N_neurons[z][j] > 0) {
				this->neurontypes[z][j]->InitStateVector(N_neurons[z][j], j);
			}
			else {
				// 分配一个占位神经元
				this->neurontypes[z][j]->InitStateVector(1, 0);
			}
		}
	}
}


void Network::CreateConnections(const std::list<ConnectionDescription>& connection_list, std::vector<int>& N_connectionsPerRule) {
	//创建连接列表迭代器
	std::list<ConnectionDescription>::const_iterator it_ConnectionDescription;
	//初始化连接数量,统计连接数量
	this->intersNum = 0;
	for (it_ConnectionDescription = connection_list.begin(); it_ConnectionDescription != connection_list.end(); it_ConnectionDescription++) {
		this->intersNum += it_ConnectionDescription->SourceNeuron.size();
	}
	//定义connection_list总索引和局部索引
	int posc;
	posc = 0;
	this->inters = new Interconnections[this->intersNum];
	this->wordination = new Interconnections * [this->intersNum];
	//
	for (it_ConnectionDescription = connection_list.begin(); it_ConnectionDescription != connection_list.end(); it_ConnectionDescription++) {
		std::vector<int> delay(it_ConnectionDescription->SourceNeuron.size(), 1);
		std::vector<float> weight(it_ConnectionDescription->SourceNeuron.size(), 1.0);

		for (int id = 0; id < it_ConnectionDescription->SourceNeuron.size(); id++, posc++) {
			//设置连接的属性信息
			inters[posc].SetIndex(posc);
			inters[posc].SetDelay(it_ConnectionDescription->Delay[id]);
			inters[posc].SetSourceNeuron(&(this->neurons[it_ConnectionDescription->SourceNeuron[id]]));
			inters[posc].SetTargetNeuron(&(this->neurons[it_ConnectionDescription->TargetNeuron[id]]));
			inters[posc].SetWeight(it_ConnectionDescription->Weight[id]);
			inters[posc].SetType(it_ConnectionDescription->Type[id]);
			inters[posc].SetTargetNeuronModel(this->neurons[it_ConnectionDescription->TargetNeuron[id]].neuron_model);
			inters[posc].SetTargetNeuronModelIndex(this->neurons[it_ConnectionDescription->TargetNeuron[id]].index_in_NeuronModel);
			inters[posc].maximum_weight = it_ConnectionDescription->MaxWeight[id];
			inters[posc].synmodel = it_ConnectionDescription->SynapseModel[id];
			inters[posc].TargetNeuronModel->CheckType(&inters[posc]);
			inters[posc].LearningRule_withPost = NULL;
			inters[posc].LearningRule_withTrigger = NULL;
			inters[posc].LearningRule_withPostAndTrigger = NULL;
			//鑾峰彇瀛︿範瑙勫垯ID
			int learningRuleID = it_ConnectionDescription->SynapseRule[id];
			//如果采用可塑性连接
			if (learningRuleID >= 0) {
				//如果可塑性规则考虑突触后活动
				if (this->LearningRules[learningRuleID]->ImplementPostSynaptic()) {
					if (this->LearningRules[learningRuleID]->ImplementTriggerSynaptic()) {
						this->inters[posc].LearningRule_withPostAndTrigger = this->LearningRules[learningRuleID];
					}
					else {
						this->inters[posc].LearningRule_withPost = this->LearningRules[learningRuleID];
					}
				}
				else {
					this->inters[posc].LearningRule_withTrigger = this->LearningRules[learningRuleID];
				}
				N_connectionsPerRule[learningRuleID]++;
			}

			int triggerlearningRuleID = it_ConnectionDescription->TriggerRule[id];
			if (triggerlearningRuleID >= 0) {
				if (this->LearningRules[triggerlearningRuleID]->ImplementTriggerSynaptic()) {
					if (this->LearningRules[triggerlearningRuleID]->ImplementPostSynaptic() == false) {
						this->inters[posc].LearningRule_withTrigger = this->LearningRules[triggerlearningRuleID];
					}
					else {
						this->inters[posc].LearningRule_withPostAndTrigger = this->LearningRules[triggerlearningRuleID];
					}
					this->inters[posc].TriggerLearning = true;
				}
				N_connectionsPerRule[triggerlearningRuleID]++;
			}
		}
	}
}
//该函数用于比较两个连接的优先级
int qsort_connections(const void* a, const void* b) {
	int ord;
	float ord_double;
	//先比较队列的优先级
	ord = ((Interconnections*)a)->TargetNeuron->Queue_index - ((Interconnections*)b)->TargetNeuron->Queue_index;
	//如果同队列
	if (!ord) {
		//姣旇緝婧愮缁忓厓绱㈠紩
		ord = ((Interconnections*)a)->SourceNeuron->Neuron_index - ((Interconnections*)b)->SourceNeuron->Neuron_index;
		if (!ord) {
			//姣旇緝寤惰繜澶у皬
			ord_double = ((Interconnections*)a)->delay - ((Interconnections*)b)->delay;
			if (ord_double < 0.0) {
				ord = -1;
			}
			else if (ord_double > 0.0) {
				ord = 1;
			}
			else if (ord_double == 0) {
				//姣旇緝鐩爣绱㈠紩
				ord = ((Interconnections*)a)->TargetNeuron->Neuron_index - ((Interconnections*)b)->TargetNeuron->Neuron_index;
			}
		}
	}
	return ord;
}

void Network::CaculateOutputConnection() {
	//对连接进行重新排序
	qsort(this->inters, this->intersNum, sizeof(Interconnections), qsort_connections);
	if (this->intersNum > 0) {
		//缁熻绗琲涓缁忓厓鍒扮j涓槦鍒楃殑杩炴帴鏁伴噺
		int** NumberOfOutputs = new int* [this->neuronsNum];
		//第i个神经元到第j个队列的连接数量计数器
		int** OutputsLeft = new int* [this->neuronsNum];
		for (int i = 0; i < this->neuronsNum; i++) {
			NumberOfOutputs[i] = new int[this->NumberOfQueue]();
			OutputsLeft[i] = new int[this->NumberOfQueue];
		}

		for (int conn = 0; conn < this->intersNum; conn++) {
			//缁熻NumberOfOutputs
			NumberOfOutputs[this->inters[conn].SourceNeuron->Neuron_index][this->inters[conn].TargetNeuron->Queue_index]++;
		}

		for (int neu = 0; neu < this->neuronsNum; neu++) {
			//为OutpusLeft赋初值
			for (int i = 0; i < this->NumberOfQueue; i++) {
				OutputsLeft[neu][i] = NumberOfOutputs[neu][i];
			}
		}
		//涓篛utputconnections鍒嗛厤鍐呭瓨
		//Outputconnections[i][j][k]表示第i个神经元到第j个队列的第k个连接
		Interconnections**** Outputconnections = new Interconnections * **[this->neuronsNum];
		for (int i = 0; i < this->neuronsNum; i++) {
			Outputconnections[i] = new Interconnections * *[this->NumberOfQueue];
		}
		for (int neu = 0; neu < this->neuronsNum; neu++) {
			for (int i = 0; i < this->NumberOfQueue; i++) {
				if (NumberOfOutputs[neu][i] > 0) {
					Outputconnections[neu][i] = new Interconnections * [NumberOfOutputs[neu][i]];
				}
				else {
					Outputconnections[neu][i] = 0;
				}
			}
		}

		for (int con = this->intersNum - 1; con >= 0; --con) {
			int SourceCell = this->inters[con].SourceNeuron->Neuron_index;
			int OpenMP_index = this->inters[con].TargetNeuron->Queue_index;
			Outputconnections[SourceCell][OpenMP_index][--OutputsLeft[SourceCell][OpenMP_index]] = this->inters + con;
		}

		for (int neu = 0; neu < this->neuronsNum; neu++) {
			this->neurons[neu].Output_Synaps = Outputconnections[neu];
			this->neurons[neu].Output_Synaps_Number = NumberOfOutputs[neu];
		}

		delete[] Outputconnections;
		delete[] NumberOfOutputs;
		for (int i = 0; i < this->neuronsNum; i++) {
			delete[] OutputsLeft[i];
		}
		delete[] OutputsLeft;

	}

}

void Network::setWeightOrdination() {
	if (this->intersNum > 0) {
		for (int ninter = 0; ninter < this->intersNum; ninter++) {
			int index = this->inters[ninter].Index;
			this->wordination[index] = &(this->inters[ninter]);
		}
	}
}

void Network::SnapshotInitialWeights() {
	this->initial_weights.resize(this->intersNum);
	for (int i = 0; i < this->intersNum; ++i) {
		this->initial_weights[static_cast<std::size_t>(i)] = this->inters[i].weight;
	}
}

void Network::RestoreInitialWeights() {
	if (static_cast<int>(this->initial_weights.size()) != this->intersNum) {
		return;
	}
	for (int i = 0; i < this->intersNum; ++i) {
		this->inters[i].weight = this->initial_weights[static_cast<std::size_t>(i)];
	}
}

void Network::ResetDynamicState(bool preserve_weights) {
	for (int z = 0; z < this->neurontypesNum; ++z) {
		for (int q = 0; q < this->NumberOfQueue; ++q) {
			if (this->neurontypes[z][q] != 0 && this->neurontypes[z][q]->StateVector != 0) {
				this->neurontypes[z][q]->StateVector->ResetAllNeuronStates();
			}
		}
	}

	if (!preserve_weights) {
		this->RestoreInitialWeights();
	}
}

void Network::CaculateInputConnection() {
	if (this->intersNum > 0) {
		//鍒嗛厤鍐呭瓨
		int** NumberOfInputsWithPostLearning = new int* [this->neuronsNum]();
		int** InputLeftWithPostLearning = new int* [this->neuronsNum]();

		int** NumberOfInputsWithTriggerLearning = new int* [this->neuronsNum]();
		int** InputLeftWithTriggerLearning = new int* [this->neuronsNum]();

		int** NumberOfInputsWithPostAndTriggerLearning = new int* [this->neuronsNum]();
		int** InputLeftWithPostAndTriggerLearning = new int* [this->neuronsNum]();

		for (int i = 0; i < this->neuronsNum; i++) {
			NumberOfInputsWithPostLearning[i] = new int[this->LearningRuleNum]();
			NumberOfInputsWithTriggerLearning[i] = new int[this->LearningRuleNum]();
			NumberOfInputsWithPostAndTriggerLearning[i] = new int[this->LearningRuleNum]();
			InputLeftWithPostLearning[i] = new int[this->LearningRuleNum]();
			InputLeftWithTriggerLearning[i] = new int[this->LearningRuleNum]();
			InputLeftWithPostAndTriggerLearning[i] = new int[this->LearningRuleNum]();
		}

		//鎸夎繛鎺ュ惊鐜洿鏂拌鏁板櫒
		for (int i = 0; i < this->intersNum; i++) {
			if (this->inters[i].LearningRule_withPost != 0) {
				//获取学习规则模型的索引
				int IndexOfLearningRule = this->inters[i].LearningRule_withPost->LearningRuleID;
				NumberOfInputsWithPostLearning[this->inters[i].TargetNeuron->Neuron_index][IndexOfLearningRule]++;
			}
			else if (this->inters[i].LearningRule_withTrigger != 0) {
				int IndexOfLearningRule = this->inters[i].LearningRule_withTrigger->LearningRuleID;
				NumberOfInputsWithTriggerLearning[this->inters[i].TargetNeuron->Neuron_index][IndexOfLearningRule]++;
			}
			else if (this->inters[i].LearningRule_withPostAndTrigger != 0) {
				int IndexOfLearningRule = this->inters[i].LearningRule_withPostAndTrigger->LearningRuleID;
				NumberOfInputsWithPostAndTriggerLearning[this->inters[i].TargetNeuron->Neuron_index][IndexOfLearningRule]++;
			}
		}

		//为InputLeft赋初值
		for (int i = 0; i < this->neuronsNum; i++) {
			for (int j = 0; j < this->LearningRuleNum; j++) {
				InputLeftWithPostLearning[i][j] = NumberOfInputsWithPostLearning[i][j];
				InputLeftWithTriggerLearning[i][j] = NumberOfInputsWithTriggerLearning[i][j];
				InputLeftWithPostAndTriggerLearning[i][j] = NumberOfInputsWithPostAndTriggerLearning[i][j];
			}
		}
		//为应用不同可塑性规则的连接分配内存
		Interconnections**** ConnectionsWithPostLearning = new Interconnections * **[this->neuronsNum]();
		Interconnections**** ConnectionsWithTriggerLearning = new Interconnections * **[this->neuronsNum]();
		Interconnections**** ConnectionsWithPostAndTriggerLearning = new Interconnections * **[this->neuronsNum]();

		for (int i = 0; i < this->neuronsNum; i++) {
			ConnectionsWithPostLearning[i] = new Interconnections * *[this->LearningRuleNum];
			ConnectionsWithTriggerLearning[i] = new Interconnections * *[this->LearningRuleNum];
			ConnectionsWithPostAndTriggerLearning[i] = new Interconnections * *[this->LearningRuleNum];
			//统计不同类别学习规则的连接
			for (int j = 0; j < this->LearningRuleNum; j++) {
				ConnectionsWithPostLearning[i][j] = 0;
				if (NumberOfInputsWithPostLearning[i][j] > 0) {
					ConnectionsWithPostLearning[i][j] = new Interconnections * [NumberOfInputsWithPostLearning[i][j]];
				}
				ConnectionsWithTriggerLearning[i][j] = 0;
				if (NumberOfInputsWithTriggerLearning[i][j] > 0) {
					ConnectionsWithTriggerLearning[i][j] = new Interconnections * [NumberOfInputsWithTriggerLearning[i][j]];
				}
				ConnectionsWithPostAndTriggerLearning[i][j] = 0;
				if (NumberOfInputsWithPostAndTriggerLearning[i][j] > 0) {
					ConnectionsWithPostAndTriggerLearning[i][j] = new Interconnections * [NumberOfInputsWithPostAndTriggerLearning[i][j]];
				}
			}
		}

		//按连接循环倒序为三个Connections指针数组赋值赋值
		for (int i = this->intersNum - 1; i >= 0; i--) {
			// 如果这是一个后学习规则
			if (this->inters[i].LearningRule_withPost != 0) {
				ConnectionsWithPostLearning[this->inters[i].TargetNeuron->Neuron_index][this->inters[i].LearningRule_withPost->LearningRuleID][--InputLeftWithPostLearning[this->inters[i].TargetNeuron->Neuron_index][this->inters[i].LearningRule_withPost->LearningRuleID]] = this->inters + i;
			}
			if (this->inters[i].LearningRule_withTrigger != 0) {
				ConnectionsWithTriggerLearning[this->inters[i].TargetNeuron->Neuron_index][this->inters[i].LearningRule_withTrigger->LearningRuleID][--InputLeftWithTriggerLearning[this->inters[i].TargetNeuron->Neuron_index][this->inters[i].LearningRule_withTrigger->LearningRuleID]] = this->inters + i;
			}
			if (this->inters[i].LearningRule_withPostAndTrigger != 0) {
				ConnectionsWithPostAndTriggerLearning[this->inters[i].TargetNeuron->Neuron_index][this->inters[i].LearningRule_withPostAndTrigger->LearningRuleID][--InputLeftWithPostAndTriggerLearning[this->inters[i].TargetNeuron->Neuron_index][this->inters[i].LearningRule_withPostAndTrigger->LearningRuleID]] = this->inters + i;
			}

		}

		for (int neu = 0; neu < this->neuronsNum; neu++) {
			//涓烘瘡涓缁忓厓璁剧疆瀛︿範瑙勫垯鍒楄〃
			this->neurons[neu].SetPostSynapticLearningRule(ConnectionsWithPostLearning[neu], NumberOfInputsWithPostLearning[neu], this->LearningRuleNum);
			this->neurons[neu].SetTriggerSynapticLearningRule(ConnectionsWithTriggerLearning[neu], NumberOfInputsWithTriggerLearning[neu], this->LearningRuleNum);
			this->neurons[neu].SetTriggerAndPostSynapticLearningRule(ConnectionsWithPostAndTriggerLearning[neu], NumberOfInputsWithPostAndTriggerLearning[neu], this->LearningRuleNum);

			//灏嗚繛鎺ュ垎閰嶇粰涓嶅悓鐨凷tate
			for (int i = 0; i < this->LearningRuleNum; i++) {
				for (int j = 0; j < NumberOfInputsWithPostLearning[neu][i]; j++) {
					//鍒嗛厤StateIndex
					ConnectionsWithPostLearning[neu][i][j]->LearningRuleIndex_withPost = ConnectionsWithPostLearning[neu][i][j]->LearningRule_withPost->StateCounter;
					//更新StateIndex计数器
					ConnectionsWithPostLearning[neu][i][j]->LearningRule_withPost->StateCounter++;

				}

				for (int j = 0; j < NumberOfInputsWithTriggerLearning[neu][i]; j++) {
					ConnectionsWithTriggerLearning[neu][i][j]->LearningRuleIndex_withTrigger = ConnectionsWithTriggerLearning[neu][i][j]->LearningRule_withTrigger->StateCounter;
					ConnectionsWithTriggerLearning[neu][i][j]->LearningRule_withTrigger->StateCounter++;
				}

				for (int j = 0; j < NumberOfInputsWithPostAndTriggerLearning[neu][i]; j++) {
					ConnectionsWithPostAndTriggerLearning[neu][i][j]->LearningRuleIndex_withPostAndTrigger = ConnectionsWithPostAndTriggerLearning[neu][i][j]->LearningRule_withPostAndTrigger->StateCounter;
					ConnectionsWithPostAndTriggerLearning[neu][i][j]->LearningRule_withPostAndTrigger->StateCounter++;
				}

			}


			this->neurons[neu].InitLearningConnections();
		}

		for (int i = 0; i < this->neuronsNum; i++) {
			delete[] InputLeftWithPostAndTriggerLearning[i];
			delete[] InputLeftWithTriggerLearning[i];
			delete[] InputLeftWithPostLearning[i];
		}

		delete[] ConnectionsWithPostAndTriggerLearning;
		delete[] ConnectionsWithTriggerLearning;
		delete[] ConnectionsWithPostLearning;
		delete[] NumberOfInputsWithPostAndTriggerLearning;
		delete[] NumberOfInputsWithTriggerLearning;
		delete[] NumberOfInputsWithPostLearning;
		delete[] InputLeftWithPostAndTriggerLearning;
		delete[] InputLeftWithTriggerLearning;
		delete[] InputLeftWithPostLearning;

	}
}

void Network::CompileNetwork(const std::list<NeuronLayerDescription>& neuron_layer_list, const std::list<ConnectionDescription>& connection_list, const std::list<LearningRuleDescription>& learning_rule_list, Simulation* simulation) {
	//创建神经元模型
	this->CreateNeuronModel(neuron_layer_list, this->timesteps, this->basetemestepsize, simulation);
	//建立可塑性规则模型
	std::vector<int> N_ConnectionPerRule = this->CreateWeightChange(learning_rule_list);
	//鍒涘缓杩炴帴
	this->CreateConnections(connection_list, N_ConnectionPerRule);
	//初始化可塑性状态
	this->InitializeSynapticPlasticityState(N_ConnectionPerRule);
	//璁＄畻杈撳嚭杩炴帴
	this->CaculateOutputConnection();
	//计算排序前后的映射关系
	this->setWeightOrdination();
	this->SnapshotInitialWeights();
	//初始化可塑性状态
	this->CaculateInputConnection();
	//璁＄畻绁炵粡鍏冧笌绁炵粡鍏冩ā鍨嬬殑杈撳嚭寤惰繜缁撴瀯
	for (int i = 0; i < this->neuronsNum; i++) {
		this->neurons[i].CaculateOutputDelayStructure();
	}
	//璁＄畻绁炵粡鍏冩ā鍨嬩笌绁炵粡鍏冩ā鍨嬬殑杩炴帴寤惰繜缁撴瀯
	for (int i = 0; i < this->neuronsNum; i++) {
		this->neurons[i].PropogationStructure->CalculateSynapseDelayIndex(this->neurons[i].neuron_model->PropogationStructure);
	}
	//初始化电流输入连接
	for (int z = 0; z < this->neurontypesNum; z++) {
		for (int j = 0; j < this->NumberOfQueue; j++) {
			this->neurontypes[z][j]->InitializeInputCurrentSynapseStructure();
		}
	}


}

std::vector<int> Network::CreateWeightChange(const std::list<LearningRuleDescription>& learning_rule_list) {
	std::list<LearningRuleDescription>::const_iterator learning_rule_it;
	this->LearningRuleNum = learning_rule_list.size();
	//鍒濆鍖栧涔犺鍒欒鏁板櫒
	std::vector<int> N_ConnectionPerRule = std::vector<int>(this->LearningRuleNum, 0);

	int weight_change_index = 0;
	this->LearningRules = new LearningRule * [this->LearningRuleNum];
	for (learning_rule_it = learning_rule_list.begin(); weight_change_index < this->LearningRuleNum; ++learning_rule_it) {
		//閫愪竴鍒涘缓瀛︿範瑙勫垯妯″瀷
		this->LearningRules[weight_change_index] = LearningRuleModelFactory::createLearningRuleModel(*learning_rule_it);
		this->LearningRules[weight_change_index]->LearningRuleID = weight_change_index;
		++weight_change_index;
	}
	return N_ConnectionPerRule;
}

void Network::InitializeSynapticPlasticityState(const std::vector<int>& N_ConnectionPerRule) {
	for (int i = 0; i < this->LearningRuleNum; i++) {
		if (N_ConnectionPerRule[i] > 0) {
			this->LearningRules[i]->InitState(N_ConnectionPerRule[i], this->neuronsNum, this->basetemestepsize);

		}
	}
}

int Network::GetMinInterpropagationTime() {
	//赋大值以初始化最小值
	int time = 1000000;
	//查找最小值
	for (int i = 0; i < this->intersNum; i++) {
		if (this->inters[i].delay < time && this->inters[i].SourceNeuron->Queue_index != this->inters[i].TargetNeuron->Queue_index) {
			time = this->inters[i].delay;
		}
	}

	if (time == 1000000) {
		time = -1; //说明不存在跨线程连接
	}
	return time;
}

void Network::SaveWeightsToFile(const char* filename) {
	//瀹氫箟鏂囦欢鎸囬拡
	FILE* fp;
	//瀹氫箟杩炴帴id
	int interid;
	float preweight = 0.0;
	float flagweight = 0.0;
	int nsameweight = 0;
	//鍙湪鏉冮噸鍑虹幇鍙樺寲鏃舵墠淇濆瓨(娓哥▼缂栫爜)
	//鎵撳紑鏂囦欢
	fp = fopen(filename, "wt");
	if (fp) {
		for (interid = 0; interid <= this->intersNum; interid++) {
			if (interid < this->intersNum) {
				flagweight = this->wordination[interid]->weight;
			}

			if (preweight != flagweight || interid == this->intersNum) {
				if (nsameweight > 0) {
					fprintf(fp, "%i %f\n", nsameweight, preweight);
				}
				preweight = flagweight;
				nsameweight = 1;
			}
			else {
				nsameweight++;
			}
		}
		fclose(fp);
	}
}


void Network::LoadWeightsFromFile(const char* filename) {

	FILE* fp;
	int interid = 0; // 用于追踪当前的连接索引
	int count = 0;   // 读取到的重复次数
	float weight = 0.0; // 读取到的权重值

	fp = fopen(filename, "rt");
	if (fp == NULL) {
		printf("Error: Unable to open file %s\n", filename);
		return;
	}

	while (fscanf(fp, "%i %f", &count, &weight) == 2) {
		// 检查是否越界
		if (interid + count > this->intersNum) {
            printf("Error: Too many weights in file %s\n", filename);
		}
		//赋值权重
		for (int i = 0; i < count; i++) {
			if (interid < this->intersNum) {
				this->wordination[interid]->weight = weight;
				interid++;
			}
		}
	}


	fclose(fp);

}


