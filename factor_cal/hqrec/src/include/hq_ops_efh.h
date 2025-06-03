#ifndef HQ_OPS_EFH_H_
#define HQ_OPS_EFH_H_

#include <thread>
#include <iostream>
#include <vector>


#include "efh_szse_spi.h"
#include "efh_sse_spi.h"
#include "efh_conf_profile.h"

using namespace std;

#define	 SZE_QUOTE_PARAM_NUM	(32)
#define	 SSE_QUOTE_PARAM_NUM	(32)

class EfhHq {
 public:
  EfhHq();
  ~EfhHq();
  
  bool string_split(const char* str_src, vector<string>& str_dst, const string& str_separator);
  bool config_sock_udp_param(efh_channel_config& quote_param, const char* section);
  exchange_authorize_config get_ats_config();
  EfhSzseSpi* run_sze();
  void release_sze(EfhSzseSpi* report);
  EfhSseSpi* run_sse();
  void release_sse(EfhSseSpi* report);

  int InitAndStartReciveHq(std::string conf_file_path);
  int StopRecvHq();
	
 private:
  EfhSzseSpi*  szse_spi_;
  EfhSseSpi*   sse_spi_;
  TIniFile     conf_parser_;
};
#endif //HQ_OPS_EFH_H_
