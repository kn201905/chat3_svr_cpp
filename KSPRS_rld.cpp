#include <cstddef>
#include <iostream>
#include <boost/asio.hpp>

#include "__common/_KString.h"
#include "__common/LogID.h"
#include "KgLog/src/KgLog.h"
#include "KSPRS_rld.h"
#include "KClient.h"

namespace asio = boost::asio;

#define IS_CREATE_SPRS_LOG  false

#if IS_CREATE_SPRS_LOG
static KgLog  s_glog_SPRS{ N_gLog_SPRS::EN_bytes_buf, N_gLog_SPRS::EN_max_bytes_msg, N_gLog_SPRS::CE_str_fbody
				, N_gLog_SPRS::EN_bytes_file, N_gLog_SPRS::EN_bytes_fflush, KgLog::EN_FileType::Binary };

#define glog_SPRS_Wrt_Push_devIP_v4(a)  s_glog_SPRS.Wrt_Push_devIP_v4(a)
#define glog_SPRS_Wrt_Push_devIP_v6(a)  s_glog_SPRS.Wrt_Push_devIP_v6(a)
#define glog_SPRS_WriteID_with_UnixTime(a, b, c)  s_glog_SPRS.WriteID_with_UnixTime((a), (b), (c))
#define glog_SPRS_Flush  s_glog_SPRS.Flush

#define glog_SPRS_Wrt_IP_with_UTime_v4(a, b)  s_glog_SPRS.Wrt_IP_with_UnixTime_v4((a), (b))
#define glog_SPRS_Wrt_IP_with_UTime_v6(a, b)  s_glog_SPRS.Wrt_IP_with_UnixTime_v6((a), (b))
#define glog_SPRS_Wrt_IP_many_times_cnct_wUT_v4(a, b)  s_glog_SPRS.Wrt_IP_many_times_cnct_wUT_v4((a), (b))
#define glog_SPRS_Wrt_IP_many_times_cnct_wUT_v6(a, b)  s_glog_SPRS.Wrt_IP_many_times_cnct_wUT_v6((a), (b))
#else
#define glog_SPRS_Wrt_Push_devIP_v4(a)
#define glog_SPRS_Wrt_Push_devIP_v6(a)
#define glog_SPRS_WriteID_with_UnixTime(a, b, c)
#define glog_SPRS_Flush(a)

#define glog_SPRS_Wrt_IP_with_UTime_v4(a, b)
#define glog_SPRS_Wrt_IP_with_UTime_v6(a, b)
#define glog_SPRS_Wrt_IP_many_times_cnct_wUT_v4(a, b)
#define glog_SPRS_Wrt_IP_many_times_cnct_wUT_v6(a, b)
#endif


/////////////////////////////////////////////////////////////////////
// リソース不足でない限り、必ず KRecd_SPRS* を返すように仕様変更

