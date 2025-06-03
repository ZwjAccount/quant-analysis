#ifndef TA_LIB_WRAPPER_H
#define TA_LIB_WRAPPER_H

#include <vector>
#include <tuple>


class QuantFactoeCal {
 public:
  QuantFactoeCal();
  ~QuantFactoeCal();

  //至少需要7个点才能输出一个数据
  double ComputeRSI(const std::vector<double>& data, int period);

  //最少需要34个数据点才能算出一组值，返回值第一个元素是DIF，第二个是DEA，第三个是macd
  std::tuple<double, double, double> ComputeMACD(const std::vector<double>& data, int fastPeriod, int slowPeriod, int signalPeriod);
};

#endif // TA_LIB_WRAPPER_H