#ifndef __PREPARE_DATA_H__
#define __PREPARE_DATA_H__
#include <cstddef>
#include <cstdint>

#include "quant_data_proc.hh"


template<int data_num>
struct pre_market_data
{
    struct market_data<data_num> data;
    uint64_t time_stamp;        // 结束时间的时间戳（包含），格式为HHMMSS
};

// 为了便于给定时间戳获取数据，使用一个模板类来存储每日数据
template<int span_mi, typename val_t>
struct daily_data_storage
{
    constexpr static TRADE_MINUTE_NUM = (11.5 - 9.5 + 15. - 13.) * 60; // 开市的时间（单位：分钟），9:30到11:30，13:00到15:00，共计120个时间段
    constexpr static ARRAY_LEN = TRADE_MINUTE_NUM / span_mi; // 每span_mi分钟一个数据
    val_t data[ARRAY_LEN]; // 存储span_mi分钟的指标数据

    inline static int cal_idx(const uint64_t timestamp)
    {
        if (timestamp < 93000 || timestamp > 150000) {
            return -1; // 非交易时间
        }
        double hour = timestamp / 10000; // 获取小时
        double minute = (timestamp % 10000) / 100; // 获取分钟
        double second = timestamp % 100; // 获取秒
        int total_minutes = (hour - 9.5) * 60 + minute; // 计算从9:30开始的总分钟数
        if (timestamp >= 130000)
        {
            total_minutes -= 90; // 减去中午休市的90分钟
        }
        return total_minutes / span_mi; // 返回总分钟数除以span_mi作为索引
    }

    val_t& get_timestamp(const uint64_t timestamp)
    {
        size_t idx = cal_idx(timestamp);
        if (idx < ARRAY_LEN) {
            return data[idx]; // 返回对应时间戳的指标数据
        }
        throw std::out_of_range("Index out of range for daily data storage");
    }

    val_t& get_index(const int idx)
    {
        if (idx < ARRAY_LEN) {
            if (idx >= 0)
                return data[idx]; // 返回对应索引的指标数据
            else
                return data[ARRAY_LEN + idx]; // 支持负索引，返回从末尾开始的索引
        }
        throw std::out_of_range("Index out of range for daily data storage");
    }

};

// 1天中各种指标在各种时间周期的数据存储
template<int span_mi, int...args>
struct one_day_data
{
   daily_data_storage<span_min, RsiWithTimestamp> rsi;      // 数据存储，存储了1天的RSI数据，RSI的统计周期为span_min分钟
   daily_data_storage<span_min, MacdWithTimestamp> macd;    // 数据存储，存储了1天的MACD数据，MACD的统计周期为span_min分钟
   one_day_data<args...> next; // 支持递归定义，允许更多的时间跨度


   one_day_data() = default; // 默认构造函数
   one_day_data(const one_day_data& other)
   {
        rsi = other.rsi; // 复制RSI数据
        macd = other.macd; // 复制MACD数据
        if constexpr (sizeof...(args) > 0)
        {
            next = other.next; // 递归复制下一个时间跨度的数据
        }
   }

   one_day_data& operator=(const one_day_data& other)
   {
        if (this != &other) // 防止自赋值
        {
            rsi = other.rsi; // 复制RSI数据
            macd = other.macd; // 复制MACD数据
            if constexpr (sizeof...(args) > 0)
            {
                next = other.next; // 递归复制下一个时间跨度的数据
            }
        }
        return *this;
   }

   // 获取以need_span_mi分钟为单位进行统计的RSI数据
   template<int need_span_mi>
   daily_data_storage<need_span_mi, RsiWithTimestamp>& get_rsi()
   {
        if constexpr (span_min == need_span_min)
        {
            return rsi; // 返回对应时间跨度的RSI数据
        }
        else if constexpr (sizeof...(args) > 0)
        {
            return next.template get_rsi<need_span_mi>(); // 递归查找下一个时间跨度
        }
        else
        {
            static_assert(need_span_mi == span_min, "No RSI data for the requested time span");
        }
   }