static KIP_ret  s_KIP_ret;
KRecd_SPRS*  KSPRS_rld::Chk_IP(const boost::asio::ip::tcp::endpoint& crtcp_ep)
{
	// 開発者チェック（ログへの記録だけを実行する。実運用して不要であれば削除してよいコード）
	const bool  cb_IsDevIP = [&]() ->bool {
		const KMng_DevIP::EN_IsDevIP  cret = KMng_DevIP::IsDevIP(crtcp_ep, &s_KIP_ret);

		if (cret == KMng_DevIP::EN_IsDevIP::EN_Yes)  // 開発者である場合
		{
			// ログ記録
			if (s_KIP_ret.mb_Is_v4)
			{ glog_SPRS_Wrt_IP_with_UTime_v4(N_LogID::EN_ChkIP_devIP_v4, s_KIP_ret.m_KIP_v4); }
			else
			{ glog_SPRS_Wrt_IP_with_UTime_v6(N_LogID::EN_ChkIP_devIP_v6, s_KIP_ret.m_KIP_v6); }

			return  true;
		}
		return  false;
	}();

	// 登録済みIP チェック
	const time_t  ctime_now = time(NULL);
	if (s_KIP_ret.mb_Is_v4)
	{
		// ----- IP_v4 の処理 -----
		if (KSPRS_rld::TrimArray_v4(ctime_now - N_SPRS::EN_SEC_Trim) < 2)
		{
			constexpr KCnstString  ce_str{ "KSPRS_rld::Chk_IP() ▶ recd_SPRS_v4 のバッファが不足しています" };
			glog_SPRS_WriteID_with_UnixTime(N_LogID::EN_ERR_STR, ce_str.mc_cpstr, ce_str.mc_len);
			return  NULL;
		}

		if (cb_IsDevIP)
		{
RET_new_SPRS_recd_v4:
			KRecd_SPRS* const  new_precd_SPRS = KSPRS_rld::GetNext_precd_SPRS_v4();

			new_precd_SPRS->mb_Is_v4 = true;
			new_precd_SPRS->mu_KIP.m_v4 = s_KIP_ret.m_KIP_v4;
			new_precd_SPRS->m_pKClient = NULL;  // 念の為（多分、不必要）

			new_precd_SPRS->m_time_1st_cnct = ctime_now;
			new_precd_SPRS->m_times_cnct = 1;

			new_precd_SPRS->mb_IsDev = cb_IsDevIP;
			new_precd_SPRS->mb_InitRI = true;
			new_precd_SPRS->m_pass_phrs = 0;

			return  new_precd_SPRS;
		}

		KRecd_SPRS* const  precd_SPRS = KSPRS_rld::Srch_inDty_v4(s_KIP_ret.m_KIP_v4);

		if (precd_SPRS == NULL)  // 新規接続のクライアント
		{
			// ログ記録
			glog_SPRS_Wrt_IP_with_UTime_v4(N_LogID::EN_ChkIP_v4, s_KIP_ret.m_KIP_v4);
			goto  RET_new_SPRS_recd_v4;
		}

		// -----------------------------------
		// 登録済みの IP の場合の処理（連続アクセスに備えて、ログを最小限に抑える）
		const uint16_t  c_times = ++precd_SPRS->m_times_cnct;
		if (c_times > N_SPRS::EN_Limit_Cnct)
		{
			// 16回の接続毎にログを残しておくことにする
			if ((c_times & 0x0f) == 0)
			{ glog_SPRS_Wrt_IP_many_times_cnct_wUT_v4(c_times, s_KIP_ret.m_KIP_v4); }
			return  precd_SPRS;
		}

		// -----------------------------------
		// 接続回数が２回以上ではあるが、EN_Limit_Cnct 以下の場合の処理
		precd_SPRS->mb_InitRI = false;  // パスフレーズ認証をクリアした後、InitRI が許可される
		// 1000 - 9191 の範囲の数が設定される
		precd_SPRS->m_pass_phrs = (uint16_t(rand()) & 0x1fff) + 1000;

		return  precd_SPRS;
	}
	else
	{
		// ----- IP_v6 の処理 -----
		if (KSPRS_rld::TrimArray_v6(ctime_now - N_SPRS::EN_SEC_Trim) < 2)
		{
			constexpr KCnstString  ce_str{ "KSPRS_rld::Chk_IP() ▶ recd_SPRS_v6 のバッファが不足しています" };
			glog_SPRS_WriteID_with_UnixTime(N_LogID::EN_ERR_STR, ce_str.mc_cpstr, ce_str.mc_len);
			return  NULL;
		}

		if (cb_IsDevIP)
		{
RET_new_SPRS_recd_v6:
			KRecd_SPRS* const  new_precd_SPRS = KSPRS_rld::GetNext_precd_SPRS_v6();

			new_precd_SPRS->mb_Is_v4 = false;
			new_precd_SPRS->mu_KIP.m_v6 = s_KIP_ret.m_KIP_v6;
			new_precd_SPRS->m_pKClient = NULL;  // 念の為（多分、不必要）

			new_precd_SPRS->m_time_1st_cnct = ctime_now;
			new_precd_SPRS->m_times_cnct = 1;

			new_precd_SPRS->mb_IsDev = cb_IsDevIP;
			new_precd_SPRS->mb_InitRI = true;
			new_precd_SPRS->m_pass_phrs = 0;

			return  new_precd_SPRS;
		}

		KRecd_SPRS* const  precd_SPRS = KSPRS_rld::Srch_inDty_v6(s_KIP_ret.m_KIP_v6);

		if (precd_SPRS == NULL)  // 新規接続のクライアント
		{
			// ログ記録
			glog_SPRS_Wrt_IP_with_UTime_v6(N_LogID::EN_ChkIP_v6, s_KIP_ret.m_KIP_v6);

			goto  RET_new_SPRS_recd_v6;
		}

		// -----------------------------------
		// 登録済みの IP の場合の処理（連続アクセスに備えて、ログを最小限に抑える）
		const uint16_t  c_times = ++precd_SPRS->m_times_cnct;
		if (c_times > N_SPRS::EN_Limit_Cnct)
		{
			// 16回の接続毎にログを残しておくことにする
			if ((c_times & 0x0f) == 0)
			{ glog_SPRS_Wrt_IP_many_times_cnct_wUT_v6(c_times, s_KIP_ret.m_KIP_v6); }
			return  precd_SPRS;
		}

		// -----------------------------------
		// 接続回数が２回以上ではあるが、EN_Limit_Cnct 以下の場合の処理
		precd_SPRS->mb_InitRI = false;  // パスフレーズ認証をクリアした後、InitRI が許可される
		// 1000 - 9191 の範囲の数が設定される
		precd_SPRS->m_pass_phrs = (uint16_t(rand()) & 0x1fff) + 1000;

		return  precd_SPRS;
	}
}


