#ifndef MESSAGE_HH
#define MESSAGE_HH

#include <sys/time.h>
#include <stdint.h>

#include <string.h>
#include <map>
#include <memory>
#include <sys/prctl.h>

#pragma pack()
#include "SPSCQueue.h"
#define sz_suffix ".sz"
#define sh_suffix ".sh"

#define SNAP_QUEUE_SIZE 1024 * 4096
#define INDEX_QUEUE_SIZE 1024 * 256

using namespace rigtorp;
namespace message {

void SetThreadName(const char* name);
int bindCpu(int i);

enum MessageType : uint8_t {
  SNAP = 0,     //初始化的行情，无意义
  INDEX = 1,    //指数行情
  TICK = 2,     //股票基金行情
};

struct PriceQty {
  double price;    //申买、申卖价格，实际值除以1000000
  uint64_t qty;      //申买、申卖数量，实际值除以100

  PriceQty() noexcept
    : price(0), qty(0) {}
  ~PriceQty() noexcept {}
};

struct Snap {
  MessageType  message_type;     //消息类型，指数为SZE_INDEX
  uint64_t  time_stamp;                 //数据生成时间（切片时间），格式YYYYMMDDHHMMSSmmm，精确到毫秒。
                                       //示例：20190411102939120表示2019年04月11日10点29分39秒120毫秒
  uint16_t channel_no;                  //频道代码

  char     security_id[16];             //证券代码
  uint32_t  exchange_id;                //交易所id，上交所：101，深交所：102，港交所：103

  uint64_t num_trades;                  //成交笔数
  uint64_t total_volume_trade;          //成交总量
  double total_value_trade;           //成交总金额

  double pre_close_price;             //昨收价
  double open_price;                  //开盘价
  double last_price;                  //最新价

  double high_price;                 //最高价
  double low_price;                  //最低价
  double close_price;                //收盘价

  double bid_weighted_avg_price;      //买入委托加权平均价
  int64_t bid_total_qty;               //买入委托总数量
  double ask_weighted_avg_price;      //卖出委托加权平均价
  int64_t ask_total_qty;               //卖出委托总数量

  PriceQty bid_info[10];           //买十档
  PriceQty ask_info[10];           //卖十档

  Snap() noexcept
    : message_type(MessageType::SNAP), time_stamp(0), channel_no(0), exchange_id(0),
      num_trades(0), total_volume_trade(0), total_value_trade(0), pre_close_price(0),
      open_price(0), last_price(0), high_price(0), low_price(0), close_price(0) {
    memset(security_id, 0, sizeof(security_id));
  }
  ~Snap() noexcept {}
};


struct Index {
  MessageType  message_type;     //消息类型，指数为SZE_INDEX
  uint64_t  time_stamp;                 //数据生成时间（切片时间），格式YYYYMMDDHHMMSSmmm，精确到毫秒。
                                       //示例：20190411102939120表示2019年04月11日10点29分39秒120毫秒
  uint16_t channel_no;                  //频道代码

  char     security_id[16];             //证券代码
  uint32_t  exchange_id;                //交易所id，上交所：101，深交所：102，港交所：103

  int64_t num_trades;                    //成交笔数
  int64_t total_volume_trade;            //成交总量
  int64_t total_value_trade;             //成交总金额
  int64_t pre_close_price;               //昨收盘指数
  int64_t last_price;                    //最新指数
  int64_t open_price;                    //开盘指数
  int64_t high_price;                    //最高指数
  int64_t low_price;                     //最低指数
  int64_t close_price;                   //收盘指数


  Index() noexcept
    : message_type(MessageType::INDEX), time_stamp(0), channel_no(0), exchange_id(0),
      num_trades(0), total_volume_trade(0), total_value_trade(0), pre_close_price(0),
      last_price(0), open_price(0), high_price(0), low_price(0), close_price(0) {
    memset(security_id, 0, sizeof(security_id));
  }
  ~Index() noexcept {}
};

extern std::atomic<bool> flag;
extern SPSCQueue<Snap>  snap_spsc_queue;
extern SPSCQueue<Index> index_spsc_queue;

} // namespace SzeMessage

#endif // message