/*
* 定义了一个积分方法的基类，用于实现各种积分方法
*/

#ifndef INTEGERATIONMETHOD_H
#define INTEGERATIONMETHOD_H

#include <iostream>
#include <boost/any.hpp>
#include <map>
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#define MAX_VARIABLES 19

class IntegerationMethod {
	public:
		//积分的时间步长
		float dt;
		//有效的积分项
		float valid_Aux = 0;
		//积分方法名称
		std::string name;

		/*
		* 构造函数
		*/
		IntegerationMethod() {};

		/*
		* 鏋愭瀯鍑芥暟
		*/
		~IntegerationMethod() {};

		/*
		* 计算积分的增量
		* index: 当前神经元索引
		*/
		virtual void CaculateIncreament(Simulation* simulation, int currenttime) = 0;


		/*
		* 初始化状态
		* NumberOfNeuron: 神经元数量
		* intit_vector: 初始状态向量
		*/
		virtual void InitState(int NumberOfNeuron, float* intit_vector) = 0;

		/*
		* 重置积分状态
		* index: 当前神经元索引
		*/
		virtual void ResetState(int index) = 0;


		/*
		* 获取积分步长
		*/
		float Gettimestep() {
			return this->dt;
		}

		/*
		* 设置积分步长
		*/
		virtual void Settimestep(float dt) {};

		/*
		* 积累有效积分项
		*/
		inline void IncrementValidIntegrationVariable(float value) {
			this->valid_Aux += value;
		}

		/*
		* 获取有效积分项
		*/
		inline float GetValidIntegrationVariable() {
			return this->valid_Aux;
		}

		/*
		* 获取积分的参数
		*/
		virtual std::map<std::string, boost::any> getParameters() {
			// 杩斿洖鍙傛暟
			std::map<std::string, boost::any> newMap;
			newMap["step"] = boost::any(this->dt);
			newMap["name"] = boost::any(this->name);
			return newMap;
		}

		/*
		* 设置积分的参数
		*/
		virtual void setParameters(std::map<std::string, boost::any> parameters) {
			// 璁剧疆鍙傛暟
			std::map<std::string, boost::any>::iterator it;
			it = parameters.find("name");
			if (it != parameters.end()) {
				std::string newParam = boost::any_cast<std::string>(it->second);
				this->name = newParam;
				parameters.erase(it);
			}

			it = parameters.find("step");
			if (it != parameters.end()) {
				float newParam = boost::any_cast<float>(it->second);
				this->dt = newParam;
				parameters.erase(it);
			}
		}

		/*
		* 瀵规瘮涓や釜绉垎妯″瀷鏄惁鐩稿悓
		*/
		virtual bool compare(IntegerationMethod* method) {
			if (this->name != method->name) {
				return false;
			}
			else if (this->dt != method->dt) {
				return false;
			}
			else {
				return true;
			}
		}


};
#endif // INTEGERATIONMETHOD_H