/////////////////////////////////////////////////////////////////////

int  KMng_DevIP::ms_num_devIP_v4 = 0;
KIP_v4  KMng_DevIP::msa_devIP_v4[EN_MAX_RECD_DEVIP];
const KIP_v4* const  KMng_DevIP::msc_pdevIP_v4_tmnt = msa_devIP_v4 + EN_MAX_RECD_DEVIP;

int  KMng_DevIP::ms_num_devIP_v6 = 0;
KIP_v6  KMng_DevIP::msa_devIP_v6[EN_MAX_RECD_DEVIP];
const KIP_v6* const  KMng_DevIP::msc_pdevIP_v6_tmnt = msa_devIP_v6 + EN_MAX_RECD_DEVIP;

// ------------------------------------------------------------------

void  KMng_DevIP::Push(const char* const pstr_ip)
{
	boost::system::error_code ec;
	const asio::ip::address  caddr = asio::ip::make_address(pstr_ip, ec);
	if (ec)
	{
		std::cout << "KMng_DevIP::Push() に渡されたアドレスが無効です : " << pstr_ip << "\n";
	}
	
	if (caddr.is_v4())
	{
		if (ms_num_devIP_v4 == EN_MAX_RECD_DEVIP)
		{
			std::cout << "KMng_DevIP::Push() ▶ DevIP_v4 の登録数が上限を超えています。\n";
			return;
		}

		msa_devIP_v4[ms_num_devIP_v4].SetBy(caddr);
		glog_SPRS_Wrt_Push_devIP_v4(msa_devIP_v4[ms_num_devIP_v4]);
		ms_num_devIP_v4++;
	}
	else
	{
		if (ms_num_devIP_v6 == EN_MAX_RECD_DEVIP)
		{
			std::cout << "KMng_DevIP::Push() ▶ DevIP_v6 の登録数が上限を超えています。\n";
			return;
		}

		msa_devIP_v6[ms_num_devIP_v6].SetBy(caddr);
		glog_SPRS_Wrt_Push_devIP_v6(msa_devIP_v6[ms_num_devIP_v6]);
		ms_num_devIP_v6++;
	}
}

// ------------------------------------------------------------------

