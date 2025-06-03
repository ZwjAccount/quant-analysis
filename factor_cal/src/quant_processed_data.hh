#ifndef QUANT_PROCESSED_DATA_HH
#define QUANT_PROCESSED_DATA_HH

#include <cstddef>
#include <cstdint>
#include <memory>
using namespace rigtorp;
using namespace message;
struct KlineData {
  std::string code;
  uint64_t time_stamp;
  double high_price;
  double open_price;
  double low_price;
  double close_price;
  double pre_close_price;

  PriceQty bid_info[10];           //买十档
  PriceQty ask_info[10];           //卖十档

  KlineData() noexcept
    : time_stamp(0), high_price(0), open_price(0), 
      low_price(0), close_price(0), pre_close_price(0) {}
  ~KlineData() noexcept {}
};


struct OrderDetail {
  double price;
  uint64_t qty;

  OrderDetail() noexcept
    : price(0), qty(0) {}
  ~OrderDetail() noexcept {}
};


struct BidAskInfo {
  OrderDetail bid_info[10];
  OrderDetail ask_info[10];

  uint64_t num_trades;                  //成交笔数
  uint64_t total_volume_trade;          //成交总量
  uint64_t total_value_trade;           //成交总金额

  int64_t bid_weighted_avg_price;      //买入委托加权平均价
  int64_t bid_total_qty;               //买入委托总数量
  int64_t ask_weighted_avg_price;      //卖出委托加权平均价
  int64_t ask_total_qty;               //卖出委托总数量

  BidAskInfo() noexcept
   : num_trades(0), total_value_trade(0), total_volume_trade(0),
     bid_weighted_avg_price(0), bid_total_qty(0), ask_weighted_avg_price(0), ask_total_qty(0) {}

  ~BidAskInfo() noexcept {}
};

struct RsiWithTimestamp {
  std::string code;
  uint64_t time_stamp;
  double rsi_6;
  double rsi_12;
  double rsi_24;

  PriceQty bid_info[10];           //买十档
  PriceQty ask_info[10];           //卖十档

  RsiWithTimestamp(std::string security_id, uint64_t time, double cal_rsi6, double cal_rsi_12, double cal_rsi24) noexcept 
   : code(security_id), time_stamp(time), rsi_6(cal_rsi6), rsi_12(cal_rsi_12), rsi_24(cal_rsi24) {}
  RsiWithTimestamp() noexcept {}
  ~RsiWithTimestamp() noexcept {} 
};

struct MacdWithTimestamp {
  std::string code;
  uint64_t time_stamp;
  double dif;
  double dea;
  double macd;

  PriceQty bid_info[10];           //买十档
  PriceQty ask_info[10];           //卖十档

  MacdWithTimestamp(std::string security_id, uint64_t time, double cal_dif, double cal_dea, double cal_macd) noexcept 
  : code(security_id), time_stamp(time), dif(cal_dif), dea(cal_dea), macd(cal_macd) {}
  MacdWithTimestamp() noexcept {}
  ~MacdWithTimestamp() noexcept {} 
};

struct QueueWithKlineIns {
  SPSCQueue<Snap>* queue;
  KlineData one_min_ins;
  KlineData five_min_ins;
  KlineData fifteen_min_ins;

  std::vector<KlineData> one_min_kline_queue;
  std::vector<KlineData> five_min_kline_queue;
  std::vector<KlineData> ten_min_kline_queue;

  uint64_t one_min_rsi_deal_index;
  uint64_t five_min_rsi_deal_index;
  uint64_t ten_min_rsi_deal_index;
  std::unique_ptr<SPSCQueue<RsiWithTimestamp>> one_min_rsi;
  std::unique_ptr<SPSCQueue<RsiWithTimestamp>> five_min_rsi;
  std::unique_ptr<SPSCQueue<RsiWithTimestamp>> ten_min_rsi;

  uint64_t one_min_macd_deal_index;
  uint64_t five_min_macd_deal_index;
  uint64_t ten_min_macd_deal_index;
  std::unique_ptr<SPSCQueue<MacdWithTimestamp>> one_min_macd;
  std::unique_ptr<SPSCQueue<MacdWithTimestamp>> five_min_macd;
  std::unique_ptr<SPSCQueue<MacdWithTimestamp>> ten_min_macd;
  
  
  QueueWithKlineIns(SPSCQueue<Snap>* ptr) noexcept 
   : queue(ptr), one_min_rsi_deal_index(0), five_min_rsi_deal_index(0), ten_min_rsi_deal_index(0),
     one_min_macd_deal_index(0), five_min_macd_deal_index(0), ten_min_macd_deal_index(0) {

    one_min_rsi = std::move(std::make_unique<SPSCQueue<RsiWithTimestamp>>(240));
    five_min_rsi = std::move(std::make_unique<SPSCQueue<RsiWithTimestamp>>(50));
    ten_min_rsi = std::move(std::make_unique<SPSCQueue<RsiWithTimestamp>>(25));

    one_min_macd = std::move(std::make_unique<SPSCQueue<MacdWithTimestamp>>(240));
    five_min_macd = std::move(std::make_unique<SPSCQueue<MacdWithTimestamp>>(50));
    ten_min_macd = std::move(std::make_unique<SPSCQueue<MacdWithTimestamp>>(25));
  }
  
  QueueWithKlineIns(QueueWithKlineIns&& tmp) 
   : queue(tmp.queue), one_min_rsi_deal_index(tmp.one_min_rsi_deal_index), 
     five_min_rsi_deal_index(tmp.five_min_rsi_deal_index), ten_min_rsi_deal_index(tmp.ten_min_rsi_deal_index),
     one_min_macd_deal_index(tmp.one_min_macd_deal_index), five_min_macd_deal_index(tmp.five_min_macd_deal_index), 
     ten_min_macd_deal_index(tmp.ten_min_macd_deal_index) {
    one_min_rsi = std::move(tmp.one_min_rsi); 
    five_min_rsi = std::move(tmp.five_min_rsi);
    ten_min_rsi = std::move(tmp.ten_min_rsi);

    one_min_macd = std::move(tmp.one_min_macd); 
    five_min_macd = std::move(tmp.five_min_macd);
    ten_min_macd = std::move(tmp.ten_min_macd);
  }

  ~QueueWithKlineIns() noexcept {}  
};

struct RsiPeriod {
  uint64_t period_one;
  uint64_t period_two;
  uint64_t period_three;

  RsiPeriod() : period_one(0), period_two(0), period_three(0) {}
  ~RsiPeriod() {}
};



#endif //QUANT_PROCESSED_DATA_HH