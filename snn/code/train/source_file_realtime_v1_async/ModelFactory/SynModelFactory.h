/*
* SynModelFactory.h
* 定义了一个工厂类，用于创建自定义突触对象
*/

#ifndef SYNMODELFACTORY_H
#define SYNMODELFACTORY_H
#include <iostream>
#include <string>
#include "../source_file_realtime_v1_async/SynModel/inc/Non_int/Voltage_jump.h"
#include "../source_file_realtime_v1_async/SynModel/inc/non_int/Exp_decay.h"
#include "../source_file_realtime_v1_async/SynModel/inc/int/Alpha.h"
#include "../source_file_realtime_v1_async/SynModel/inc/int/PaperPSP.h"
#include "../source_file_realtime_v1_async/SynModel/inc/non_int/PaperPSPHistory.h"

class SynModelFactory {
	public:
		static Synmodel* createSynModel(const std::string& type, std::map<std::string, boost::any> params, int timestepzise, float dt) {
			if (type == "Voltage_jump") {
				Voltage_jump* model = new Voltage_jump(timestepzise);
				model->setParameters(params);
				return model;
			}
			else if (type == "Exp_decay") {
				Exp_decay* model = new Exp_decay(timestepzise);
				model->setParameters(params);
				return model;
			}
			else if (type == "Alpha") {
                Alpha* model = new Alpha(timestepzise);
				model->setParameters(params);
				return model;
			}
			else if (type == "PaperPSPHistory") {
                auto* model = new PaperPSPHistory(timestepzise);
                model->setParameters(params); return model;
            }
			else if (type == "PaperPSP") {
				PaperPSP* model = new PaperPSP(timestepzise);
				model->setParameters(params);
				return model;
			}
			else {
				std::cout << "Unknown Synmodel type: " << type << std::endl;
				return 0;
			}

		}


};
#endif // SYNMODELFACTORY_H