void  KMng_DevIP::LogFlush()
{
	constexpr KCnstString  ce_str{ "KMng_DevIP::LogFlush() が実行されました" };
	glog_SPRS_WriteID_with_UnixTime(N_LogID::EN_STR, ce_str.mc_cpstr, ce_str.mc_len);
	glog_SPRS_Flush();
}

// ------------------------------------------------------------------
// 開発者であってもなくても、o_pret にアドレスが記録される

KMng_DevIP::EN_IsDevIP  KMng_DevIP::IsDevIP(const asio::ip::tcp::endpoint& crtcp_ep, KIP_ret* o_pret)
{
	if (crtcp_ep.impl_.is_v4())
	{
		const uint32_t  cipv4 = crtcp_ep.impl_.data_.v4.sin_addr.s_addr;
		o_pret->mb_Is_v4 = true;
		o_pret->m_KIP_v4.m_IPv4 = cipv4;

		const KIP_v4*  pdevIP_v4 = msa_devIP_v4;
		for (int i = ms_num_devIP_v4; i > 0; --i)
		{
			if (pdevIP_v4->m_IPv4 == cipv4) { return  EN_IsDevIP::EN_Yes; }
			pdevIP_v4++;
		}
	}
	else
	{
		const uint64_t* const  cp_ipv6 = (uint64_t*)crtcp_ep.impl_.data_.v6.sin6_addr.__in6_u.__u6_addr8;
		const uint64_t  cipv6_up = *cp_ipv6;
		const uint64_t  cipv6_down = *(cp_ipv6 + 1);
		o_pret->mb_Is_v4 = false;
		o_pret->m_KIP_v6.m_IPv6_up = cipv6_up;
		o_pret->m_KIP_v6.m_IPv6_down = cipv6_down;

		const KIP_v6*  pdevIP_v6 = msa_devIP_v6;
		for (int i = ms_num_devIP_v6; i > 0; --i)
		{
			if (pdevIP_v6->m_IPv6_up == cipv6_up && pdevIP_v6->m_IPv6_down == cipv6_down) { return  EN_IsDevIP::EN_Yes; }
			pdevIP_v6++;
		}
	}

	return  EN_IsDevIP::EN_No;
}


/////////////////////////////////////////////////////////////////////

KRecd_SPRS  KSPRS_rld::msa_recd_SPRS_v4[N_SPRS::EN_MAX_RECD_SPRS];
const KRecd_SPRS* const  KSPRS_rld::msc_precd_SPRS_tmnt_v4 = msa_recd_SPRS_v4 + N_SPRS::EN_MAX_RECD_SPRS;
KRecd_SPRS*  KSPRS_rld::ms_precd_SPRS_dtytop_v4 = msa_recd_SPRS_v4;
KRecd_SPRS*  KSPRS_rld::ms_precd_SPRS_next_v4 = msa_recd_SPRS_v4;

KRecd_SPRS  KSPRS_rld::msa_recd_SPRS_v6[N_SPRS::EN_MAX_RECD_SPRS];
const KRecd_SPRS* const  KSPRS_rld::msc_precd_SPRS_tmnt_v6 = msa_recd_SPRS_v6 + N_SPRS::EN_MAX_RECD_SPRS;
KRecd_SPRS*  KSPRS_rld::ms_precd_SPRS_dtytop_v6 = msa_recd_SPRS_v6;
KRecd_SPRS*  KSPRS_rld::ms_precd_SPRS_next_v6 = msa_recd_SPRS_v6;

// ------------------------------------------------------------------
// 本来は static であるが、KClient.h で friend 指定しているため、static にはできなかった

