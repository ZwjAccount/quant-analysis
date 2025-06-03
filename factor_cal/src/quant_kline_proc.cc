#include <iostream>
#include "quant_kline_proc.hh"


QuantKlineProc::QuantKlineProc(QuantFactoeCal* ptr) : fac_cal_(ptr), thread_flag_(true) {
  try {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *tmp = localtime(&ts.tv_sec);
    char day[12] = {0};
    snprintf(day, 12, "%04d%02d%02d", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday);

    std::string m_day = day;

    std::string filename = "one_min_kline_" + m_day + ".csv";
    one_min_kline_file_ = fopen(filename.data(), "a");

    filename = "five_min_kline_" + m_day + ".csv";
    five_min_kline_file_ = fopen(filename.data(), "a");

    filename = "fifteen_min_kline_" + m_day + ".csv";
    fifteen_min_kline_file_ = fopen(filename.data(), "a");
  } catch (const std::exception& ex) {
    std::cerr << "Exception caught: " << ex.what() << std::endl;
    exit(EXIT_FAILURE);
}
}
QuantKlineProc::~QuantKlineProc() {}


void QuantKlineProc::UpdateKlineIns(Snap* snap, int time_gap, KlineData& ins, std::vector<KlineData>& kline_queue,
                                    uint64_t& rsi_cur_index, std::unique_ptr<SPSCQueue<RsiWithTimestamp>>& rsi_queue,
                                    uint64_t& macd_index, std::unique_ptr<SPSCQueue<MacdWithTimestamp>>& macd_queue) {
  uint64_t new_min = (snap->time_stamp / 100000) % 10000;
  std::cout << "new_min: " << new_min << std::endl;

  if ((new_min >= 930 && new_min <= 1130) || (new_min >= 1300 && new_min <= 1500)) {
    uint64_t hour = new_min / 100;
    uint64_t min = new_min % 100;
    uint64_t min_gap = hour * 60 + min;

    if (ins.time_stamp == 0) {
      if (time_gap == 5) {
        if ((min_gap % 5) == 0) {
          ins.time_stamp = new_min;
          ins.high_price = snap->last_price;
          ins.open_price = snap->last_price;
          ins.low_price = snap->last_price;
          ins.close_price = snap->last_price;
          ins.code = std::string(snap->security_id);
        }
      } else if (time_gap == 15) {
        if ((min_gap % 15) == 0) {
          ins.time_stamp = new_min;
          ins.high_price = snap->last_price;
          ins.open_price = snap->last_price;
          ins.low_price = snap->last_price;
          ins.close_price = snap->last_price;
          ins.code = std::string(snap->security_id);
        }
      } else {
          ins.time_stamp = new_min;
          ins.high_price = snap->last_price;
          ins.open_price = snap->last_price;
          ins.low_price = snap->last_price;
          ins.close_price = snap->last_price;
          ins.code = std::string(snap->security_id);
      }
    } else {
      int current_gap = new_min - ins.time_stamp;
      std::cout << "current_gap: " << current_gap << std::endl;
      std::cout << "time_gap: " << time_gap << std::endl;
      if (current_gap >= time_gap) {
        kline_queue.push_back(ins);
        if (time_gap == 1) {
          std::cout << "in time gap 1" << std::endl;
          std::cout << "open_price: " << ins.open_price << std::endl;
          StoreKline(ins, one_min_kline_file_);
        } else if(time_gap == 5) {
          StoreKline(ins, five_min_kline_file_);
        } else {
          StoreKline(ins, fifteen_min_kline_file_);
        }
        GetRsi(kline_queue, rsi_cur_index, rsi_queue);
        GetMacd(kline_queue, macd_index, macd_queue);
        ins.time_stamp = new_min;
        ins.high_price = snap->last_price;
        ins.open_price = snap->last_price;
        ins.low_price = snap->last_price;
        ins.close_price = snap->last_price;
        ins.code = std::string(snap->security_id);
        for (int i = 0; i < 10; i++) {
          ins.bid_info[i] = snap->bid_info[i];
          ins.ask_info[i] = snap->ask_info[i];
        }
      }
      ins.high_price = std::max(ins.high_price, snap->last_price);
      ins.low_price = std::min(ins.low_price, snap->last_price);
      ins.close_price = snap->last_price;
      for (int i = 0; i < 10; i++) {
        ins.bid_info[i] = snap->bid_info[i];
        ins.ask_info[i] = snap->ask_info[i];
      }
    }
  }
}

void QuantKlineProc::StoreKline(KlineData& ins, FILE* file) {
  fprintf(file, "%s,%lu,%.2f,%.2f,%.2f,%.2f,%.2f", ins.code.c_str(), ins.time_stamp, ins.high_price, 
    ins.open_price, ins.low_price, ins.close_price, ins.pre_close_price);

  for (int i = 0; i < 10; i++) {
    fprintf(file, ",%.2f,%lu", ins.bid_info[i].price, ins.bid_info[i].qty);
  }
  for (int i = 0; i < 10; i++) {
    fprintf(file, ",%.2f,%lu", ins.ask_info[i].price, ins.ask_info[i].qty);
  }
  fprintf(file, "\n");
  fflush(file);
}