    // 获取以need_span_mi分钟为单位进行统计的MACD数据
    template<int need_span_mi>
    daily_data_storage<need_span_mi, MacdWithTimestamp>& get_macd()
    {
        if constexpr (span_min == need_span_min)
        {
            return macd; // 返回对应时间跨度的MACD数据
        }
        else if constexpr (sizeof...(args) > 0)
        {
            return next.template get_macd<need_span_mi>(); // 递归查找下一个时间跨度
        }
        else
        {
            static_assert(need_span_mi == span_min, "No RSI data for the requested time span");
        }
    }
};

struct market_data_producer
{
    struct PriceVolumn
    {
        double price; // 最新价，按照涨跌停价格归一化
        double volume; // 成交量
    };
    double up_limit; // 涨停价
    double down_limit; // 跌停价
    one_day_data<1, 5, 10> yesterday_data; // 昨日数据
    one_day_data<1, 5, 10> today_data;     // 今日数据

    // 价格转换成0-1之间的浮点数，涨停价归一化为1.0，跌停价归一化为0.0，其他价格按比例归一化到0-1之间
    double price_to_double(const double& price)
    {
        if (price >= up_limit) return 1.0; // 涨停价归一化为1.0
        if (price <= down_limit) return 0.0; // 跌停价归一化为0.0
        return (price - down_limit) / (up_limit - down_limit); // 其他价格归一化处理
    }

    // 价格转换成整数，涨停价归一化为200，跌停价归一化为0，其他价格按比例归一化到0-200之间
    int price_to_int(const double& price)
    {
        if (price >= up_limit) return 100; // 涨停价归一化为200
        if (price <= down_limit) return 0; // 跌停价归一化为0
        return static_cast<int>((price - down_limit) / (up_limit - down_limit) * 200); // 其他价格归一化处理
    }

    template<int span_mi, int data_num>
    void get_batch_data(const uint64_t time_stamp, RsiWithTimestamp* rsi_data, int& rsi_idx, MacdWithTimestamp* macd_data, int& macd_idx)
    {
        uint64_t cur_timestamp = time_stamp - span_mi * 100;
        int yesterday_idx = 0;
        for (int i = 0; i < data_num; ++i)
        {
            if (yesterday_idx < 0)
            {
                // 使用昨天数据
                yesterday_data--;
                rsi_data[rsi_idx++] = yesterday_data.template get_rsi<span_mi>.get_index(yesterday_idx);
                macd_data[macd_idx++] = yesterday_data.template get_macd<span_mi>.get_index(yesterday_idx);
            }
            else
            {
                int today_idx = today_data.template get_rsi<span_mi>.cal_idx(cur_timestamp);    // 查看当前时间戳是否有越界，如果越界则使用昨天数据
                if (today_idx < 0)                                          // 如果当前时间戳在今日数据范围之外，则使用昨天数据
                {
                    if (yesterday_idx < 0) // 昨天数据也越界了
                    {
                        rsi_data[rsi_idx++] = rsi_data[0]; // 使用第一个数据作为默认值
                        macd_data[macd_idx++] = macd_data[0];
                    }
                    else
                    {
                        yesterday_idx--;
                        rsi_data[rsi_idx++] = yesterday_data.template get_rsi<span_mi>.get_index(yesterday_idx);
                        macd_data[macd_idx++] = yesterday_data.template get_macd<span_mi>.get_index(yesterday_idx);
                    }
                }
                else                                               // 如果今天的数据索引合法
                {
                    rsi_data[rsi_idx++] = today_data.template get_rsi<span_mi>.get_index(today_idx);
                    macd_data[macd_idx++] = today_data.template get_macd<span_mi>.get_index(today_idx);
                    cur_timestamp -= 100*span_mi;   // 减去当前时间戳的分钟数
                    continue;
                }
            }
        }
    }

