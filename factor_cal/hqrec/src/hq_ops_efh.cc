#include "hq_ops_efh.h"

EfhHq::EfhHq() : szse_spi_(nullptr), sse_spi_(nullptr) {}

EfhHq::~EfhHq() {}

bool EfhHq::string_split(const char* str_src, vector<string>& str_dst, const string& str_separator) {
    if (NULL == str_src) {
        return false;
    }

    // 注意，这里选择了清空输入的字符数组
    str_dst.clear();

    size_t sep_len = str_separator.length();
    if (0 == sep_len) return false;

    size_t src_len = strlen(str_src);
    if (0 == src_len) return true;
    
		// 这种情况，认为只有一个元素而返回
    if (src_len < sep_len)   {
			str_dst.push_back(str_src);
			return true;
    }

    // 2010-08-05 Find Problem ls
    // 测试发现分割字符串，没有考虑最后一块，修改部分如下：
    //-----------------------------------------------------------
    /// add by zhou.you  review 2012/7/30
    /// 原修改不合理，应该先判断是否以分隔符结尾的。
    string srcstring = str_src;
    if (srcstring.substr(src_len - sep_len, sep_len) != str_separator)
        srcstring += str_separator;

    size_t lastIdx = 0;
    size_t idx     = srcstring.find_first_of(str_separator);
		// 这种情况，认为只有一个元素而返回
    if (idx == string::npos) {
			str_dst.push_back(str_src);
			return true;
    }

    while (idx != string::npos) {
			string strTemp = srcstring.substr(lastIdx, idx - lastIdx);  // 有可能空字符串，也作为结果加入。
			str_dst.push_back(strTemp);
			lastIdx = idx + sep_len;
			idx     = srcstring.find_first_of(str_separator, lastIdx);
    }
    return true;
}

bool EfhHq::config_sock_udp_param(efh_channel_config& quote_param, const char* section) {
	int enable = conf_parser_.ReadInt(section, "enable", 0);
	if (enable == 1) {
		char msg[4096];
		memset(msg, 0, sizeof(msg));

		quote_param.m_i_cpu_id = conf_parser_.ReadInt(section, "cpu_id", 0);

		int len = conf_parser_.ReadString(section, "multicast_ip", "", msg, IP_LEN);
		if (len) {
			vector<std::string> str_vec;
			string_split(msg, str_vec, ";");
			quote_param.m_i_channel_num = str_vec.size() > NET_CHANNEL_NUM ? NET_CHANNEL_NUM : str_vec.size();
			for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++) {
				memset(quote_param.m_channel_info[i].m_ch_src_ip, 0, sizeof(quote_param.m_channel_info[i].m_ch_src_ip));
        memcpy(quote_param.m_channel_info[i].m_ch_src_ip, str_vec[i].c_str(), str_vec[i].length());
			}
		}
    
		len = conf_parser_.ReadString(section, "multicast_port", "", msg, sizeof(msg));
    if (len) {
			vector<string> str_vec;
			string_split(msg, str_vec, ";");
			for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++) {
					quote_param.m_channel_info[i].m_i_src_port = atoi(str_vec[i].c_str());
			}
		}

		len = conf_parser_.ReadString(section, "data_ip", "", msg, sizeof(msg));
    if (len) {
			vector<string> str_vec;
			string_split(msg, str_vec, ";");
			for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++)
			{
					memset(quote_param.m_channel_info[i].m_ch_dest_ip, 0, sizeof(quote_param.m_channel_info[i].m_ch_dest_ip));
					memcpy(quote_param.m_channel_info[i].m_ch_dest_ip, str_vec[i].c_str(), str_vec[i].length());
			}
		}

		len = conf_parser_.ReadString(section, "data_port", "", msg, sizeof(msg));
		if (len) {
				vector<string> str_vec;
				string_split(msg, str_vec, ";");
				for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++) {
						quote_param.m_channel_info[i].m_i_dest_port = atoi(str_vec[i].c_str());
				}
		}

		len = conf_parser_.ReadString(section, "eth_name", "", msg, sizeof(msg));
		if (len) {
				vector<string> str_vec;
				string_split(msg, str_vec, ";");
				for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++) {
						memset(quote_param.m_channel_info[i].m_ch_eth_name, 0, sizeof(quote_param.m_channel_info[i].m_ch_eth_name));
						memcpy(quote_param.m_channel_info[i].m_ch_eth_name, str_vec[i].c_str(), str_vec[i].length());
				}
		}

		len = conf_parser_.ReadString(section, "nic_type", "", msg, sizeof(msg));
		if (len) {
				vector<string> str_vec;
				string_split(msg, str_vec, ";");
				for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++) {
						quote_param.m_channel_info[i].m_nic_type = (enum_efh_nic_type)atoi(str_vec[i].c_str());
				}
		}

		quote_param.m_ll_cache_size = conf_parser_.ReadInt(section, "cache_size", 0) * 1024ul * 1024ul;
		quote_param.m_ll_proc_data_wait_time = conf_parser_.ReadInt( section , "proc_data_wait_time" , 10 );
		quote_param.m_ll_normal_socket_rxbuf = conf_parser_.ReadInt(section, "normal_socket_rxbuf", 10) * 1024ul * 1024ul;
		quote_param.m_b_out_of_order_correction = (conf_parser_.ReadInt(section, "out_of_order_correction", 0) != 0);

		return true;
	} else {
		return false;
	}
}