void QuantKlineProc::GetNewestKline(int thread_num) {
  int code_num = queue_location_.size();
  int begin_index = STOCK_NUM_PER_THREAD * thread_num;
  int cal_end_index = STOCK_NUM_PER_THREAD * (thread_num + 1);
  int real_end_index = std::min(cal_end_index, code_num); 
  while (thread_flag_) {
    for (int i = begin_index; i <  real_end_index; i++) {
      //printf("vector queue_ptr %p\n",  queue_location_[i].queue);
      Snap* snap = queue_location_[i].queue->front();
      if (snap != nullptr) {
        UpdateKlineIns(snap, 1, queue_location_[i].one_min_ins, queue_location_[i].one_min_kline_queue,
                      queue_location_[i].one_min_rsi_deal_index, queue_location_[i].one_min_rsi,
                      queue_location_[i].one_min_macd_deal_index, queue_location_[i].one_min_macd);

        UpdateKlineIns(snap, 5, queue_location_[i].five_min_ins, queue_location_[i].five_min_kline_queue,
                      queue_location_[i].five_min_rsi_deal_index, queue_location_[i].five_min_rsi,
                      queue_location_[i].five_min_macd_deal_index, queue_location_[i].five_min_macd);

        UpdateKlineIns(snap, 15, queue_location_[i].fifteen_min_ins, queue_location_[i].ten_min_kline_queue,
                      queue_location_[i].ten_min_rsi_deal_index, queue_location_[i].ten_min_rsi,
                      queue_location_[i].ten_min_macd_deal_index, queue_location_[i].ten_min_macd);

        queue_location_[i].queue->pop();
      }
    }
  }
}

void QuantKlineProc::GetMacd(std::vector<KlineData>& queue, uint64_t& macd_index, std::unique_ptr<SPSCQueue<MacdWithTimestamp>>& macd_queue) {
  size_t size = queue.size();
  if ((size - macd_index) == 34) {
    auto macd_res = DataReadyAndComputeMacd(queue, macd_index, size - 1, 12, 26, 9);
    std::cout << "dif" << std::get<0>(macd_res) << std::endl;
    std::cout << "dea" << std::get<1>(macd_res) << std::endl;
    std::cout << "macd" << std::get<2>(macd_res) << std::endl;

    MacdWithTimestamp dif_val(queue[size-1].code, queue[size-1].time_stamp, std::get<0>(macd_res), 
                              std::get<1>(macd_res), std::get<2>(macd_res));
    for (int i = 0; i < 10; i++) {
      dif_val.bid_info[i] = queue[size-1].bid_info[i];
      dif_val.ask_info[i] = queue[size-1].ask_info[i];
    }
    macd_queue->try_push(dif_val);
  }
}

void QuantKlineProc::GetRsi(std::vector<KlineData>& queue, uint64_t& rsi_cur_index, std::unique_ptr<SPSCQueue<RsiWithTimestamp>>& rsi_queue) {
  size_t size = queue.size();
  double one_min_rsi_6 = 0.0, one_min_rsi_12 = 0.0, one_min_rsi_24 = 0.0;
  if ((size - rsi_cur_index) == 25 ) {
    one_min_rsi_6 = DataReadyAndComputeRsi(queue, size - 1, 6);
    one_min_rsi_12 = DataReadyAndComputeRsi(queue, size - 1, 12);
    one_min_rsi_24 = DataReadyAndComputeRsi(queue, size - 1, 24);
    std::cout << "one_min_rsi_6" << one_min_rsi_6 << std::endl;
    std::cout << "one_min_rsi_12" << one_min_rsi_12 << std::endl;
    std::cout << "one_min_rsi_24" << one_min_rsi_24 << std::endl;
    rsi_cur_index++;

    RsiWithTimestamp tmp(queue[size-1].code, queue[size-1].time_stamp, one_min_rsi_6, one_min_rsi_12, one_min_rsi_24);
    for (int i = 0; i < 10; i++) {
      tmp.bid_info[i] = queue[size-1].bid_info[i];
      tmp.ask_info[i] = queue[size-1].ask_info[i];
    }
    rsi_queue->try_push(tmp);
  }
}

double QuantKlineProc::DataReadyAndComputeRsi(std::vector<KlineData>& src, uint64_t end_idx, int period) {
  std::vector<double> close_prices;
  for (int i = (end_idx - period); i <= end_idx; i++) {
    close_prices.push_back(src[i].close_price);
  }
  return fac_cal_->ComputeRSI(close_prices, period);
}

std::tuple<double, double, double> QuantKlineProc::DataReadyAndComputeMacd(
      std::vector<KlineData>& src, uint64_t beg_idx, uint64_t end_idx, 
      int fastPeriod, int slowPeriod, int signalPeriod) {
  std::vector<double> close_prices;
  for (int i = beg_idx; i <= end_idx; i++) {
    close_prices.push_back(src[i].close_price);
  }
  return fac_cal_->ComputeMACD(close_prices, fastPeriod, slowPeriod, signalPeriod);
}