inline int  S_Trim(time_t ctime_trim, KRecd_SPRS* const cprecd_top, const KRecd_SPRS* const cprecd_tmnt
						, KRecd_SPRS** const cpprecd_dty, KRecd_SPRS* const cprecd_next)
{	
	KRecd_SPRS*  pdty = *cpprecd_dty;
	if (pdty == cprecd_next) { return  N_SPRS::EN_MAX_RECD_SPRS; }

	if (cprecd_next < pdty)  // dty が先頭部分にもある場合
	{
		while (true)
		{
			if (ctime_trim < pdty->m_time_1st_cnct)  // Array に残す情報が見つかった場合
			{
				*cpprecd_dty = pdty;
				return  pdty - cprecd_next;
			}

			// 監視対象の KClient が存在していれば、監視対象から外れたことを通知する
			if (pdty->m_pKClient)
			{
				pdty->m_pKClient->m_precd_SPRS = NULL;
				pdty->m_pKClient = NULL;
			}

			if (++pdty == cprecd_tmnt) { break; }
		}
		pdty = cprecd_top;
	}

	while (true)
	{
		if (pdty == cprecd_next)
		{
			*cpprecd_dty = cprecd_next;
			return  N_SPRS::EN_MAX_RECD_SPRS;
		}

		// pdty を trim する
		if (pdty->m_pKClient)
		{
			pdty->m_pKClient->m_precd_SPRS = NULL;
			pdty->m_pKClient = NULL;
		}

		if (ctime_trim < pdty->m_time_1st_cnct)  // Array に残す情報が見つかった場合
		{
			*cpprecd_dty = pdty;
			return  N_SPRS::EN_MAX_RECD_SPRS - (cprecd_next - pdty);
		}
	}
};

int  KSPRS_rld::TrimArray_v4(time_t const ctime_trim)
{
	return  S_Trim(ctime_trim, (KRecd_SPRS*)msa_recd_SPRS_v4, msc_precd_SPRS_tmnt_v4
					, &ms_precd_SPRS_dtytop_v4, ms_precd_SPRS_next_v4);
}

int  KSPRS_rld::TrimArray_v6(time_t const ctime_trim)
{
	return  S_Trim(ctime_trim, (KRecd_SPRS*)msa_recd_SPRS_v6, msc_precd_SPRS_tmnt_v6
					, &ms_precd_SPRS_dtytop_v6, ms_precd_SPRS_next_v6);
}

// ------------------------------------------------------------------

KRecd_SPRS*  KSPRS_rld::Srch_inDty_v4(const KIP_v4& kip_v4)
{
	const uint32_t  cip_v4 = kip_v4.m_IPv4;

	KRecd_SPRS*  pdty;
	if (ms_precd_SPRS_next_v4 < ms_precd_SPRS_dtytop_v4)  // dty が先頭部分にもある場合
	{
		pdty = ms_precd_SPRS_dtytop_v4;
		while (true)
		{
			if (pdty->mu_KIP.m_v4.m_IPv4 == cip_v4)
			{ return  pdty; }

			if (++pdty == msc_precd_SPRS_tmnt_v4) { break; }
		}
		pdty = msa_recd_SPRS_v4;
	}
	else
	{ pdty = ms_precd_SPRS_dtytop_v4; }

	while (true)
	{
		if (pdty == ms_precd_SPRS_next_v4) { break; }

		if (pdty->mu_KIP.m_v4.m_IPv4 == cip_v4)
		{ return  pdty; }
		pdty++;
	}
	return  NULL;
}

// ------------------------------------------------------------------

KRecd_SPRS*  KSPRS_rld::Srch_inDty_v6(const KIP_v6& kip_v6)
{
	const uint64_t  cip_v6_up = kip_v6.m_IPv6_up;
	const uint64_t  cip_v6_down = kip_v6.m_IPv6_down;

	KRecd_SPRS*  pdty;
	if (ms_precd_SPRS_next_v6 < ms_precd_SPRS_dtytop_v6)  // dty が先頭部分にもある場合
	{
		pdty = ms_precd_SPRS_dtytop_v6;
		while (true)
		{
			if (pdty->mu_KIP.m_v6.m_IPv6_up == cip_v6_up && pdty->mu_KIP.m_v6.m_IPv6_down == cip_v6_down)
			{ return  pdty; }

			if (++pdty == msc_precd_SPRS_tmnt_v6) { break; }
		}
		pdty = msa_recd_SPRS_v6;
	}
	else
	{ pdty = ms_precd_SPRS_dtytop_v6; }

	while (true)
	{
		if (pdty == ms_precd_SPRS_next_v6) { break; }

		if (pdty->mu_KIP.m_v6.m_IPv6_up == cip_v6_up && pdty->mu_KIP.m_v6.m_IPv6_down == cip_v6_down)
		{ return  pdty; }
		pdty++;
	}
	return  NULL;
}


