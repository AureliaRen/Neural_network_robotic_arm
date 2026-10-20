#include "../source_file_realtime_v1_async/SynModel/inc/non_int/Non_int.h"

Non_int::Non_int(int timestep) : Synmodel(timestep) {
	
}

Non_int::~Non_int() {
	
}


std::map<std::string, boost::any> Non_int::getParameters() {
	std::map<std::string, boost::any> parametermap;
	parametermap = Synmodel::getParameters();
	return parametermap;
}

void Non_int::setParameters(std::map<std::string, boost::any> parametermap) {
	Synmodel::setParameters(parametermap);
}

