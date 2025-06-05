#include "prepare_data.h"

double market_data_producer::price_to_double(const double& price)
{
    if (price >= up_limit)
        return 1.0; // 涨停价归一化为1.0
    if (price <= down_limit)
        return 0.0;                                        // 跌停价归一化为0.0
    return (price - down_limit) / (up_limit - down_limit); // 其他价格归一化处理
}

double market_data_producer::price_to_int(const double& price)
{
    if (price >= up_limit)
        return 100; // 涨停价归一化为200
    if (price <= down_limit)
        return 0;                                                                  // 跌停价归一化为0
    return static_cast<int>((price - down_limit) / (up_limit - down_limit) * 200); // 其他价格归一化处理
}
void  market_data_producer::load_data_from_yesterday(QueueWithKlineIns* psrc)
{
    SPSCQueue<RsiWithTimestamp> &one_min_rsi = *(psrc->one_min_rsi);
    SPSCQueue<MacdWithTimestamp> &one_min_macd = *(psrc->one_min_macd);
    SPSCQueue<RsiWithTimestamp> &five_min_rsi = *(psrc->five_min_rsi);
    SPSCQueue<MacdWithTimestamp> &five_min_macd = *(psrc->five_min_macd);
    SPSCQueue<RsiWithTimestamp> &ten_min_rsi = *(psrc->ten_min_rsi);
    SPSCQueue<MacdWithTimestamp> &ten_min_macd = *(psrc->ten_min_macd);
    // 依次调用函数加载各个时间周期的RSI和MACD数据
    load_from_source<1>(yesterday_data.template get_rsi<1>(), today_data.template get_rsi<1>(), one_min_rsi);
    load_from_source<5>(yesterday_data.template get_rsi<5>(), today_data.template get_rsi<5>(), five_min_rsi);
    load_from_source<10>(yesterday_data.template get_rsi<10>(), today_data.template get_rsi<10>(), ten_min_rsi);
    load_from_source<1>(yesterday_data.template get_macd<1>(), today_data.template get_macd<1>(), one_min_macd);
    load_from_source<5>(yesterday_data.template get_macd<5>(), today_data.template get_macd<5>(), five_min_macd);
    load_from_source<10>(yesterday_data.template get_macd<10>(), today_data.template get_macd<10>(), ten_min_macd);
    // todo: 加载涨跌停价格
}