///////////////////////////////////////////////////////////////////
// 以下は、デバッグ用コード（運用中は利用されないコード。削除可）

static uint8_t*  S_Wrt_Dec_3dig_dot(const uint8_t num, uint8_t* pdst)
{
	if (num > 99)
	{
		const uint  cdig_1 = num / 100;
		const uint  crem_1 = num % 100;
		*(uint32_t*)pdst = (cdig_1 + ((crem_1 / 10) << 8) + ((crem_1 % 10) << 16)) + 0x2e30'3030;
		return  pdst + 4;
	}
	
	if (num > 9)
	{
		*(uint32_t*)pdst = ((num / 10) + ((num % 10) << 8)) + 0x2e'3030;
		return  pdst + 3;
	}

	*(uint16_t*)pdst = num + 0x2e30;
	return  pdst + 2;
}

uint8_t*  KIP_v4::DBG__toStr(uint8_t* pdst) const
{
	pdst = S_Wrt_Dec_3dig_dot(m_IPv4 & 0xff, pdst);
	pdst = S_Wrt_Dec_3dig_dot((m_IPv4 >> 8) & 0xff, pdst);
	pdst = S_Wrt_Dec_3dig_dot((m_IPv4 >> 16) & 0xff, pdst);
	return  S_Wrt_Dec_3dig_dot((m_IPv4 >> 24) & 0xff, pdst) - 1;
}

// ----------------------------------------------------------------