exchange_authorize_config EfhHq::get_ats_config() {
	exchange_authorize_config ret;
	conf_parser_.ReadString("ATS_SERVER", "server_ip", "127.0.0.1", ret.m_ch_server_ip, sizeof(ret.m_ch_server_ip));
	ret.m_i_server_port = conf_parser_.ReadInt("ATS_SERVER", "server_port", 0);
	conf_parser_.ReadString("ATS_SERVER", "local_ip", "127.0.0.1", ret.m_ch_local_ip, sizeof(ret.m_ch_local_ip));
	ret.m_i_local_port = conf_parser_.ReadInt("ATS_SERVER", "local_port", 0);
	ret.m_dev_id       = conf_parser_.ReadInt("ATS_SERVER", "dev_id", 0);

	char buf[4096];
	memset(buf, 0, sizeof(buf));
	conf_parser_.ReadString("ATS_SERVER", "user_id", "", buf, sizeof(buf));
	sscanf(buf, "%lld", &ret.m_user_id);

	conf_parser_.ReadString("ATS_SERVER", "sys", "", ret.m_sys, sizeof(ret.m_sys));
	conf_parser_.ReadString("ATS_SERVER", "machine", "", ret.m_machine, sizeof(ret.m_machine));
	conf_parser_.ReadString("ATS_SERVER", "full_name", "", ret.m_full_name, sizeof(ret.m_full_name));
	conf_parser_.ReadString("ATS_SERVER", "user_password", "", ret.m_user_pwd, sizeof(ret.m_user_pwd));
	return ret;
}

