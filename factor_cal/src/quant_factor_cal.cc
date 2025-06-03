#include "quant_factor_cal.hh"
#include <ta_libc.h>
#include <cmath>  // for NAN
#include <stdexcept>
#include <assert.h>

QuantFactoeCal::QuantFactoeCal() {
    TA_Initialize();
}

QuantFactoeCal::~QuantFactoeCal() {
    TA_Shutdown();
}

double QuantFactoeCal::ComputeRSI(const std::vector<double>& data, int period) {
  //assert(data.size() == period);

  int outBeg = 0, outNb = 0;
  std::vector<double> out(data.size(), NAN);
  double output = 0.0;
  TA_RetCode retCode = TA_RSI(0, data.size() - 1,
                              data.data(),
                              period,
                              &outBeg, &outNb,
                              &output);

  if (retCode != TA_SUCCESS) {
      throw std::runtime_error("TA_RSI failed.");
  }

  return output;
}

std::tuple<double, double, double> QuantFactoeCal::ComputeMACD(const std::vector<double>& data, int fastPeriod, int slowPeriod, int signalPeriod) {
    int outBeg = 0, outNb = 0;
    double dif = 0, dea = 0, macd = 0;

    TA_RetCode retCode = TA_MACD(
        0, data.size() - 1,
        data.data(),
        fastPeriod, slowPeriod, signalPeriod,
        &outBeg, &outNb,
        &dif,
        &dea,
        &macd
    );

    if (retCode != TA_SUCCESS) {
        throw std::runtime_error("TA_MACD failed.");
    }

    return std::make_tuple(dif, dea, macd);
}
