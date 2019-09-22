#pragma once

namespace  N_gLog_SPRS
{
	enum
	{
		EN_bytes_buf = 50 * 1000,
		EN_max_bytes_msg = 500,
		EN_bytes_file = 2 * 1'000'000,
		EN_bytes_fflush = 10 * 1000
	};
	constexpr char CE_str_fbody[] = "log_SPRS";
}

namespace  N_SPRS
{
	enum
	{
		EN_SEC_Trim = 15 * 60,  // 監視期限は 15分
		EN_Limit_Cnct = 3,  // ３回まで接続を許可する
	};

	enum { EN_MAX_RECD_SPRS = 100 };  // 保持する KRecd_Sprs の最大数
}

//////////////////////////////////////////////////////////////

struct  KIP_v4
{
	uint32_t  m_IPv4;

	void  SetBy(const boost::asio::ip::address& addr)
	{ m_IPv4 = addr.ipv4_address_.addr_.s_addr; }

	// ---------------------------------------
	// 戻り値は、書き終えた位置の次のアドレス
	uint8_t*  DBG__toStr(uint8_t* pdst) const;  // pdst は、16バイト以上あること
};

struct  KIP_v6
{
	uint64_t  m_IPv6_up;
	uint64_t  m_IPv6_down;
	
	void  SetBy(const boost::asio::ip::address& addr) {
		m_IPv6_up = *(uint64_t*)addr.ipv6_address_.addr_.__in6_u.__u6_addr8;
		m_IPv6_down = *(uint64_t*)(addr.ipv6_address_.addr_.__in6_u.__u6_addr8 + 8);
	}

	// ---------------------------------------
	// 戻り値は、書き終えた位置の次のアドレス
	uint8_t*  DBG__toStr(uint8_t* pdst) const;  // pdst は、40バイト以上あること
};

struct  KIP_ret
{
	bool  mb_Is_v4;
	KIP_v4  m_KIP_v4;
	KIP_v6  m_KIP_v6;
};


//////////////////////////////////////////////////////////////

class  KMng_DevIP
{
public:
	enum { EN_MAX_RECD_DEVIP = 10 };  // 保持する KRecd_DevIP の最大数
	enum class  EN_IsDevIP { EN_Yes, EN_No, EN_Err };

	static void  Push(const char* pstr_ip);
	// Push() の後に、登録がなされたかどうかの確認で呼び出されることを想定している
	static void  LogFlush();

	static EN_IsDevIP  IsDevIP(const boost::asio::ip::tcp::endpoint& crtcp_ep, KIP_ret* o_pret);

private:
	static int  ms_num_devIP_v4;  // 現在保持している個数
	static KIP_v4  msa_devIP_v4[EN_MAX_RECD_DEVIP];
	static const KIP_v4* const  msc_pdevIP_v4_tmnt;

	static int  ms_num_devIP_v6;  // 現在保持している個数
	static KIP_v6  msa_devIP_v6[EN_MAX_RECD_DEVIP];
	static const KIP_v6* const  msc_pdevIP_v6_tmnt;
};

//////////////////////////////////////////////////////////////
// KRecd_SPRS は、KClient オブジェクトと協調動作する

class  KClient;

struct  KRecd_SPRS
{
	bool  mb_Is_v4;
	union {
		KIP_v4  m_v4;
		KIP_v6  m_v6;
	} mu_KIP;
	// SPRS で監視対象になっている KClient
	// KClient がリサイクルされた場合、NULL に設定される
	KClient*  m_pKClient = NULL;

	// ---------------------------------------
	time_t  m_time_1st_cnct;
	uint16_t  m_times_cnct = 0;

	// 開発者の場合 true となる。SPRS の監視対象ではないが、Acceptor の監視対象にはなる
	bool  mb_IsDev = false;
	bool  mb_InitRI = false;  // InitRI の下り通信生成フラグ

	// パスフレーズは 1000 - 9999 までの４桁の数字（実際は 1000 - 9191）
	// この値が 0 のときは、パスフレーズ無効を表す
	uint16_t  m_pass_phrs = 0;

	// =======================================
	// 以下はデバッグ用のコード（運用時には利用されないため、速度も要求されない）
	// メンバ変数の値を、テキスト形式で s_glog_SPRS に書き出す
	void  DBG__Write_toSPRS_Log(bool bIs_v4) const;
};

// -----------------------------------------------------------

class  KSPRS_rld
{
public:
	// m_times_cnct <= EN_Limit_Cnct + 1 までは、WebSocket の破棄処理が必要となる
	// m_times_cnct > EN_Limit_Cnct + 1 であるときは、何もしなくてもよい
	// DevIP でなく、戻り値が NULL であった場合はエラーであるため、socket は close させること
	static KRecd_SPRS*  Chk_IP(const boost::asio::ip::tcp::endpoint& crtcp_ep);

private:
	// m_time_1st_cnct が time_trim 以前のものを Trim する（ms_precd_SPRS_dtytop の位置更新）
	// 戻り値は、Trim後の空き領域の個数
	static int  TrimArray_v4(time_t time_trim);
	static int  TrimArray_v6(time_t time_trim);

	// dty部分に、precd_IP と一致するものがあるかどうか探す。一致するものがなければ、NULL が返される
	static KRecd_SPRS*  Srch_inDty_v4(const KIP_v4& kip_v4);
	static KRecd_SPRS*  Srch_inDty_v6(const KIP_v6& kip_v6);

	static KRecd_SPRS*  GetNext_precd_SPRS_v4() {
		KRecd_SPRS*  cret = ms_precd_SPRS_next_v4;
		if (++ms_precd_SPRS_next_v4 == msc_precd_SPRS_tmnt_v4) { ms_precd_SPRS_next_v4 = msa_recd_SPRS_v4; }
		return  cret;
	};
	static KRecd_SPRS*  GetNext_precd_SPRS_v6() {
		KRecd_SPRS*  cret = ms_precd_SPRS_next_v6;
		if (++ms_precd_SPRS_next_v6 == msc_precd_SPRS_tmnt_v6) { ms_precd_SPRS_next_v6 = msa_recd_SPRS_v6; }
		return  cret;
	};

	// =======================================
	static KRecd_SPRS  msa_recd_SPRS_v4[N_SPRS::EN_MAX_RECD_SPRS];
	static const KRecd_SPRS* const  msc_precd_SPRS_tmnt_v4;
	static KRecd_SPRS*  ms_precd_SPRS_dtytop_v4;  // データ有効範囲トップ
	static KRecd_SPRS*  ms_precd_SPRS_next_v4;  // 次にストアを行う位置

	static KRecd_SPRS  msa_recd_SPRS_v6[N_SPRS::EN_MAX_RECD_SPRS];
	static const KRecd_SPRS* const  msc_precd_SPRS_tmnt_v6;
	static KRecd_SPRS*  ms_precd_SPRS_dtytop_v6;  // データ有効範囲トップ
	static KRecd_SPRS*  ms_precd_SPRS_next_v6;  // 次にストアを行う位置

public:
	// =======================================
	// 以下はデバッグ用コード
	static void  DBG__List_recds_SPRS();
	// 現在保持している KRecd_SPRS の m_time_1st_cnct を、指定された「秒数」だけ戻す
	// 時間を指定した「秒数」だけ、時間を進めた効果が得られる（sec <= 9999（= 166分）であること）
	static void  MS_DBG__Set_1st_cnct_back_by(int sec);
};
