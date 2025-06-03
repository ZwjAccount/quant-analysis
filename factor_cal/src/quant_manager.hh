#ifndef QUANT_MANAGER_HH
#define QUANT_MANAGER_HH


#include <stdio.h>
#include <time.h>

#include <map>
#include <vector>
#include <iostream>

#include "hq_ops_efh.h"


class QuantManager {
 public:
  QuantManager();
  virtual ~QuantManager();
  
  void SetConfigPath(std::string config_path);

  //行情控制（接收停止）
  int StartRecvEfhHq();
  int StopRecvEfhHq();  //0代表成功 -1代表失败

 private:
  EfhHq efh_hq_source_;
  std::string config_path_;
};

#endif //HQ_OPS_MANAGER_H_