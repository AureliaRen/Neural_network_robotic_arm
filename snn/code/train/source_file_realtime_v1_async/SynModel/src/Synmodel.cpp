#include "../source_file_realtime_v1_async/SynModel/inc/Synmodel.h"


Synmodel::Synmodel(int timesteps):timestep_size(timesteps), synapsejumpindex(0){

}

Synmodel::~Synmodel() {
	if (this->synapsejumpindex != 0) {
		delete[] this->synapsejumpindex;
	}

}

int Synmodel::getTimestepSize() {
	return this->timestep_size;
}

void Synmodel::setTimestepSize(int timesteps) {
	this->timestep_size = timesteps;
}

void Synmodel::setModelName(std::string name) {
	this->model_name = name;
}

std::string Synmodel::getModelName() {
	return this->model_name;
}

std::map<std::string, boost::any> Synmodel::getParameters() {
	std::map<std::string, boost::any> parametermap;
	parametermap["name"] = this->getModelName();
	parametermap["timestep_size"] = this->getTimestepSize();
	return parametermap;
}

void Synmodel::setParameters(std::map<std::string, boost::any> parameters) {
	std::map<std::string, boost::any>::iterator it = parameters.find("timestep_size");
	if (it != parameters.end()) {
		int timestep_size = boost::any_cast<int>(it->second);
		this->setTimestepSize(timestep_size);
		parameters.erase(it);
	}
	it = parameters.find("name");
	if (it != parameters.end()) {
        std::string name = boost::any_cast<std::string>(it->second);
		this->setModelName(name);
		parameters.erase(it);
	}
}

