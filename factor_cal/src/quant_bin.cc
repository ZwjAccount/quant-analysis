#include <iostream>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>

#include "quant_manager.hh"
#include "quant_data_proc.hh"
#include "ConfigFile.h"

#define CONF_FILE_PATH  "./hq_ops_efh.ini"
#define DATA_PROC_INFO_CONFIG "./config.conf"

int main(int argc, char *argv[]) {

  xini_file_t conf_file(DATA_PROC_INFO_CONFIG);
  std::string code_list = conf_file["REGIST_CODES"]["codes"].m_xstr_value;
  int queue_szie = stoi(conf_file["QUEUE_INFO"]["map_queue_size"].m_xstr_value);
  std::cout << "code_list is: " << code_list << std::endl;
  std::cout << "queue_szie is: " << queue_szie << std::endl;

  QuantManager manager;
  manager.SetConfigPath(CONF_FILE_PATH);
  if (manager.StartRecvEfhHq() != 0) {
    std::cout << "manager StartRecvEfhHq failed!" << std::endl;
    return -1;
  }



  QuantDataProc& quant_data_proc = QuantDataProc::GetInstance();
  QuantFactoeCal quant_fac_cal;
  QuantKlineProc kline_date_proc(&quant_fac_cal);
  quant_data_proc.kline_proc_ = &kline_date_proc;
  quant_data_proc.InitSecurityMap(code_list, queue_szie);



  uint64_t thread_num = (quant_data_proc.code_queue_.size() / STOCK_NUM_PER_THREAD) + 1;
  std::vector<std::thread> threads(thread_num);

  for (uint64_t i = 0; i < thread_num; ++i) {
      threads[i] = std::thread([&kline_date_proc, i]() {
          kline_date_proc.GetNewestKline(i);  // 原来是从 1 开始编号
          std::string thread_name = "GetNewestKline_" + std::to_string(i);
          message::SetThreadName(thread_name.c_str());
      });
  }


  std::thread quant_data_proc_thread([&quant_data_proc]() { 
      message::SetThreadName("StoreSnapInMapAccordingCode");
      quant_data_proc.DataClassify();
  });


  std::cout << "init efh success" << std::endl;
  for (auto& t: threads) {
    t.join();
  }
  quant_data_proc_thread.join();
}
