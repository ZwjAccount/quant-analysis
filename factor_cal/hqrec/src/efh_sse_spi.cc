#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sstream>
#include <cstdarg>

#ifndef WINDOWS
#include <dlfcn.h>
#include <arpa/inet.h>
#endif

#include "efh_sse_spi.h"

EfhSseSpi::EfhSseSpi() 
 : m_h_core_(nullptr), 
   m_p_quote_(nullptr) {}

EfhSseSpi::~EfhSseSpi() {}

bool EfhSseSpi::init(efh_channel_config* p_param, int num) {
	const char* err_address = "operation::init:";

	m_h_core_ = dlopen(DLL_EFH_LEV2_DLL_NAME, RTLD_LAZY);
	if (m_h_core_ == nullptr)
	{
		string msg = format_str("%s init：load dll:%s error：%s!\n", err_address, DLL_EFH_LEV2_DLL_NAME, dlerror());
		efh_sse_lev2_error(msg.c_str(), msg.length());
		return false;
	}

	func_create_efh_sse_lev2_api func_create = (func_create_efh_sse_lev2_api)dlsym(m_h_core_, CREATE_EFH_SSE_LEV2_API_FUNCTION);
	if (func_create == nullptr)
	{
		string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
		efh_sse_lev2_error(msg.c_str(), msg.length());
		return false;
	}

	m_p_quote_ = func_create();
	if (m_p_quote_ == nullptr)
	{
		string msg = format_str("%s create sqs function ptr null.\n", err_address);
		efh_sse_lev2_error(msg.c_str(), msg.length());
		return false;
	}
  m_p_quote_->set_channel_config(p_param, num);
	if (!m_p_quote_->init_sse(static_cast<efh_sse_lev2_api_event*>(this), static_cast<efh_sse_lev2_api_depend*>(this)))
	{
		string msg = format_str("%s init parse! error\n", err_address);
		efh_sse_lev2_error(msg.c_str(), msg.length());
		return false;
	}

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm *tmp = localtime(&ts.tv_sec);
  char day[12] = {0};
  snprintf(day, 12, "%04d%02d%02d", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday);
  std::string m_day = day;
  std::string filename = m_day + "_sh_sys.log";
  sh_sys_logger_ = spdlog::basic_logger_mt("sh_sys_logger", filename);

	return true;
}

