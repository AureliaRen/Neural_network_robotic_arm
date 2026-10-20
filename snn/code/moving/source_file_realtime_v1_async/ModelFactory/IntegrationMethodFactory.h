/*
* 文件名：IntegrationMethodFactory.h
* 积分方法工厂
*/

#ifndef INTEGRATIONMETHODFACTORY_H
#define INTEGRATIONMETHODFACTORY_H
#include "../source_file_realtime_v1_async/Intergration/inc/IntegerationMethod.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/Intergration/inc/FixStep/ForwardEulerMethod.h"

template<typename NeuralModel>
class IntegrationMethodFactory {
public:
	static IntegerationMethod* createIntegerationMethod(ModelDescription intDescription, NeuralModel* neuralmodel) {
		if (intDescription.ModelName == "ForwardEulerMethod") {
			IntegerationMethod* Int_Model = ForwardEulerMethod<NeuralModel>::CreateIntegerationMethod(intDescription.ModelParameter, neuralmodel);
			return Int_Model;
		}
		else {
			std::cout << "Unknown learning rule type: " << intDescription.ModelName << std::endl;
			return 0;
		}
	}
};





#endif // INTEGRATIONMETHODFACTORY_H
