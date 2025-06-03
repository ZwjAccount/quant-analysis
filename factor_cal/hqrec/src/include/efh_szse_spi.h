#ifndef EFH_SZSE_SPI_H_
#define EFH_SZSE_SPI_H_

#include <iostream>
#include <thread>
#include "message.hh"
#include <sze_hpf_define.h>
#include <efh_lev2_define.h>
#include <i_efh_sze_lev2_api.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

typedef void* dll_handle_sl;

using namespace std;
using namespace message;
class EfhSzseSpi : public efh_sze_lev2_api_event, public efh_sze_lev2_api_depend {
 public:
  EfhSzseSpi();
  ~EfhSzseSpi();
  bool init(efh_channel_config* param, int num);
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
  virtual void on_report_efh_sze_lev2_after_close(session_identity id, sze_hpf_after_close* p_after_close);
  virtual void on_report_efh_sze_lev2_snap(session_identity id, sze_hpf_lev2* p_snap);
  virtual void on_report_efh_sze_lev2_tick(session_identity id, int msg_type, sze_hpf_order* p_order, sze_hpf_exe* p_exe);
  virtual void on_report_efh_sze_lev2_idx(session_identity id, sze_hpf_idx* p_idx);
  virtual void on_report_efh_sze_lev2_tree(session_identity id, sze_hpf_tree* p_tree);
  virtual void on_report_efh_sze_lev2_ibr_tree(session_identity id, sze_hpf_ibr_tree* p_ibr_tree);
  virtual void on_report_efh_sze_lev2_turnover(session_identity id, sze_hpf_turnover* p_turnover);
  virtual void on_report_efh_sze_lev2_bond_snap(session_identity id, sze_hpf_bond_snap* p_bond_snap);
  virtual void on_report_efh_sze_lev2_bond_tick(session_identity id, int msg_type, sze_hpf_bond_order* p_order, sze_hpf_bond_exe* p_exe);

  virtual void efh_sze_lev2_debug(const char* msg, int len);
  virtual void efh_sze_lev2_error(const char* msg, int len);
  virtual void efh_sze_lev2_info(const char* msg, int len);

  string format_str(const char* pFormat, ...);
  void get_sze_src_trading_phase_code(char ch_sl_trading_phase_code, char* trading_phase_code);
  
  void DataConvert(message::Snap* dst, sze_hpf_lev2* src);
  void DataConvert(message::Index* dst, sze_hpf_idx* src);
  void DataConvert(message::Snap* dst, sze_hpf_bond_snap* src);




private:
    dll_handle_sl            m_h_core_;
    i_efh_sze_lev2_api*      m_p_quote_;
    
    std::shared_ptr<spdlog::logger> sz_sys_logger_;
};







#endif // EFH_SZSE_SPI_H_