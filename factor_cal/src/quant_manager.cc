#include "quant_manager.hh"

QuantManager:: QuantManager() {}
QuantManager::~QuantManager() {}

void QuantManager::SetConfigPath(std::string config_path) {
  config_path_ = config_path;
}


int QuantManager::StartRecvEfhHq() {
  return efh_hq_source_.InitAndStartReciveHq(config_path_);
}

int QuantManager::StopRecvEfhHq() {
  return efh_hq_source_.StopRecvHq();
}