    // 获取盘口数据，整理到中间结构中，从time_stamp开始，向前获取data_num个时间点的盘口数据，使用的是rsi里面存储的盘口数据
    template<int data_num>
    void get_price_volumn(const uint64_t time_stamp, PriceVolumn* price_volumn_data, int& idx)
    {
        // todo: 盘口数据转换，把1分钟的盘口信息转换成涨跌停价格百分比的格式并赋值给pre_data.data.mt_price_volume，举例来说，假设涨停价格为h，跌停价格为l，那么如果价格为p，那么转换成的数值为(p - l) / (h - l)，转换后的价格区间为[0, 1]，即涨停价格对应1，跌停价格对应0；申卖申卖数量按照比例转换，即\frac{v_i}{\sum_{j=0}^{19}v_j}，其中v_i为第i档的申买或申卖数量，\sum_{j=0}^{19}v_j为所有20档的申买和申卖数量之和，这样转换后每个档位的申买或申卖数量都在[0, 1]之间。

        uint64_t cur_timestamp = time_stamp;
        int yesterday_idx = 0;
        for (int i = 0; i < data_num; ++i)
        {
            if (yesterday_idx < 0)
            {
                yesterday_idx--;
                double sum_volume = 0.0;
                for (int j = 0; j < 10; ++j)
                {
                    double bid_price = yesterday_data.template get_rsi<1>.get_index(yesterday_idx).bid_info[j].price;
                    double ask_price = yesterday_data.template get_rsi<1>.get_index(yesterday_idx).ask_info[j].price;
                    double bid_qty = (double)yesterday_data.template get_rsi<1>.get_index(yesterday_idx).bid_info[j].qty;
                    double ask_qty = (double)yesterday_data.template get_rsi<1>.get_index(yesterday_idx).ask_info[j].qty;
                    price_volumn_data[idx++] = {bid_price, bid_qty};
                    price_volumn_data[idx++] = {ask_price, ask_qty};
                    sum_volume += bid_qty + ask_qty; // 累加成交量
                }
                // 成交量归一化处理
                for (int j = 0; j < 20; ++j)
                {
                    price_volumn_data[idx - 20 + j].volume /= sum_volume; // 将成交量归一化
                }
            }
            else
            {
                int today_idx = today_data.template get_rsi<1>.cal_idx(cur_timestamp);
                if (today_idx < 0) // 如果当前时间戳在今日数据范围之外，则使用昨天数据
                {
                    yesterday_idx--;
                    double sum_volume = 0.0;
                    for (int j = 0; j < 10; ++j)
                    {
                        double bid_price = yesterday_data.template get_rsi<1>.get_index(yesterday_idx).bid_info[j].price;
                        bid_price = price_to_double(bid_price); // 将价格归一化
                        double ask_price = yesterday_data.template get_rsi<1>.get_index(yesterday_idx).ask_info[j].price;
                        ask_price = price_to_double(ask_price); // 将价格归一化
                        double bid_qty = (double)yesterday_data.template get_rsi<1>.get_index(yesterday_idx).bid_info[j].qty;
                        double ask_qty = (double)yesterday_data.template get_rsi<1>.get_index(yesterday_idx).ask_info[j].qty;
                        price_volumn_data[idx++] = {bid_price, bid_qty};
                        price_volumn_data[idx++] = {ask_price, ask_qty};
                        sum_volume += bid_qty + ask_qty; // 累加成交量
                    }
                    // 成交量归一化处理
                    for (int j = 0; j < 20; ++j)
                    {
                        price_volumn_data[idx - 20 + j].volume /= sum_volume; // 将成交量归一化
                    }
                }
                else // 如果今天的数据索引合法
                {
                    double sum_volume = 0.0;
                    for (int j = 0; j < 10; ++j)
                    {
                        double bid_price = today_data.template get_rsi<1>.get_index(yesterday_idx).bid_info[j].price;
                        bid_price = price_to_double(bid_price); // 将价格归一化
                        double ask_price = today_data.template get_rsi<1>.get_index(yesterday_idx).ask_info[j].price;
                        ask_price = price_to_double(ask_price); // 将价格归一化
                        double bid_qty = (double)today_data.template get_rsi<1>.get_index(yesterday_idx).bid_info[j].qty;
                        double ask_qty = (double)today_data.template get_rsi<1>.get_index(yesterday_idx).ask_info[j].qty;
                        price_volumn_data[idx++] = {bid_price, bid_qty};
                        price_volumn_data[idx++] = {ask_price, ask_qty};
                        sum_volume += bid_qty + ask_qty; // 累加成交量
                    }
                    // 成交量归一化处理
                    for (int j = 0; j < 20; ++j)
                    {
                        price_volumn_data[idx - 20 + j].volume /= sum_volume; // 将成交量归一化
                    }
                    cur_timestamp -= 100; // 减去当前时间戳的分钟数
                }
            }
        }
    }

