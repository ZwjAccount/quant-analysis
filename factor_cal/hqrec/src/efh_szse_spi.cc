#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sstream>
#include <cstdarg>

#ifndef WINDOWS
#include <dlfcn.h>
#include <arpa/inet.h>
#endif

#include "efh_szse_spi.h"


EfhSzseSpi::EfhSzseSpi() 
 : m_h_core_(nullptr), 
   m_p_quote_(nullptr) {}

EfhSzseSpi::~EfhSzseSpi() {}

bool EfhSzseSpi::init(efh_channel_config* param, int num) {
  
	const char* err_address = "operation::init：";
  
	m_h_core_ = dlopen(DLL_EFH_LEV2_DLL_NAME, RTLD_LAZY);
	if (m_h_core_ == nullptr) {
		string msg = format_str("%s init：load dll:%s error:%s!\n", err_address, DLL_EFH_LEV2_DLL_NAME,dlerror());
		efh_sze_lev2_error(msg.c_str(), msg.length());
		return false;
	}

	func_create_efh_sze_lev2_api func_create = (func_create_efh_sze_lev2_api)dlsym(m_h_core_, CREATE_EFH_SZE_LEV2_API_FUNCTION);
	if (func_create == nullptr) {
		string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
		efh_sze_lev2_error(msg.c_str(), msg.length());
		return false;
	}

	m_p_quote_ = func_create();
  std::cout << "m_p_quote_" << m_p_quote_ << std::endl;
	if (m_p_quote_ == nullptr) {
		string msg = format_str("%s create sqs function ptr null.\n", err_address);
		efh_sze_lev2_error(msg.c_str(), msg.length());
		return false;
	}
  
  m_p_quote_->set_channel_config(param, num);
	if (!m_p_quote_->init_sze(static_cast<efh_sze_lev2_api_event*>(this), static_cast<efh_sze_lev2_api_depend*>(this)))
	{
		string msg = format_str("%s init parse! error\n", err_address);
		efh_sze_lev2_error(msg.c_str(), msg.length());
		return false;
	}

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm *tmp = localtime(&ts.tv_sec);
  char day[12] = {0};
  snprintf(day, 12, "%04d%02d%02d", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday);
  std::string m_day = day;
  std::string filename = m_day + "_sz_sys.log";
  sz_sys_logger_ = spdlog::basic_logger_mt("sz_sys_logger", filename);

	return true;
}

bool EfhSzseSpi::init_with_ats(
  exchange_authorize_config& ats_config,
  efh_channel_config*        param,
  int                        num,
  bool                       is_keep_connection,
  int                        use_ate_report_multicast) {

  const char* err_address = "operation::init: ";
  m_h_core_ = dlopen(DLL_EFH_LEV2_DLL_NAME, RTLD_LAZY);
  if (m_h_core_ == nullptr) {
    string msg = format_str("%s init：load dll:%s error:%s!\n", err_address, DLL_EFH_LEV2_DLL_NAME,dlerror());
    efh_sze_lev2_error(msg.c_str(), msg.length());
    return false;
  }

  func_create_efh_sze_lev2_api func_create =
      (func_create_efh_sze_lev2_api)dlsym(m_h_core_, CREATE_EFH_SZE_LEV2_API_FUNCTION);
  if (func_create == nullptr) {
    string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
    efh_sze_lev2_error(msg.c_str(), msg.length());
    return false;
  }

  m_p_quote_ = func_create();
  if (m_p_quote_ == nullptr) {
    string msg = format_str("%s create sqs function ptr null.\n", err_address);
    efh_sze_lev2_error(msg.c_str(), msg.length());
    return false;
  }

  efh_channel_config* p_old = new efh_channel_config[num];
  memcpy(p_old, param, num * sizeof(efh_channel_config));
  if (!m_p_quote_->set_channel_config_with_ats(param, num, ats_config, is_keep_connection)) {
    return false;
  }

  if (0 == use_ate_report_multicast) {
    m_p_quote_->set_channel_config(p_old, num);
  }
  delete[] p_old;

  if (!m_p_quote_->init_sze(static_cast<efh_sze_lev2_api_event*>(this), static_cast<efh_sze_lev2_api_depend*>(this))) {
		string msg = format_str("%s init parse! error\n", err_address);
		efh_sze_lev2_error(msg.c_str(), msg.length());
		return false;
  }
  
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm *tmp = localtime(&ts.tv_sec);
  char day[12] = {0};
  snprintf(day, 12, "%04d%02d%02d", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday);
  std::string m_day = day;
  std::string filename = m_day + "_sz_sys.log";
  sz_sys_logger_ = spdlog::basic_logger_mt("sz_sys_logger", filename);

  return true;
}

void EfhSzseSpi::run() {
  std::cout << "in szse run" << std::endl;
  if (!m_p_quote_->start_sze()) {
    string msg = format_str( "start parse error\n" );
    efh_sze_lev2_error( msg.c_str( ) , msg.length( ) );
  }
  std::cout << "in szse run over" << std::endl;
}

