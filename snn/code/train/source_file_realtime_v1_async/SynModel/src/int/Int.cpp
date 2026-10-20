#include "../source_file_realtime_v1_async/SynModel/inc/int/Int.h"
#include "../source_file_realtime_v1_async/Intergration/inc/IntegerationMethod.h"

Int::Int(int timestep) : Synmodel(timestep), integrationMethod(0) {

}

Int::~Int() {
	if (this->integrationMethod != 0) {
		delete this->integrationMethod;
	}
}

std::map<std::string, boost::any> Int::getParameters() {
	std::map<std::string, boost::any> parametermap;
	parametermap = Synmodel::getParameters();
	return parametermap;
}

void Int::setParameters(std::map<std::string, boost::any> parametermap) {
	Synmodel::setParameters(parametermap);
}
