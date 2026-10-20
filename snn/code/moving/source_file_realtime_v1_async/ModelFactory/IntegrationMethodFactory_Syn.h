/*
* IntegrationMethodFactory_Syn.h
*/
#ifndef _INTEGRATIONMETHODFACTORY_SYN_H_
#define _INTEGRATIONMETHODFACTORY_SYN_H_

#include "../source_file_realtime_v1_async/Intergration/inc/IntegerationMethod.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/Intergration/inc/FixStep/ForwardEulerMethod_Syn.h"

template<typename SynModel>
class IntegrationMethodFactory_Syn {
public:
	static IntegerationMethod_Syn* createIntegerationMethod(ModelDescription intDescription, SynModel* synmodel) {
		if (intDescription.ModelName == "ForwardEulerMethod") {
			IntegerationMethod_Syn* Int_Model = ForwardEulerMethod_Syn<SynModel>::CreateIntegerationMethod(intDescription.ModelParameter, synmodel);
			return Int_Model;
		}
		else {
			std::cout << "Unknown learning rule type: " << intDescription.ModelName << std::endl;
			return 0;
		}
	}
};

#endif