#ifndef QUANT_DATA_PROC_HH
#define QUANT_DATA_PROC_HH

#include<unordered_map>
#include<vector>
#include <memory>
#include <sstream>  // 用于 std::stringstream
#include <string>
#include <iostream>

#include "quant_kline_proc.hh"
#include"quant_processed_data.hh"

using namespace rigtorp;
using namespace message;
class QuantDataProc {
 public:
  // 获取单例实例的方法
  static QuantDataProc& GetInstance();
  
  // 禁用拷贝构造函数和拷贝赋值运算符
  QuantDataProc(const QuantDataProc&) = delete;
  QuantDataProc& operator=(const QuantDataProc&) = delete;

  void InitSecurityMap(std::string codes, size_t queue_size);
  void DataClassify();

  

 public:

  QuantKlineProc* kline_proc_;
  std::atomic<bool> thread_flag_;
  std::unordered_map<std::string, std::unique_ptr<SPSCQueue<Snap>>>  code_queue_;

 private:
  QuantDataProc();
  ~QuantDataProc();

};

#endif //QUANT_DATA_PROC_HH


