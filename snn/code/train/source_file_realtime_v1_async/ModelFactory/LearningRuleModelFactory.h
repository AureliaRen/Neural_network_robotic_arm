/*
* 文件名：LearningRuleModelFactory.h
* 定义了学习规则工厂类，用于创建学习规则模型
*/

#ifndef LEARNINGRULEMODELFACTORY_H
#define LEARNINGRULEMODELFACTORY_H

#include <iostream>
#include "../source_file_realtime_v1_async/LearningRule/inc/LearningRule.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/R_STDP.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/STDP.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/DASTDP.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/CerebullarLearningRule.h"

class LearningRuleModelFactory {
	public:
		static LearningRule* createLearningRuleModel(LearningRuleDescription lrDescription) {
			if (lrDescription.RuleName == "STDP") {
				LearningRule* Rule_Model = new STDP(lrDescription.RuleParameter);
				return Rule_Model;
			}
			else if (lrDescription.RuleName == "R_STDP" || lrDescription.RuleName == "R-STDP") {
				LearningRule* Rule_Model = new R_STDP(lrDescription.RuleParameter);
				return Rule_Model;
			}
			else if (lrDescription.RuleName == "DA-STDP" || lrDescription.RuleName == "DASTDP") {
				LearningRule* Rule_Model = new DASTDP(lrDescription.RuleParameter);
				return Rule_Model;
			}
			else if (lrDescription.RuleName == "CerebullarLearningRule") {
				LearningRule* Rule_Model = new CerebullarLearningRule(lrDescription.RuleParameter);
				return Rule_Model;
			}
			else {
				std::cout << "Unknown learning rule type: " << lrDescription.RuleName << std::endl;
				return 0;
			}
		}
};



#endif // LEARNINGRULEMODELFACTORY_H
