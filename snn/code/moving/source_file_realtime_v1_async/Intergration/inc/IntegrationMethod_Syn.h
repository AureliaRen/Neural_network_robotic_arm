/*
* IntegrationMethod_Syn.h
* 突触的积分方法
*/
#ifndef INTEGRATIONMETHOD_SYN_H
#define INTEGRATIONMETHOD_SYN_H


#include <iostream>
#include <boost/any.hpp>
#include <map>
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#define MAX_VARIABLES 19

class IntegerationMethod_Syn {
public:
	//积分的时间步长
	float dt;
	//累计积分变量
	float valid_Aux = 0;
	//积分方法名称
	std::string name;

	/*
	* 构造函数
	*/
	IntegerationMethod_Syn() {};

	/*
	* 析构函数
	*/
	~IntegerationMethod_Syn() {};


	virtual void CaculateIncreament() {};


	/*
	* 重置积分状态
	* index: 神经元状态下标
	*/
	virtual void ResetState(int index) {};


	/*
	* 获取积分时间步长
	*/
	float Gettimestep() {
		return this->dt;
	}

	/*
	* 设置积分时间步长
	*/
	virtual void Settimestep(float dt) {};

	/*
	* 增加累计积分变量
	*/
	inline void IncrementValidIntegrationVariable(float value) {
		this->valid_Aux += value;
	}

	/*
	* 获取累计积分变量
	*/
	inline float GetValidIntegrationVariable() {
		return this->valid_Aux;
	}

	/*
	* 获取积分方法参数
	*/
	virtual std::map<std::string, boost::any> getParameters() {
		// 返回参数映射
		std::map<std::string, boost::any> newMap;
		newMap["step"] = boost::any(this->dt);
		newMap["name"] = boost::any(this->name);
		return newMap;
	}

	/*
	* 设置积分方法参数
	*/
	virtual void setParameters(std::map<std::string, boost::any> parameters) {
		// 设置参数映射
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
	* 比较两个积分方法是否一致
	*/
	virtual bool compare(IntegerationMethod_Syn* method) {
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


#endif // !INTEGRATIONMETHOD_SYN_H
