#ifndef EFH_SSE_SPI_H_
#define EFH_SSE_SPI_H_

#include <iostream>
#include <thread>

#include "message.hh"
#include <sse_hpf_define.h>
#include <efh_lev2_define.h>
#include <i_efh_sse_lev2_api.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"


typedef void* dll_handle_sl;

using namespace std;
using namespace message;
class EfhSseSpi : public efh_sse_lev2_api_event, public efh_sse_lev2_api_depend {
public:
  EfhSseSpi();
  ~EfhSseSpi();
  bool init(efh_channel_config* p_param, int num);
  bool init_with_ats(
    exchange_authorize_config& ats_config,
    efh_channel_config*        param,
    int                        num,
    bool                       is_keep_connection,
    int                        use_ate_report_multicast
  );
  void run();
  void close();

protected:
  virtual void on_report_efh_sse_lev2_idx(session_identity id, sse_hpf_idx* p_idx);
  virtual void on_report_efh_sse_lev2_snap(session_identity id, sse_hpf_lev2* p_snap);
  virtual void on_report_efh_sse_lev2_option(session_identity id, sse_hpf_stock_option* p_opt);
  virtual void on_report_efh_sse_lev2_tree(session_identity id, sse_hpf_tree* p_tree);
  virtual void on_report_efh_sse_lev2_tick(session_identity id, int msg_type, sse_hpf_order* p_order, sse_hpf_exe* p_exe);
  virtual void on_report_efh_sse_lev2_bond_snap(session_identity id, sse_hpf_bond_snap* p_bond_snap);
  virtual void on_report_efh_sse_lev2_bond_tick(session_identity id, sse_hpf_bond_tick* p_bond_tick);
  virtual void on_report_efh_sse_lev2_tick_merge(session_identity id, sse_hpf_tick_merge* p_tick);
  virtual void on_report_efh_sse_lev2_etf(session_identity id, sse_hpf_etf* p_etf);
  virtual void on_report_efh_sse_lev2_bond_tree(session_identity id, sse_hpf_bond_tree* p_bond_tree);

  virtual void efh_sse_lev2_debug(const char* msg, int len);
  virtual void efh_sse_lev2_error(const char* msg, int len);
  virtual void efh_sse_lev2_info(const char* msg, int len);

  void DataConvert(Snap* dst, sse_hpf_lev2* src);
  void DataConvert(Snap* dst, sse_hpf_bond_snap* src);
  void DataConvert(Index* dst, sse_hpf_idx* src);
  

 private:
  string format_str(const char* pFormat, ...);
  void get_sse_src_trading_phase_code(char ch_sl_trading_phase_code, char* trading_phase_code);
  void get_sse_src_instrument_status(char ch_sl_instrument_status, char* instrument_status);
  string get_sse_bond_trading_phase_code_by_instrument_status(char ch_sl_instrument_status);

 private:
  dll_handle_sl       m_h_core_;
  i_efh_sse_lev2_api* m_p_quote_;
 
  std::shared_ptr<spdlog::logger> sh_sys_logger_;
};






#endif // EFH_SSE_SPI_H_