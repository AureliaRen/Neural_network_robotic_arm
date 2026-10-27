/*
* 文件名：Synmodel.h
* 定义了突触模型的基类
*/

#ifndef SYNMODEL_H
#define SYNMODEL_H

class Interconnections;
#include <iostream>
#include <map>
#include "boost/any.hpp"


class Synmodel {
	private:
		std::string model_name; //模型的名称

		int timestep_size; //基准时间步长的倍数

	public:


        //整形向量，用于存储突触跳变变量的局部索引
        int* synapsejumpindex;


       /*
       * 有参构造函数
       */
       Synmodel(int timestep_size);

       /*
       * 析构函数,用到了多态，所以需要声明为虚函数
       */
       virtual ~Synmodel();

       /*
        * 意义了一个比较函数，用于比较两个神经元模型是否相同
        */
       virtual bool compare(Synmodel* synmodel) {
           return this->model_name == synmodel->getModelName();
       }

       /*
        * 处理输入
        */
       virtual void ProcessInput(Interconnections* inter, float* NeuronState, float weight) = 0;

       /*
       * 更新突触
       */
       virtual void UpdateSynState(float dt) = 0;

       /*
       * 获取电流
       */
       virtual float GetCurrent(float* NeuronState, int i) = 0;

       /*
       * 初始化模型状态向量
       */
       virtual void InitSynState(int NumberOfNeurons) = 0;

       /*
        * 获取时间步大小
        */
       int getTimestepSize();

       /*
       * 设置时间步大小
       */
       void setTimestepSize(int timestepsize);

       /*
        * 设置模型名称
        * name:模型名称
        */
       void setModelName(std::string name);

       /*
       * 获取模型名称
       */
       std::string getModelName();

       /*
        * 纯虚函数，获取神经元模型的参数
        */
       virtual std::map<std::string, boost::any> getParameters();

       /*
       * 纯虚函数，设置神经元模型的参数
       */
       virtual void setParameters(std::map<std::string, boost::any> parameters);




};


#endif
