/*
* 文件名：non_int.h
* 采用指数衰减模式的突触，该类型不需要通过微分方程更新
*/

#ifndef NONINT_H
#define NONINT_H
class Interconnections;

#include "../source_file_realtime_v1_async/SynModel/inc/Synmodel.h"

class Non_int : public Synmodel {
	public:


		/*
		* 构造函数
		*/
		Non_int(int timestep);

		/*
		* 鏋愭瀯鍑芥暟
		*/
		virtual ~Non_int();

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
		virtual float GetCurrent(float* NeuronState, int index) = 0;

		/*
	   * 初始化模型状态向量
	   */
		virtual void InitSynState(int NumberOfNeurons) = 0;

		/*
		* 比较函数，用于比较两个模型是否相同
		*/
		virtual bool compare(Synmodel* synmodel) {
			if (!Synmodel::compare(synmodel)) {
				return false;
			}
			Non_int* e = dynamic_cast<Non_int*>(synmodel);
			if (e == NULL) {
				return false;
			}
			return this->getTimestepSize() == e->getTimestepSize();

		}

		/*
		* 璁剧疆鍙傛暟
		*/
		virtual std::map<std::string, boost::any> getParameters();

		/*
		* 鑾峰彇鍙傛暟
		*/
		virtual void setParameters(std::map<std::string, boost::any> parameters);



};

#endif