bool EfhSseSpi::init_with_ats(
  exchange_authorize_config& ats_config,
  efh_channel_config*        param,
  int                        num,
  bool                       is_keep_connection,
  int                        use_ate_report_multicast) {

  const char* err_address = "operation::init: ";
  m_h_core_ = dlopen(DLL_EFH_LEV2_DLL_NAME, RTLD_LAZY);
  if (m_h_core_ == nullptr) {
    string msg = format_str("%s init：load dll:%s error:%s!\n", err_address, DLL_EFH_LEV2_DLL_NAME,dlerror());
    efh_sse_lev2_error(msg.c_str(), msg.length());
    return false;
  }

  func_create_efh_sse_lev2_api func_create =
      (func_create_efh_sse_lev2_api)dlsym(m_h_core_, CREATE_EFH_SSE_LEV2_API_FUNCTION);
  if (func_create == nullptr) {
    string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
    efh_sse_lev2_error(msg.c_str(), msg.length());
    return false;
  }

  m_p_quote_ = func_create();
  if (m_p_quote_ == nullptr) {
    string msg = format_str("%s create sqs function ptr null.\n", err_address);
    efh_sse_lev2_error(msg.c_str(), msg.length());
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

  if (!m_p_quote_->init_sse(static_cast<efh_sse_lev2_api_event*>(this), static_cast<efh_sse_lev2_api_depend*>(this))) {
		string msg = format_str("%s init parse! error\n", err_address);
		efh_sse_lev2_error(msg.c_str(), msg.length());
		return false;
  }
  
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm *tmp = localtime(&ts.tv_sec);
  char day[12] = {0};
  snprintf(day, 12, "%04d%02d%02d", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday);
  std::string m_day = day;
  std::string filename = m_day + "_sh_sys.log";
  sh_sys_logger_ = spdlog::basic_logger_mt("sh_sys_logger", filename);

  return true;
}


void EfhSseSpi::run() {
  if (!m_p_quote_->start_sse()) {
    string msg = format_str( "start parse error\n" );
    efh_sse_lev2_error( msg.c_str( ) , msg.length( ) );
  }
}

void EfhSseSpi::close() {
	if (m_p_quote_ == NULL) {
		return;
	}

	m_p_quote_->stop_sse();
	m_p_quote_->close_sse();

	func_destroy_efh_sse_lev2_api func_destroy = (func_destroy_efh_sse_lev2_api)dlsym(m_h_core_, DESTROY_EFH_SSE_LEV2_API_FUNCTION);

	if (func_destroy == NULL) {
		return;
	}

	func_destroy(m_p_quote_);
	dlclose(m_h_core_);
}

void EfhSseSpi::DataConvert(Snap* dst, sse_hpf_lev2* src) {
  dst->message_type = MessageType::SNAP;
  dst->time_stamp = src->m_quote_update_time;
  dst->exchange_id = src->m_head.m_exchange_id;

  memcpy(dst->security_id, src->m_symbol, sizeof(src->m_symbol));
  strcat(dst->security_id, sh_suffix);

  dst->num_trades = src->m_total_trade_num;
  dst->total_volume_trade = (src->m_total_quantity / 1000);
  dst->total_value_trade = (src->m_total_value / 100000);

  dst->pre_close_price = (src->m_pre_close_price / 1000);
  dst->last_price = (src->m_last_price / 1000);
  dst->open_price = (src->m_open_price / 1000);
  dst->high_price = (src->m_day_high_price / 1000);
  dst->low_price = (src->m_day_low_price / 1000);
  dst->close_price = (src->m_today_close_price / 1000);

  dst->bid_weighted_avg_price = (src->m_total_bid_weighted_avg_price / 1000);
  dst->bid_total_qty = (src->m_total_bid_quantity / 1000);
  dst->ask_weighted_avg_price = (src->m_total_ask_weighted_avg_price / 10000);
  dst->ask_total_qty = (src->m_total_ask_quantity / 1000);
  

  for (int i = 0; i < 10; i++) {
    dst->bid_info[i].price = (src->m_bid_unit[i].m_price / 10000);
    dst->bid_info[i].qty = (src->m_bid_unit[i].m_quantity / 100);

    dst->ask_info[i].price = (src->m_ask_unit[i].m_price / 10000);
    dst->ask_info[i].qty = (src->m_ask_unit[i].m_quantity / 100);
  }  
}

void EfhSseSpi::DataConvert(Snap* dst, sse_hpf_bond_snap* src) {
  dst->message_type = MessageType::SNAP;
  dst->time_stamp = src->m_quote_update_time;
  dst->exchange_id = src->m_head.m_exchange_id;

  memcpy(dst->security_id, src->m_symbol, sizeof(src->m_symbol));
  strcat(dst->security_id, sh_suffix);

  dst->num_trades = src->m_total_trade_num;
  dst->total_volume_trade = (src->m_total_quantity / 1000);
  dst->total_value_trade = (src->m_total_value / 100000);

  dst->pre_close_price = (src->m_pre_close_price / 1000);
  dst->last_price = (src->m_last_price / 1000);
  dst->open_price = (src->m_open_price / 1000);
  dst->high_price = (src->m_day_high_price / 1000);
  dst->low_price = (src->m_day_low_price / 1000);
  dst->close_price = (src->m_today_close_price / 1000);

  dst->bid_weighted_avg_price = (src->m_total_bid_weighted_avg_price / 1000);
  dst->bid_total_qty = (src->m_total_bid_quantity / 1000);
  dst->ask_weighted_avg_price = (src->m_total_ask_weighted_avg_price / 10000);
  dst->ask_total_qty = (src->m_total_ask_quantity / 1000);
  

  for (int i = 0; i < 10; i++) {
    dst->bid_info[i].price = (src->m_bid_unit[i].m_price / 10000);
    dst->bid_info[i].qty = (src->m_bid_unit[i].m_quantity / 100);

    dst->ask_info[i].price = (src->m_ask_unit[i].m_price / 10000);
    dst->ask_info[i].qty = (src->m_ask_unit[i].m_quantity / 100);
  } 
}

void EfhSseSpi::DataConvert(Index* dst, sse_hpf_idx* src) {
  dst->message_type = MessageType::INDEX;
  dst->time_stamp = src->m_quote_update_time;
  dst->exchange_id = src->m_head.m_exchange_id;

  memcpy(dst->security_id, src->m_symbol, sizeof(src->m_symbol));
  strcat(dst->security_id, sh_suffix);

  dst->total_volume_trade = (src->m_total_quantity / 100000);
  dst->total_value_trade = (src->m_total_value / 10);

  dst->pre_close_price = (src->m_pre_close_price / 100000);
  dst->last_price = (src->m_last_price / 100000);
  dst->open_price = (src->m_open_price / 100000);
  dst->high_price = (src->m_day_high_price / 100000);
  dst->low_price = (src->m_day_low_price / 100000);
  dst->close_price = (src->m_today_close_price / 100000);
}


void EfhSseSpi::on_report_efh_sse_lev2_idx(session_identity id, sse_hpf_idx* p_idx) {

  Index sh_index;
  DataConvert(&sh_index, p_idx);

  if (index_spsc_queue.try_push(sh_index) == false) {
    std::ostringstream oss;
    oss << "sheng li sh_index_spsc_queue is full" 
        << ", stock_code:" << p_idx->m_symbol 
        << ", timestamp:" << p_idx->m_quote_update_time;
    sh_sys_logger_->error(oss.str());
  }
}

void EfhSseSpi::on_report_efh_sse_lev2_snap(session_identity id, sse_hpf_lev2* p_snap) {

  Snap sh_snap;
  DataConvert(&sh_snap, p_snap);

  if (snap_spsc_queue.try_push(sh_snap) == false ) {
    std::ostringstream oss;
    oss << "sheng li sh_snap_spsc_queue is full" 
        << ", stock_code:" << p_snap->m_symbol 
        << ", timestamp:" << p_snap->m_quote_update_time;
    sh_sys_logger_->error(oss.str());
  }
}

void EfhSseSpi::on_report_efh_sse_lev2_bond_snap(session_identity id, sse_hpf_bond_snap* p_bond_snap) {

  Snap sh_bond_snap;
  DataConvert(&sh_bond_snap, p_bond_snap);

  if (snap_spsc_queue.try_push(sh_bond_snap) == false) {
    std::ostringstream oss;
    oss << "sheng li sh_bond_snap_spsc_queue is full" 
        << ", stock_code:" << p_bond_snap->m_symbol 
        << ", timestamp:" << p_bond_snap->m_quote_update_time;
    sh_sys_logger_->error(oss.str());
  }
}

void EfhSseSpi::on_report_efh_sse_lev2_bond_tick(session_identity id, sse_hpf_bond_tick* p_bond_tick) {}

void EfhSseSpi::on_report_efh_sse_lev2_tick_merge(session_identity id, sse_hpf_tick_merge* p_tick) {}

void EfhSseSpi::on_report_efh_sse_lev2_option(session_identity id, sse_hpf_stock_option* p_opt) {}

void EfhSseSpi::on_report_efh_sse_lev2_tree(session_identity id, sse_hpf_tree* p_tree) {}

void EfhSseSpi::on_report_efh_sse_lev2_etf(session_identity id, sse_hpf_etf* p_etf) {}

void EfhSseSpi::on_report_efh_sse_lev2_bond_tree(session_identity id, sse_hpf_bond_tree* p_bond_tree) {}

void EfhSseSpi::on_report_efh_sse_lev2_tick(session_identity id, int msg_type, sse_hpf_order* p_order, sse_hpf_exe* p_exe) {}

void EfhSseSpi::efh_sse_lev2_debug(const char* msg, int len)
{
	printf("[DEBUG] %s\n", msg);
}

void EfhSseSpi::efh_sse_lev2_error(const char* msg, int len)
{
	printf("[ERROR] %s\n", msg);
}

void EfhSseSpi::efh_sse_lev2_info(const char* msg, int len)
{
	printf("[INFO] %s\n", msg);
}


string EfhSseSpi::format_str(const char* pFormat, ...)
{
	va_list args;
	va_start(args, pFormat);
	char buffer[40960];
	vsnprintf(buffer, 40960, pFormat, args);
	va_end(args);
	return string(buffer);
}

void EfhSseSpi::get_sse_src_trading_phase_code(char ch_sl_trading_phase_code, char* trading_phase_code) {
	switch (ch_sl_trading_phase_code & 0xF0)
	{
    case 0x00:
      trading_phase_code[0] = 'S';
      break;
    case 0x10:
      trading_phase_code[0] = 'C';
      break;
    case 0x20:
      trading_phase_code[0] = 'T';
      break;
    case 0x30:
      trading_phase_code[0] = 'E';
      break;
    case 0x40:
      trading_phase_code[0] = 'P';
      break;
    case 0x50:
      trading_phase_code[0] = 'M';
      break;
    case 0x60:
      trading_phase_code[0] = 'N';
      break;
    case 0x70:
      trading_phase_code[0] = 'U';
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

	if ((ch_sl_trading_phase_code & 0x04) == 0x04) {
		trading_phase_code[2] = '1';
	} else if ((ch_sl_trading_phase_code & 0x04) == 0x00) {
		trading_phase_code[2] = '0';
	}

	if ((ch_sl_trading_phase_code & 0x02) == 0x02) {
		trading_phase_code[3] = '1';
	} else if ((ch_sl_trading_phase_code & 0x02) == 0x00) {
		trading_phase_code[3] = '0';
	}
}

void EfhSseSpi::get_sse_src_instrument_status(char ch_sl_instrument_status, char* instrument_status) {

	switch (ch_sl_instrument_status)
	{
    case 0:
      strcpy(instrument_status, " ");
      break;
    case 1:
      strcpy(instrument_status, "START");
      break;
    case 2:
      strcpy(instrument_status, "OCALL");
      break;
    case 3:
      strcpy(instrument_status, "TRADE");
      break;
    case 4:
      strcpy(instrument_status, "SUSP");
      break;
    case 5:
      strcpy(instrument_status, "CCALL");
      break;
    case 6:
      strcpy(instrument_status, "CLOSE");
      break;
    case 7:
      strcpy(instrument_status, "ENDTR");
      break;
    case 8:
      strcpy(instrument_status,"ADD");
      break;
    default:
      strcpy(instrument_status," ");
      break;
	}
}

string EfhSseSpi::get_sse_bond_trading_phase_code_by_instrument_status(char ch_sl_instrument_status)
{
	char	ch_buf[9];
	memset(ch_buf, 0, sizeof(ch_buf));
	switch (ch_sl_instrument_status)
	{
	case 0:
		strcpy(ch_buf, "        ");
		break;
	case 1:
		strcpy(ch_buf, "S       ");
		break;
	case 2:
		strcpy(ch_buf, "C       ");
		break;
	case 3:
		strcpy(ch_buf, "T       ");
		break;
	case 4:
		strcpy(ch_buf, "P       ");
		break;
	case 6:
		strcpy(ch_buf, "E       ");
		break;
	case 7:
		strcpy(ch_buf, "E       ");
		break;
	case 8:
		strcpy(ch_buf, "        ");
		break;
	default:
		strcpy(ch_buf, "        ");
		break;
	}

	string str_buf = ch_buf;
	return str_buf;
}