void EfhSzseSpi::close() {
	if (m_p_quote_ == nullptr) {
		return;
	}

	m_p_quote_->stop_sze();
	m_p_quote_->close_sze();

	func_destroy_efh_sze_lev2_api func_destroy = (func_destroy_efh_sze_lev2_api)dlsym(m_h_core_, DESTROY_EFH_SZE_LEV2_API_FUNCTION);

	if (func_destroy == nullptr) {
		return;
	}

	func_destroy(m_p_quote_);
	dlclose(m_h_core_);
}


void EfhSzseSpi::on_report_efh_sze_lev2_snap(session_identity id, sze_hpf_lev2* p_snap) {
  Snap sz_snap; 
  DataConvert(&sz_snap, p_snap); 
  if (snap_spsc_queue.try_push(sz_snap) == false) {
    std::ostringstream oss;
    oss << "on_report_efh_sze_lev2_snap snap_spsc_queue is full" 
        << ", stock_code:" << p_snap->m_head.m_symbol 
        << ", timestamp:" << p_snap->m_head.m_quote_update_time;
    sz_sys_logger_->error(oss.str());
  }
}


void EfhSzseSpi::on_report_efh_sze_lev2_idx(session_identity id, sze_hpf_idx* p_idx) {

  Index sz_index;
  DataConvert(&sz_index, p_idx);

  if (index_spsc_queue.try_push(sz_index) == false) {
    std::ostringstream oss;
    oss << "index_spsc_queue is full" 
        << ", stock_code:" << p_idx->m_head.m_symbol 
        << ", timestamp:" << p_idx->m_head.m_quote_update_time;
    sz_sys_logger_->error(oss.str());
  }
}

void EfhSzseSpi::on_report_efh_sze_lev2_bond_snap(session_identity id, sze_hpf_bond_snap* p_bond_snap) {

  Snap sz_bond_snap;
  DataConvert(&sz_bond_snap, p_bond_snap);

  if (snap_spsc_queue.try_push(sz_bond_snap) == false) {
    std::ostringstream oss;
    oss << "on_report_efh_sze_lev2_bond_snap snap_spsc_queue is full" 
        << ", stock_code:" << p_bond_snap->m_head.m_symbol 
        << ", timestamp:" << p_bond_snap->m_head.m_quote_update_time;
    sz_sys_logger_->error(oss.str());
  }
}

void EfhSzseSpi::on_report_efh_sze_lev2_bond_tick(session_identity id, int msg_type, sze_hpf_bond_order* p_order, sze_hpf_bond_exe* p_exe) {}
void EfhSzseSpi::on_report_efh_sze_lev2_after_close(session_identity id, sze_hpf_after_close* p_after_close) {}
void EfhSzseSpi::on_report_efh_sze_lev2_tree(session_identity id, sze_hpf_tree* p_tree) {}
void EfhSzseSpi::on_report_efh_sze_lev2_ibr_tree(session_identity id, sze_hpf_ibr_tree* p_ibr_tree) {}
void EfhSzseSpi::on_report_efh_sze_lev2_turnover(session_identity id, sze_hpf_turnover* p_turnover) {}
void EfhSzseSpi::on_report_efh_sze_lev2_tick(session_identity id, int msg_type, sze_hpf_order* p_order, sze_hpf_exe* p_exe) {}


void EfhSzseSpi::DataConvert(message::Snap* dst, sze_hpf_lev2* src) {

  dst->message_type = MessageType::SNAP;
  dst->time_stamp = src->m_head.m_quote_update_time;
  dst->channel_no = src->m_head.m_channel_num;
  dst->exchange_id = src->m_head.m_exchange_id;

  memcpy(dst->security_id, src->m_head.m_symbol, sizeof(src->m_head.m_symbol));
  strcat(dst->security_id, sz_suffix);

  dst->num_trades = src->m_total_trade_num;
  dst->total_volume_trade = (src->m_total_quantity / 100);
  dst->total_value_trade = (src->m_total_value / 1000000.0);

  dst->pre_close_price = (src->m_pre_close_price / 10000.0);
  dst->last_price = (src->m_last_price / 10000.0);
  dst->open_price = (src->m_open_price / 10000.0);
  dst->high_price = (src->m_day_high_price / 10000.0);
  dst->low_price = (src->m_day_low_price / 10000.0);
  dst->close_price = (src->m_today_close_price / 10000.0);

  dst->bid_weighted_avg_price = (src->m_total_bid_weighted_avg_price / 1000000.0);
  dst->bid_total_qty = (src->m_total_bid_quantity / 100);
  dst->ask_weighted_avg_price = (src->m_total_ask_weighted_avg_price / 1000000.0);
  dst->ask_total_qty = (src->m_total_ask_quantity / 100);
  

  for (int i = 0; i < 10; i++) {
    dst->bid_info[i].price = (src->m_bid_unit[i].m_price / 10000.0);
    dst->bid_info[i].qty = (src->m_bid_unit[i].m_quantity / 100);

    dst->ask_info[i].price = (src->m_ask_unit[i].m_price / 10000.0);
    dst->ask_info[i].qty = (src->m_ask_unit[i].m_quantity / 100);
  }  
}

