/*
* 文件名: SynapseState.h
* 定义了突触状态类，用于存储突触的动态变化
*/
#ifndef SYNAPSESTATE_H
#define SYNAPSESTATE_H

class SynapseState {
	public:

		float* StateValue; //突触状态值向量

		int* LastUpdate; //上一次更新突触状态的时间

		int NumberOfConnections; //使用该可塑性突触连接的数量

		int NumberOfState; //每个突触状态变量的个数

		/*
		* 构造函数
		* NumberOfConnections: 使用该可塑性突触连接的数量
		* NumberOfState: 每个突触状态变量的个数
		*/
		SynapseState(int NumberOfConnections, int NumberOfState);

		/*
		* 析构函数
		*/
		virtual ~SynapseState();

		/*
		* 纯虚函数，设置更新时间
		* ConnectionIndex: 连接索引
		* Time: 更新时间
		* Basetimestep: 基本时间步长
		*/
		virtual void SetUpdate(int ConnectionIndex, int Time, float Basetimestep) = 0;

		/*
		* 内联函数，设置状态
		* ConnectionIndex: 连接索引
		* Position: 状态位置
		*/
		inline void SetStateValue(int ConnectionIndex, int Position, float Value) {
			this->StateValue[ConnectionIndex * this->NumberOfState + Position] = Value;
		}

		/*
		* 内联函数，状态衰减
		*/
		inline void StateDecay(int ConnectionIndex, int Position, float DecayRate) {
			this->StateValue[ConnectionIndex * this->NumberOfState + Position] *= DecayRate;
		}

		/*
		* 内联函数，状态跳变
		*/
		inline void StateJump(int ConnectionIndex, int Position, float JumpValue) {
			this->StateValue[ConnectionIndex * this->NumberOfState + Position] += JumpValue;
		}

		/*
		* 内联函数，获取状态
		*/
		inline float GetStateValue(int ConnectionIndex, int Position) {
			return this->StateValue[ConnectionIndex * this->NumberOfState + Position];
		}

		/*
		* 纯虚函数，应用突触前脉冲
		*/
		virtual void ApplyPresynapticSpike(int index) = 0;

		/*
		* 纯虚函数，应用突触后脉冲
		*/
		virtual void ApplyPostsynapticSpike(int index) = 0;

};

#endif