static uint8_t*  S_Wrt_Hex_4dig_col(const uint16_t num, uint8_t* pdst)
{
	if (num & 0x00f0)
	{
		uint  dig_1 = (num & 0xf0) >> 4;
		if (dig_1 > 9) { dig_1 += 39; }
		uint  dig_2 = (num & 0x0f) << 8;
		if (dig_2 > 0x900) { dig_2 += 39 << 8; }
		uint  dig_3 = (num & 0xf0'00) << 4;
		if (dig_3 > 0x9'0000) { dig_3 += 39 << 16; }
		uint  dig_4 = (num & 0x0f'00) << 16;
		if (dig_4 > 0x900'0000) { dig_4 += 39 << 24; }

		*(uint32_t*)pdst = dig_1 + dig_2 + dig_3 + dig_4 + 0x3030'3030;
		*(pdst + 4) = 0x3a;
		return  pdst + 5;
	}

	if (num & 0xf)
	{
		uint  dig_1 = num & 0xf;
		if (dig_1 > 9) { dig_1 += 39; }
		uint  dig_2 = (num & 0xf0'00) >> 4;
		if (dig_2 > 0x900) { dig_2 += 39 << 8; }
		uint  dig_3 = (num & 0xf'00) << 8;
		if (dig_3 > 0x9'0000) { dig_3 += 39 << 16; }

		*(uint32_t*)pdst = dig_1 + dig_2 + dig_3 + 0x3a30'3030;
		return  pdst + 4;
	}

	if (num & 0xf0'00)
	{
		uint  dig_1 = (num & 0xf0'00) >> 12;
		if (dig_1 > 9) { dig_1 += 39; }
		uint  dig_2 = num & 0xf'00;
		if (dig_2 > 0x900) { dig_2 += 39 << 8; }

		*(uint32_t*)pdst = dig_1 + dig_2 + 0x3a'3030;
		return  pdst + 3;
	}

	uint  dig_1 = (num & 0xf'00) >> 8;
	if (dig_1 > 9) { dig_1 += 39; }

	*(uint16_t*)pdst = dig_1 + 0x3a30;
	return  pdst + 2;
}

uint8_t*  KIP_v6::DBG__toStr(uint8_t* pdst) const
{
	auto  Uin64_ToStr = [](uint64_t num, uint8_t* pdst) -> uint8_t* {
		for (int i = 4; i > 0; --i)
		{
			pdst = S_Wrt_Hex_4dig_col(num & 0xffff, pdst);
			num >>= 16;
		}
		return  pdst;
	};

	// ---------------------------
	pdst = Uin64_ToStr(m_IPv6_up, pdst);
	return  Uin64_ToStr(m_IPv6_down, pdst) - 1; 
}

// ----------------------------------------------------------------

void  S_DBG__WriteTxt_toSPRS_Log(const char* psrc)
{
#if IS_CREATE_SPRS_LOG
	const uint16_t  cbytes_len = CE_StrLen(psrc);
	s_glog_SPRS.WriteID(N_LogID::EN_STR, psrc, cbytes_len);
#endif
}

void  KSPRS_rld::DBG__List_recds_SPRS()
{
	S_DBG__WriteTxt_toSPRS_Log("----- List_recd_SPRS -----");

	KRecd_SPRS*  precd = ms_precd_SPRS_dtytop_v4;
	while (true)
	{
		if (precd == ms_precd_SPRS_next_v4) { break; }
		precd->DBG__Write_toSPRS_Log(true);  // KRecd_SPRS の内容を s_glog_SPRS に書き出す

		precd++;
		if (precd == msc_precd_SPRS_tmnt_v4) { precd = msa_recd_SPRS_v4; }
	}

	precd = ms_precd_SPRS_dtytop_v6;
	while (true)
	{
		if (precd == ms_precd_SPRS_next_v6) { break; }
		precd->DBG__Write_toSPRS_Log(false);  // KRecd_SPRS の内容を s_glog_SPRS に書き出す

		precd++;
		if (precd == msc_precd_SPRS_tmnt_v6) { precd = msa_recd_SPRS_v6; }
	}

	S_DBG__WriteTxt_toSPRS_Log("----- List_recd_SPRS END -----");
}

// ----------------------------------------------------------------
// ヘッダは EN_STR として、書込みを行う

void  KRecd_SPRS::DBG__Write_toSPRS_Log(const bool bIs_v4) const
{
#if IS_CREATE_SPRS_LOG
	// 下準備 ----------------------------
	constexpr size_t  ce_max_bytes = 1 + 2 + 12  // LogID + len + "KRecd_SPRS/ "
		+ 4 + 39  // "IP: " + 最大 39文字（IPv6）
		+ 18 + 5  // " , 1st_cnct past: " + 最大4桁（マイナス符号を含めて）
		+ 10 + 4  // " , times: " + 最大4桁
		+ 11 + 5  // " , InitRI: " + "false"
		+ 9 + 4;  // " , pass: " + 4桁
	static_assert(ce_max_bytes <= N_gLog_SPRS::EN_max_bytes_msg);
	// -----------------------------------

	// ここから関数本体（pbuf_log のみを通して利用する）
	uint8_t* pbuf_log = s_glog_SPRS.Get_pos_next();  // KgLog から、直接書込み位置を受け取る
	uint8_t* const  pcbuf_log_bgn = pbuf_log;  // 書込み初めのアドレスを保存しておく

	*pbuf_log = N_LogID::EN_STR;  // ヘッダ書き込み
	pbuf_log += 3;  // ID + len の３バイトをとばす

	// IP アドレスの書き込み
	{
		constexpr KCnstString  ce_str{ "KRecd_Sprs/ IP: " };
		pbuf_log = (uint8_t*)ce_str.WriteTo(pbuf_log);

		if (bIs_v4)
		{ pbuf_log = mu_KIP.m_v4.DBG__toStr(pbuf_log); }
		else
		{ pbuf_log = mu_KIP.m_v6.DBG__toStr(pbuf_log); }
	}

	// m_time_1st_cnct と「現時刻までの経過時間」の書込み
	{
		constexpr KCnstString  ce_str{ " , 1st_cnct past: " };
		pbuf_log = (uint8_t*)ce_str.WriteTo(pbuf_log);

		const int  cpast_sec = ([&]() -> int {
			const int64_t  cret_val = time(NULL) - m_time_1st_cnct;
			if (cret_val > 9999) { return  9999; }
			if (cret_val < -9999) { return  -9999; }
			return  int(cret_val);
		})();

		const int  cbytes_past_sec = sprintf((char*)pbuf_log, "%d", cpast_sec);  // 確実に -9999 <= cpast_sec <= 9999
		pbuf_log += cbytes_past_sec;
	}

	// 接続回数の書込み（最大 9999 まで）
	{
		constexpr KCnstString  ce_str{ " , times: " };
		pbuf_log = (uint8_t*)ce_str.WriteTo(pbuf_log);

		const uint16_t  c_times_cnct = m_times_cnct > 9999 ? 9999 : m_times_cnct;

		const int  cbytes_times = sprintf((char*)pbuf_log, "%d", c_times_cnct);  // 確実に 1 <= c_times_cnct <= 9999
		pbuf_log += cbytes_times;
	}

	// mb_InitRI
	{
		if (mb_InitRI)
		{
			constexpr KCnstString  ce_str{ " , InitRI: true" };
			pbuf_log = (uint8_t*)ce_str.WriteTo(pbuf_log);
		}
		else
		{
			constexpr KCnstString  ce_str{ " , InitRI: false" };
			pbuf_log = (uint8_t*)ce_str.WriteTo(pbuf_log);
		}
	}

	// ma_pass_phrs の書込み
	{
		constexpr KCnstString  ce_str{ " , pass: " };
		pbuf_log = (uint8_t*)ce_str.WriteTo(pbuf_log);

		const int  cpass = m_pass_phrs > 9999 ? 9999 : m_pass_phrs;

		const int  cbytes_pass = sprintf((char*)pbuf_log, "%d", cpass);
		pbuf_log += cbytes_pass;
	}

	const uint16_t  cbytes_wrt = uint16_t(pbuf_log - pcbuf_log_bgn);
	*(uint16_t*)(pcbuf_log_bgn + 1) = uint16_t(cbytes_wrt - 3);  // len の書込み（-3: ID + len）
	s_glog_SPRS.Adv_pos_next(cbytes_wrt);
#endif
}

// ----------------------------------------------------------------
// 現在保持している KRecd_Sprs の m_time_1st_cnct を、指定された「秒数」だけ戻す

char  s_SB_DBG__Set_1st_cnct_back_by[] = "----- Set_1st_cnct_time_back_by #### -----";

void  KSPRS_rld::MS_DBG__Set_1st_cnct_back_by(int sec)
{
#if IS_CREATE_SPRS_LOG
	// -----------------------------------
	// MS_DBG__Set_1st_cnct_back_by を実行することを表す文字列を書き込む
	{
		if (sec <= 0) { sec = 1; }
		if (sec > 9999) { sec = 9999; }
		const int  clen_hdr = sprintf(s_SB_DBG__Set_1st_cnct_back_by
										, "----- Set_1st_cnct_time_back_by %d -----", sec);
		s_glog_SPRS.WriteID(N_LogID::EN_STR, s_SB_DBG__Set_1st_cnct_back_by, clen_hdr);
	}

	// -----------------------------------
	// 秒数を戻す
	KRecd_SPRS*  precd = ms_precd_SPRS_dtytop_v4;
	while (true)
	{
		if (precd == ms_precd_SPRS_next_v4) { break; }
		precd->m_time_1st_cnct -= sec;  // 接続時刻を変更する

		precd++;
		if (precd == msc_precd_SPRS_tmnt_v4) { precd = msa_recd_SPRS_v4; }
	}

	precd = ms_precd_SPRS_dtytop_v6;
	while (true)
	{
		if (precd == ms_precd_SPRS_next_v6) { break; }
		precd->m_time_1st_cnct -= sec;  // 接続時刻を変更する

		precd++;
		if (precd == msc_precd_SPRS_tmnt_v6) { precd = msa_recd_SPRS_v6; }
	}
#endif
}
