#include "dbn_t.hpp"
#include "model_struct.h"
#include "proxy_dbn_t.hpp"

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
        model.finetune<cross_entropy>(ret); // 使用DBN模型进行反向传播
    }
};


int main(int argc, char* argv[])
{
    using raw_data_type = market_data<30, 5>; //
    proxy_dbn_t<all_idx, raw_data_type, 5> model; // 定义模型
    std::vector<raw_data_type> train_data;
    model.train(train_data, 100, 100); // 训练模型
    raw_data_type test_data; // 测试数据
    std::vector<predict_result> results;
    model.predict(test_data, results); // 进行预测
    for (const auto& result : results)
    {
        std::cout << "Predicted index: " << result.idx << ", Probability: " << result.d_poss << std::endl;
    }

    return 0;
}