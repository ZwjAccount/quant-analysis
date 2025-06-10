#ifndef __MODEL_STRUCT_H__
#define __MODEL_STRUCT_H__
#include "mat.hpp"

template<int data_num, int output_num>
struct market_data
{
   static const int RSI_SIZE = (data_num + data_num + data_num)*3;        // RSI6/12/24、1/5/10顺序排列
   static const int MACD_SIZE = (data_num + data_num)*3;        // DIF/DEA、1/5/10顺序排列
   static const int KDJ_SIZE = (data_num + data_num + data_num)*3;        // KDJ1/5/10顺序排列
   static const int PRICE_VOLUME_SIZE = 20 * 2 * data_num;         // 10档价格/成交量，最后data_num个盘口信息
   mat<RSI_SIZE, 1, double>            mt_rsi;                    // RSI
   mat<MACD_SIZE, 1, double>           mt_macd;                   // MACD
   mat<KDJ_SIZE, 1, double>            mt_kdj;                    // KDJ
   mat<PRICE_VOLUME_SIZE, 1, double>   mt_price_volume;           // 10档盘口+成交量
   int i_time;                                                  // 将1天的事件按分钟进行划分，d_time为分钟数
   int labels[output_num];                                  // 输出的标签，涨跌10%精确到0.1%就是200个档位
};

template<typename trans_name, typename raw_data_type>
struct trans_t
{
};

template<typename raw_data_type>
struct trans_t<class rsi_pv, raw_data_type>
{
    static constexpr int input_size = raw_data_type::RSI_SIZE + raw_data_type::PRICE_VOLUME_SIZE;
    static mat<input_size, 1, double> trans_data_type(const raw_data_type &data)
    {
        return join_col(data.mt_rsi, data.mt_price_volume);
    }
};

template<typename raw_data_type>
struct trans_t<class macd_pv, raw_data_type>
{
    static constexpr int input_size = raw_data_type::MACD_SIZE + raw_data_type::PRICE_VOLUME_SIZE;
    static mat<input_size, 1, double> trans_data_type(const raw_data_type& data)
    {
        return join_col(data.mt_macd, data.mt_price_volume);
    }
};

template<typename raw_data_type>
struct trans_t<class kdj_pv, raw_data_type>
{
    static constexpr int input_size = raw_data_type::KDJ_SIZE + raw_data_type::PRICE_VOLUME_SIZE;
    static mat<input_size, 1, double> trans_data_type(const raw_data_type& data)
    {
        return join_col(data.mt_kdj, data.mt_price_volume);
    }
};

template<typename raw_data_type>
struct trans_t<class macd_rsi, raw_data_type>
{
    static constexpr int input_size = raw_data_type::MACD_SIZE + raw_data_type::RSI_SIZE;
    static mat<input_size, 1, double> trans_data_type(const raw_data_type& data)
    {
        return join_col(data.mt_macd, data.mt_rsi);
    }
};

template<typename raw_data_type>
struct trans_t<class all_idx, raw_data_type>
{
    static constexpr int input_size = raw_data_type::RSI_SIZE + raw_data_type::MACD_SIZE + raw_data_type::KDJ_SIZE + raw_data_type::PRICE_VOLUME_SIZE;
    static mat<input_size, 1, double> trans_data_type(const raw_data_type& data)
    {
        return join_col(data.mt_rsi, data.mt_macd, data.mt_kdj, data.mt_price_volume);
    }
};



#endif