    template<int data_num>
    void trans_to_pre(const RsiWithTimestamp* rsi_data, int rsi_idx, const MacdWithTimestamp* macd_data, int macd_idx, pre_market_data<data_num>& pre_data)
    {
        // RSI格式转换
        for (int ipre = 0, idata = 0; idata < data_num*3; ipre+=3, idata++)
        {
            if (idata < rsi_idx)
            {
                pre_data.data.mt_rsi[ipre] = rsi_data[idata].rsi_6;
                pre_data.data.mt_rsi[ipre+1] = rsi_data[idata].rsi_12;
                pre_data.data.mt_rsi[ipre+2] = rsi_data[idata].rsi_24;
            }
            else
            {
                pre_data.data.mt_rsi[ipre] = 0.0;
                pre_data.data.mt_rsi[ipre+1] = 0.0;
                pre_data.data.mt_rsi[ipre+2] = 0.0;
            }
        }
        // MACD格式转换
        for (int ipre = 0, idata = 0; idata < data_num*3; ipre+=2, idata++)
        {
            if (idata < macd_idx)
            {
                pre_data.data.mt_macd[ipre] = macd_data[idata].dif;
                pre_data.data.mt_macd[ipre+1] = macd_data[idata].dea;
            }
            else
            {
                pre_data.data.mt_macd[ipre] = 0.0;
                pre_data.data.mt_macd[ipre+1] = 0.0;
            }
        }
    }

    template<int data_num>
    pre_market_data<data_num> get_pre_market_data(const uint64_t time_stamp) const
    {
        RsiWithTimestamp rsi_data[data_num*3];
        int rsi_idx = 0;
        MacdWithTimestamp macd_data[data_num*3];
        int macd_idx = 0;
        get_batch_data<1, data_num>(time_stamp, rsi_data, rsi_idx, macd_data, macd_idx);
        get_batch_data<5, data_num>(time_stamp, rsi_data, rsi_idx, macd_data, macd_idx);
        get_batch_data<10, data_num>(time_stamp, rsi_data, rsi_idx, macd_data, macd_idx);

        PriceVolumn price_volumn_data[data_num*20];
        int price_volumn_idx = 0;
        get_price_volumn<data_num>(time_stamp, price_volumn_data, price_volumn_idx);        // 获取盘口数据

        pre_market_data<data_num> pre_data;
        pre_data.time_stamp = time_stamp;
        trans_to_pre<data_num>(rsi_data, rsi_idx, macd_data, macd_idx, pre_data);
        // todo:找到下一分钟的收盘价格并给pre_data.data.label赋值，赋值是按照涨跌停价格分成200个档位，每0.1%一个档位，举例来说假设前日收盘价格为100块，那么最高最低价格为[110, 90]，那么涨跌停价格百分比为[110%, 90%]，那么每个档位的价格为[110, 109.9, ..., 90.1, 90]，那么如果下一分钟的收盘价格为105块，那么pre_data.data.label = (105 - 90) / (110 - 90) * 200 = 75，普遍而言假设最高价格为h，最低价格为l，那么pre_data.data.label = (next_close_price - l) / (h - l) * 200，注意这里的next_close_price是下一分钟的收盘价格
        uint64_t next_time_stamp = time_stamp + 100; // 假设下一分钟的时间戳
        int next_idx = today_data.template get_rsi<1>.cal_idx(next_time_stamp);
        if (next_idx >= 0)
        {
            double next_close_price = today_data.template get_rsi<1>.get_index(next_idx).close_price; // 获取下一分钟的收盘价格
            pre_data.data.label = price_to_int(next_close_price); // 将收盘价格转换成档位标签
        }
        else
        {
            pre_data.data.label = 0; // 如果没有下一分钟数据，则默认标签为0
        }

        return pre_data;
    }

