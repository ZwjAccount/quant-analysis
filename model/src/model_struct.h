#ifndef __MODEL_STRUCT_H__
#define __MODEL_STRUCT_H__
#include "mat.hpp"
#include "dbn_t.hpp"

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


// DBN中的RBM进行学习，然后通过多个bp神经网络进行微调
template<int predict_num, typename val_t, int first_input_row, int...args>
struct predict_net_t
{
    using bp_type = bp<val_t, 1, nadam, ReLu, XavierGaussian, first_input_row, args...>;
    bp_type m_bps[predict_num];    // 每个预测结果对应一个BP神经网络
    using input_type = typename bp_type::input_type;
    using ret_type = mat<bp_type::ret_type::r, predict_num, val_t>;   // 输出结果的类型
    using softmax_type = bp<val_t, 1, nadam, softmax, XavierGaussian, bp_type::ret_type::r, 200>; // Softmax层
    softmax_type m_softmax[predict_num];    // 每个预测结果对应一个Softmax层

    ret_type forward(const input_type& input)
    {
        ret_type ret;
        for (int i = 0; i < predict_num; ++i)
        {
            auto&& bp_out = m_softmax[i].forward(m_bps[i].forward(input));    // 前向传播
            for (int j = 0; j < bp_type::ret_type::r; ++j)
            {
                ret.get(j, i) = bp_out.get(j, 0);    // 将每个BP的输出结果存入ret
            }
        }
        return ret;
    }

    input_type backward(const ret_type& ret)
    {
        input_type input;
        for (int i = 0; i < predict_num; ++i)
        {
            auto&& bp_out = m_bps[i].backward(m_softmax[i].backward(ret.col(i)));    // 反向传播
            for (int j = 0; j < bp_type::input_type::r; ++j)
            {
                input.get(j, 0) += bp_out.get(j, 0);    // 将每个BP的输入结果累加到input
            }
        }
        input = input / static_cast<val_t>(predict_num);    // 平均化输入
        return input;
    }
};

struct predict_result
{
    int idx;            // 预测结果的索引
    double d_poss;     // 预测的概率
};

template<typename trans_name, typename raw_data_type, int output_num>
class proxy_dbn_t
{
private:
    using local_trans_t = trans_t<trans_name, raw_data_type>;
    template<int ih>
    using predict_t = predict_net_t<ih, double, 200, 200, 200>;
    using dbn_type = dbn_t<predict_t, double, local_trans_t::input_size, local_trans_t::input_size/2, local_trans_t::input_size/4>;
    using input_type = typename dbn_type::input_type;
    using ret_type = typename dbn_type::ret_type;
    dbn_type m_dbn;    // 定义DBN模型

public:
    void train(const std::vector<raw_data_type>& vec_data, const int& i_pretrain_times = 100, const int& i_finetune_times = 100)
    {
        std::vector<input_type> vec_input;
        vec_input.resize(vec_data.size());
        for (int idx = 0; idx < vec_data.size(); ++idx)
        {
            auto&& data = vec_data[idx];
            vec_input[idx] = local_trans_t::trans_data_type(data);    // 将RSI和盘口数据拼接
        }
        m_dbn.pretrain(vec_input, i_pretrain_times);    // 预训练
        // 获得期望值，对DBN进行微调
        std::vector<ret_type> vec_expect;
        vec_expect.resize(vec_data.size());
        for (int idx = 0; idx < vec_data.size(); ++idx)
        {
            auto& data = vec_data[idx];
            auto& mt_expect = vec_expect[idx];
            mt_expect = 0.0;
            //mt_expect.get(data.label, 0) = 1.0;    // 10分钟后的价格标签
            for (int i = 0; i < output_num; ++i)
            {
                int label = data.labels[i];
                mt_expect.get(label, i) = 1.0;    // 设置标签位置为1.0
            }
        }
        m_dbn.finetune(vec_expect, i_finetune_times);    // 微调
    }

    // 获取最大值的索引
    static int get_max_index(const mat<200, 1, double>& mt_out, double& d_poss)
    {
        int idx = 0;
        d_poss = mt_out.get(0, 0);
        for (int i = 1; i < 200; ++i)
        {
            if (mt_out.get(i, 0) > d_poss)
            {
                d_poss = mt_out.get(i, 0);
                idx = i;
            }
        }
        return idx;
    }

    void predict(const raw_data_type& raw_data, std::vector<predict_result>& vec_result)
    {
        input_type data = local_trans_t::trans_data_type(raw_data);    // 将RSI和盘口数据拼接
        auto mt_out = m_dbn.forward(data);    // 直接前向传播
        for (int c = 0; c < output_num; ++c)
        {
            predict_result result;
            result.idx = get_max_index(mt_out.col(c), result.d_poss);    // 获取最大值的索引和概率
            vec_result.push_back(result);    // 将结果添加到结果向量中
        }
    }
};


#endif