void EfhSzseSpi::DataConvert(message::Index* dst, sze_hpf_idx* src) {
  dst->message_type = MessageType::INDEX;
  dst->time_stamp = src->m_head.m_quote_update_time;
  dst->channel_no = src->m_head.m_channel_num;
  dst->exchange_id = src->m_head.m_exchange_id;

  memcpy(dst->security_id, src->m_head.m_symbol, sizeof(src->m_head.m_symbol));
  strcat(dst->security_id, sz_suffix);

  
  dst->num_trades = src->m_total_trade_num;
  dst->total_volume_trade = (src->m_total_quantity / 100);
  dst->total_value_trade = (src->m_total_value / 1000000);

  dst->pre_close_price = (src->m_pre_close_price / 10000);
  dst->last_price = (src->m_last_price / 10000);
  dst->open_price = (src->m_open_price / 10000);
  dst->high_price = (src->m_day_high_price / 10000);
  dst->low_price = (src->m_day_low_price / 10000);
  dst->close_price = (src->m_today_close_price / 10000);
}

void EfhSzseSpi::DataConvert(message::Snap* dst, sze_hpf_bond_snap* src) {
  dst->message_type = MessageType::SNAP;
  dst->time_stamp = src->m_head.m_quote_update_time;
  dst->channel_no = src->m_head.m_channel_num;
  dst->exchange_id = src->m_head.m_exchange_id;

  memcpy(dst->security_id, src->m_head.m_symbol, sizeof(src->m_head.m_symbol));
  strcat(dst->security_id, sz_suffix);

  dst->num_trades = src->m_total_trade_num;
  dst->total_volume_trade = (src->m_total_quantity / 100);
  dst->total_value_trade = (src->m_total_value / 1000000);

  dst->pre_close_price = (src->m_pre_close_price / 10000);
  dst->last_price = (src->m_last_price / 10000);
  dst->open_price = (src->m_open_price / 10000);
  dst->high_price = (src->m_day_high_price / 10000);
  dst->low_price = (src->m_day_low_price / 10000);
  dst->close_price = (src->m_today_close_price / 10000);

  dst->bid_weighted_avg_price = (src->m_total_bid_weighted_avg_price / 1000000);
  dst->bid_total_qty = (src->m_total_bid_quantity / 100);
  dst->ask_weighted_avg_price = (src->m_total_ask_weighted_avg_price / 1000000);
  dst->ask_total_qty = (src->m_total_ask_quantity / 100);
  

  for (int i = 0; i < 10; i++) {
    dst->bid_info[i].price = (src->m_bid_unit[i].m_price / 10000);
    dst->bid_info[i].qty = (src->m_bid_unit[i].m_quantity / 100);

    dst->ask_info[i].price = (src->m_ask_unit[i].m_price / 10000);
    dst->ask_info[i].qty = (src->m_ask_unit[i].m_quantity / 100);
  }  
}

void EfhSzseSpi::efh_sze_lev2_debug(const char* msg, int len) {
	printf("[DEBUG] %s\n",msg);
}

void EfhSzseSpi::efh_sze_lev2_error(const char* msg, int len) {
	printf("[ERROR] %s\n", msg);
}

void EfhSzseSpi::efh_sze_lev2_info(const char* msg, int len) {
	printf("[INFO] %s\n", msg);
}


string EfhSzseSpi::format_str(const char* pFormat, ...) {
	va_list args;
	va_start(args, pFormat);
	char buffer[40960];
	vsnprintf(buffer, 40960, pFormat, args);
	va_end(args);
	return string(buffer);
}


void EfhSzseSpi::get_sze_src_trading_phase_code(char ch_sl_trading_phase_code, char* trading_phase_code) {
	switch (ch_sl_trading_phase_code & 0xF0)
	{
    case 0x00:
      trading_phase_code[0] = 'S';
      break;
    case 0x10:
      trading_phase_code[0] = 'O';
      break;
    case 0x20:
      trading_phase_code[0] = 'T';
      break;
    case 0x30:
      trading_phase_code[0] = 'B';
      break;
    case 0x40:
      trading_phase_code[0] = 'C';
      break;
    case 0x50:
      trading_phase_code[0]= 'E';
      break;
    case 0x60:
      trading_phase_code[0] = 'H';
      break;
    case 0x70:
      trading_phase_code[0] = 'A';
      break;
    case 0x80:
      trading_phase_code[0] = 'V';
      break;
    default:
      break;
	}

	if ((ch_sl_trading_phase_code & 0x08) == 0x08) {
		trading_phase_code[1] = '1';
	} else if ((ch_sl_trading_phase_code & 0x08) == 0x00) {
		trading_phase_code[1] = '0';
	}
}
