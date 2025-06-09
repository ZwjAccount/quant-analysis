#include "dbn_t.hpp"
#include "model_struct.h"

struct net_t
{
    template<int ih>
    using predict_t = predict_net_t<5, double, ih, 200, 200, 200>;
    using dbn_type = dbn_t<predict_t, double, 200, 300, 400>;

    using input_type = typename dbn_type::input_type;
    using ret_type = typename dbn_type::ret_type;

    dbn_type model; // 定义DBN模型
    ret_type predict(const input_type& input)
    {
        return model.forward(input); // 使用DBN模型进行预测
    }

    void backward(const std::vector<ret_type>& ret)
    {
        model.finetune(ret); // 使用DBN模型进行反向传播
    }
};

int main(int argc, char* argv[])
{
    net_t nt;
    net_t::input_type input;
    // 假设input已经被填充了数据
    auto result = nt.predict(input); // 调用预测函数
    std::vector<net_t::ret_type> ret;
    // 假设ret已经被填充了数据
    nt.backward(ret); // 调用反向传播函数

    return 0;
}