    template<int span_mi, typename data_t>
    void load_from_source(
        daily_data_storage<span_mi, data_t>& dds_yesterday,
        daily_data_storage<span_mi, data_t>& dds_today,
        SPSCQueue<data_t>& queue)
    {
        // 从数据源加载span_mi分钟的RSI数据到dds中
        uint64_t last_timestamp = 0;
        data_t _data;
        daily_data_storage<span_mi, data_t>* pdds = &dds_yesterday;
        while (queue.pop(_data))
        {
            if (_data.time_stamp < last_timestamp) // 如果时间戳小于最后一个时间戳说明跨天了，使用今天的
            {
                if (pdds == &dds_today)
                {
                    dds_today = dds_yesterday;      // 如果已经是今天的数据了，还有跨日的情况说明队列里面可能有多天的数据
                }
                else
                {
                    pdds = &dds_today; // 切换到今天的数据存储
                }
            }
            last_timestamp = _data.time_stamp; // 更新最后一个时间戳
            int idx = pdds->cal_idx(_data.time_stamp); // 计算索引
            if (idx >= 0 && idx < pdds->ARRAY_LEN) // 确保索引在有效范围内
            {
                pdds->data[idx] = _data; // 存储RSI数据
            }
            else
            {
                // 索引越界，可能是数据不完整或时间戳不在交易时间内
                continue;
            }
        }
    }

    // 从数据源中加载昨天和今天的RSI和MACD数据到对应的存储中
    void load_data_from_yesterday(QueueWithKlineIns* psrc)
    {
        SPSCQueue<RsiWithTimestamp>& one_min_rsi = *(psrc->one_min_rsi);
        SPSCQueue<MacdWithTimestamp>& one_min_macd = *(psrc->one_min_macd);
        SPSCQueue<RsiWithTimestamp>& five_min_rsi = *(psrc->five_min_rsi);
        SPSCQueue<MacdWithTimestamp>& five_min_macd = *(psrc->five_min_macd);
        SPSCQueue<RsiWithTimestamp>& ten_min_rsi = *(psrc->ten_min_rsi);
        SPSCQueue<MacdWithTimestamp>& ten_min_macd = *(psrc->ten_min_macd);
        // 依次调用函数加载各个时间周期的RSI和MACD数据
        load_from_source<1>(yesterday_data.template get_rsi<1>(), today_data.template get_rsi<1>(), one_min_rsi);
        load_from_source<5>(yesterday_data.template get_rsi<5>(), today_data.template get_rsi<5>(), five_min_rsi);
        load_from_source<10>(yesterday_data.template get_rsi<10>(), today_data.template get_rsi<10>(), ten_min_rsi);
        load_from_source<1>(yesterday_data.template get_macd<1>(), today_data.template get_macd<1>(), one_min_macd);
        load_from_source<5>(yesterday_data.template get_macd<5>(), today_data.template get_macd<5>(), five_min_macd);
        load_from_source<10>(yesterday_data.template get_macd<10>(), today_data.template get_macd<10>(), ten_min_macd);
    }
};


#endif
