#ifndef QUANT_KLINE_PROC_HH
#define QUANT_KLINE_PROC_HH

#include <vector>
#include <tuple>

#include"message.hh"
#include "quant_processed_data.hh"
#include "quant_factor_cal.hh"

using namespace rigtorp;
using namespace message;
#define PERIOD_NUM 3
#define STOCK_NUM_PER_THREAD 5

class QuantKlineProc {
 public:
  QuantKlineProc(QuantFactoeCal* ptr);
  ~QuantKlineProc();

  void UpdateKlineIns(Snap* snap, int time_gap, KlineData& ins, std::vector<KlineData>& kline_queue,
                      uint64_t& rsi_cur_index, std::unique_ptr<SPSCQueue<RsiWithTimestamp>>& rsi_queue,
                      uint64_t& macd_index, std::unique_ptr<SPSCQueue<MacdWithTimestamp>>& macd_queue);
  void GetNewestKline(int thread_num);
  void StoreKline(KlineData& ins, FILE* file);
  void GetRsi(std::vector<KlineData>& queue, uint64_t& rsi_cur_index, std::unique_ptr<SPSCQueue<RsiWithTimestamp>>& rsi_queue);
  void GetMacd(std::vector<KlineData>& queue, uint64_t& macd_index, std::unique_ptr<SPSCQueue<MacdWithTimestamp>>& one_min_macd);

  double DataReadyAndComputeRsi(std::vector<KlineData>& src, uint64_t end_idx, int period);
  std::tuple<double, double, double> 
  DataReadyAndComputeMacd(std::vector<KlineData>& src, uint64_t beg_idx, uint64_t end_idx, 
                          int fastPeriod, int slowPeriod, int signalPeriod);

 public:
  std::atomic<bool> thread_flag_;
  FILE* one_min_kline_file_;   
  FILE* five_min_kline_file_;  
  FILE* fifteen_min_kline_file_;  
  QuantFactoeCal* fac_cal_;
  std::vector<QueueWithKlineIns> queue_location_; //每一只票对应一个spsc的队列
  
};

#endif //QUANT_KLINE_PROC_HH