EfhSzseSpi* EfhHq::run_sze() {
	efh_channel_config	quote_param[SZE_QUOTE_PARAM_NUM];
	int i = 0;

	if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_TICK")) {
		quote_param[i].m_efh_type = enum_efh_sze_lev2_tick;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_IDX")) {
		quote_param[i].m_efh_type = enum_efh_sze_lev2_idx;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_SNAP")) {
		quote_param[i].m_efh_type = enum_efh_sze_lev2_snap;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_AFTER_CLOSE")) {
		quote_param[i].m_efh_type = enum_efh_sze_lev2_after_close;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_TREE")) {
		quote_param[i].m_efh_type = enum_efh_sze_lev2_tree;
		i++;
	}

	if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_IBR_TREE")) {
		quote_param[i].m_efh_type = enum_efh_sze_lev2_ibr_tree;
		i++;
	}

	if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_TURNOVER")) {
		quote_param[i].m_efh_type = enum_efh_sze_lev2_turnover;
		i++;
	}

	if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_BOND_SNAP")) {
		quote_param[i].m_efh_type = enum_efh_sze_lev2_bond_snap;
		i++;
	}

	if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_BOND_TICK")) {
		quote_param[i].m_efh_type = enum_efh_sze_lev2_bond_tick;
		i++;
	}

	EfhSzseSpi* report = new EfhSzseSpi();

	int ats_enable = conf_parser_.ReadInt("ATS_SERVER", "enable", 0);

	if (ats_enable) {
		    int use_ate_report_multicast = conf_parser_.ReadInt("ATS_SERVER", "use_ate_report_multicast", 1);
        exchange_authorize_config ats_config = get_ats_config();
        bool is_keep_connect = (0 != conf_parser_.ReadInt("ATS_SERVER", "is_keep_connect", 0));
        if (!report->init_with_ats(ats_config, quote_param, i, is_keep_connect, use_ate_report_multicast)) {
          return nullptr;
        }
	} else {
	  if (!report->init(quote_param, i)) {
		  return nullptr;
	  }
	}
	
	report->run();
	std::cout << "run sze over" << std::endl;
	return report;
}

void EfhHq::release_sze(EfhSzseSpi* report) {
	report->close();
	delete report;
}

EfhSseSpi* EfhHq::run_sse() {
	efh_channel_config	quote_param[SSE_QUOTE_PARAM_NUM];
	int i = 0;

	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_TICK")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_tick;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_IDX")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_idx;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_SNAP")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_snap;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_OPTION")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_opt;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_TREE")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_tree;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_BOND_SNAP")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_bond_snap;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_BOND_TICK")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_bond_tick;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_TICK_MERGE")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_tick_merge;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_ETF")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_etf;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_BOND_TREE")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_bond_tree;
		i++;
	}
	if (config_sock_udp_param(quote_param[i], "EFH_SSE_STATIC_INFO")) {
		quote_param[i].m_efh_type = enum_efh_sse_lev2_static_info;
		i++;
	}

	EfhSseSpi* report = new EfhSseSpi();
	int ats_enable = conf_parser_.ReadInt("ATS_SERVER", "enable", 0);

	if (ats_enable) {
		    int use_ate_report_multicast = conf_parser_.ReadInt("ATS_SERVER", "use_ate_report_multicast", 1);
        exchange_authorize_config ats_config = get_ats_config();
        bool is_keep_connect = (0 != conf_parser_.ReadInt("ATS_SERVER", "is_keep_connect", 0));
        if (!report->init_with_ats(ats_config, quote_param, i, is_keep_connect, use_ate_report_multicast)) {
          return nullptr;
        }
	} else {
	  if (!report->init(quote_param, i)) {
		  return nullptr;
	  }
	}

	report->run();
	std::cout << "run sse over" << std::endl;
	return report;
}

void EfhHq::release_sse(EfhSseSpi* report) {
	report->close();
	delete report;
}

int EfhHq::InitAndStartReciveHq(std::string conf_file_path) {
  conf_parser_.Open(const_cast<char*>(conf_file_path.c_str()));

	int sse_enable = conf_parser_.ReadInt( "EFH_QUOTE_TYPE" , "enable_sse" , 0 );
	int sze_enable = conf_parser_.ReadInt( "EFH_QUOTE_TYPE" , "enable_sze" , 0 );

	if ( sse_enable == 0 && sze_enable == 0 ) {
		printf( "sheng li sze enable and sse enable is all is 0\n" );
		return -1;
	}

  if (sse_enable == 1) {
		std::cout << "run sse" << std::endl;
    sse_spi_ = run_sse();
  }
  if (sze_enable == 1) {
		std::cout << "run szse" << std::endl;
    szse_spi_ = run_sze();
  }
  return 0;
}

int EfhHq::StopRecvHq() {
  if (sse_spi_ != nullptr) {
    release_sse(sse_spi_);
  }
  if (szse_spi_ != nullptr) {
    release_sze(szse_spi_);
  }
  return 0;
}

  