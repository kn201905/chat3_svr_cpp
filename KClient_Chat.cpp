#include <iostream>

#include <stdint.h>
#include <stddef.h>
#include <boost/asio.hpp>

#include "KServer.h"
#include "KClient_Chat.h"
#include "KSPRS_rld.h"  // InitRI の許可判断に必要

#include "__common/LogID.h"
#include "KgLog/src/KgLog.h"


///===DEBUG===///
extern void  G_cout_ui32(uint32_t srcval);
extern void  G_cout_xbytes(const uint8_t* psrc, const size_t bytes);


extern KgLog  g_glog;

using  std::cout;
using  std::endl;


KSmplList2<KRI, KRI::EN_NUM_KRI>  KRI::ms_List;
KSmplList2<KUInfo, KUInfo::EN_NUM_KUInfo>  KUInfo::ms_List;


////////////////////////////////////////////////////////////////
// KLargeBuf

KLargeBuf::KInfo  KLargeBuf::msa_Info[ENN_NUM_BUF];
const KLargeBuf::KInfo* const  KLargeBuf::msc_pInfo_tmnt = msa_Info + ENN_NUM_BUF;

uint8_t  KLargeBuf::msa_buf[EN_BYTES_BUF * ENN_NUM_BUF];

// -------------------------------------------------------------

static KLargeBuf_Initr  s_KLargeBuf_Initr;  // コンストラクタを起動させたいだけのもの
KLargeBuf_Initr::KLargeBuf_Initr()
{
	uint16_t  id = 1;
	uint8_t*  pbuf = KLargeBuf::msa_buf;
	KLargeBuf::KInfo*  pinfo = KLargeBuf::msa_Info;
	for (int i = 0; i < KLargeBuf::ENN_NUM_BUF; ++i)
	{
		pinfo->m_ID = id++;
		pinfo->m_pbuf = pbuf;
		pbuf += KLargeBuf::EN_BYTES_BUF;
		pinfo->m_pbuf_tmnt = pbuf;
		pinfo++;
	}
}

// -------------------------------------------------------------

KLargeBuf::KInfo*  KLargeBuf::ReqBuf_Single()
{
	KInfo*  pinfo = msa_Info;
	while (true)
	{
		if (pinfo->m_stt == 0)
		{
			pinfo->m_stt = 1;
			g_glog.WriteID_with_UnixTime(N_LogID::EN_REQ_LargeBuf, &pinfo->m_ID, 2);
			return  pinfo;
		}
		pinfo++;
		if (pinfo == msc_pInfo_tmnt)
		{
			g_glog.WriteID_with_UnixTime(N_LogID::EN_REQ_LargeBuf_Exausted);
			return NULL;
		}
	}
}

// -------------------------------------------------------------

void  KLargeBuf::Return_Buf(KInfo* pinfo_buf)
{
	auto num = pinfo_buf->m_stt;
	while (true)
	{
		pinfo_buf->m_stt = 0;
		if (--num == 0) { return; }
		pinfo_buf++;
	}
}


////////////////////////////////////////////////////////////////
// KUInfo
// 現在は、単純に super user は 1 - 999、通常の user は 1000 - としている

static  uint32_t  s_super_usr_ID = 1;  // 現在は暫定的な仕様
uint32_t  KUInfo::Crt_New_SuperUsrID()
{
	return  s_super_usr_ID++;
}

// -------------------------------------------------------------

static  uint32_t  s_usr_ID = 1000;  // 現在は暫定的な仕様
uint32_t  KUInfo::Crt_New_UsrID()
{
	return  s_usr_ID++;
}

////////////////////////////////////////////////////////////////
// KRI

// 書き込みバイト数の計算ミスを防ぐために uint8_t* を受け取るようにしている
void  KRI::Srlz_to(uint8_t* pdst)
{
	const uint8_t* const  c_pdst_begin = pdst;

	*(uint32_t*)pdst = m_rmID;
	*(uint16_t*)(pdst + 4) = m_topicID;
	*(pdst + 6) = m_capa;
	*(pdst + 7) = m_num_usrs_cur;
	*(uint16_t*)(pdst + 8) = m_bytes_rm_prof;
	memcpy(pdst + 10, ma_rm_prof, m_bytes_rm_prof);
	pdst += 10 + m_bytes_rm_prof;

	// ----------------------------------
	// ユーザリストの書き込み
	KUInfo_Elmt**  pp_UInfo = ma_pUInfo;
	int  i = m_num_usrs_cur;
	while (true)
	{
		const KUInfo* const  cpUInfo = *pp_UInfo;
		const size_t  cbytes_uInfo = cpUInfo->m_bytes_KUInfo;
		memcpy(pdst, cpUInfo, cbytes_uInfo);
		pdst += cbytes_uInfo;

		if (--i == 0) { break; }
		pp_UInfo++;
	}

	// ----------------------------------
///===DEBUG===///
	assert(m_bytes_send == pdst - c_pdst_begin);
}


////////////////////////////////////////////////////////////////
// コンパイル速度を優先させるためだけに存在している部分

KClient_Chat  ga_KClnt_Chat[KServer::NUM_CLIENT];
static KClient_Chat*  s_pKClnt_Chat_onGetNext = ga_KClnt_Chat;

KClient_Chat_Intf*  G_GetNext_KClnt_Chat()
{
	KClient_Chat_Intf*  ret_val = s_pKClnt_Chat_onGetNext;
	s_pKClnt_Chat_onGetNext++;
	return  ret_val;
}

////////////////////////////////////////////////////////////////
// KClient_Chat

void  KClient_Chat::Reset_prvt_buf_write()
{
	if (m_pInfo_LargeBuf)
	{
		KLargeBuf::Return_Buf(m_pInfo_LargeBuf);
		m_pInfo_LargeBuf = NULL;
	}
}

// -------------------------------------------------------------

void  KClient_Chat::Recycle()
{
	this->Reset_prvt_buf_write();

	if (m_pUInfo_Elmt)
	{
		// 念のため
		m_pUInfo_Elmt->m_bytes_KUInfo = 0;
		m_pUInfo_Elmt->m_uID = 0;

		KUInfo::ms_List.MoveToEnd(m_pUInfo_Elmt);
		m_pUInfo_Elmt = NULL;
	}
}

// -------------------------------------------------------------
// 受信したものに対してレスポンスを帰す場合は、KClient::m_asio_prvt_buf_write に値を設定して、EN_Write を返す

KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr  KClient_Chat::WS_Read_Hndlr(
											uint16_t* const c_pdata_payload, const size_t c_bytes_payload)
{
	m_time_WS_Read_Hndlr = ::time(NULL);

	const uint16_t  cop = *c_pdata_payload;
	switch (cop)
	{
		case  N_JSC::EN_UP_Init_RI:
			return  this->UP_InitRI();

		case  N_JSC::EN_UP_Crt_Usr:
			return  this->UP_Crt_Usr(c_pdata_payload, c_bytes_payload);

		default:
			cout << "RECEIVE Unknown OP" << endl;
	}

	return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Read;
//	return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Write;
}

// -------------------------------------------------------------

void  KClient_Chat::Set_AsioWrtBuf_with_PLHdr_to_RRQ(uint16_t rereq_jsc, uint16_t sec_wait)
{
	this->Reset_prvt_buf_write();  // 念の為

	uint8_t*  pbuf = m_pKClnt->ma_ui8_prvt_buf;
	m_pKClnt->m_asio_prvt_buf_write.data_ = pbuf;
	m_pKClnt->m_asio_prvt_buf_write.size_ = 6;

	// ペイロードヘッダを書き込む（ma_ui8_prvt_buf を先頭から利用する）
	*pbuf++ = 0x82;  // FIN: 0x80, バイナリフレーム 0x2
	*pbuf++ = 0x04;  // ペイロード長 4bytes: 遅延対象となったコマンド（2 bytes）+ sec_wait（2 bytes）
	*(uint16_t*)pbuf = rereq_jsc;
	pbuf += 2;
	*(uint16_t*)pbuf = sec_wait;
}

// -------------------------------------------------------------

using  KRI_Elmt = KSmplListElmt<KRI>;

KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr  KClient_Chat::UP_InitRI()
{
//	static_assert(sizeof(KUInfo) == 30, "KUInfo");

	// ----------------------------------
	// まず、この KClient が InitRI の情報を得る資格を持っているかどうかのチェック
	KRecd_SPRS* const  precd_SPRS = m_pKClnt->m_precd_SPRS;
	if (precd_SPRS == NULL || precd_SPRS->mb_InitRI == false)
	{
		// InitRI の受信資格を持っていな場合、ソケットを即閉じる
		if (m_pKClnt->mb_Is_v4)
		{ g_glog.WriteID_with_UnixTime(N_LogID::EN_NO_PMSN_InitRI_v4, &m_pKClnt->mu_IP.m_IPv4, 4); }
		else
		{ g_glog.WriteID_with_UnixTime(N_LogID::EN_NO_PMSN_InitRI_v6, &m_pKClnt->mu_IP.ma_IPv6, 16); }
		return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Close;
	}

	// まず、遅延リクエストのチェックを行う
	if (m_rrq_time_InitRI_CrtUsr)
	{
		if (m_time_WS_Read_Hndlr < m_rrq_time_InitRI_CrtUsr)
		{
			// 規定の時間より早く再リクエストを送ってきた場合は、不正クラアントとして切断する
			return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Close;
		}
		m_rrq_time_InitRI_CrtUsr = 0;
	}

	// ----------------------------------
	// Large バッファの準備をする
	m_pInfo_LargeBuf = KLargeBuf::ReqBuf_Single();
	if (m_pInfo_LargeBuf == NULL)
	{
		// バッファの準備に失敗した場合は、JSクライアントに再送要求を出すように指示をする
		m_rrq_time_InitRI_CrtUsr
			= m_time_WS_Read_Hndlr + N_JSC::EN_SEC_Wait_Init_RI - N_JSC::EN_SEC_Margin_Force_toCLOSE;

		this->Set_AsioWrtBuf_with_PLHdr_to_RRQ(N_JSC::EN_DN_Init_RI | N_JSC::EN_BUSY_WAIT_SEC
												, N_JSC::EN_SEC_Wait_Init_RI);
		return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Write;
	}

	// Large バッファに返信内容を書き込んでいく
	// 特別な資格がないクライアントに対しては、最大バイト数は EN_BYTES_BUF とする。
	uint8_t* const  pdata_payload = m_pInfo_LargeBuf->GetBuf() + 4;
	uint8_t*  pbuf_large = pdata_payload;
	const uint8_t* const  c_pbuf_large_tmnt = m_pInfo_LargeBuf->GetBuf_tmnt();
	// まずは、cmd ID を書き込んでおく
	*pbuf_large = N_JSC::EN_DN_Init_RI;
	pbuf_large += 2;

	// まず、多重接続が検出された場合の警告があれば、先に書き込んでおく
	if (precd_SPRS->m_times_cnct != 1)
	{
		*(uint16_t*)pbuf_large = N_JSC::EN_DN_Init_RI__WARN_multi_cnct;
		*(uint16_t*)(pbuf_large + 2) = precd_SPRS->m_times_cnct;
		*(uint16_t*)(pbuf_large + 4) = uint16_t(m_time_WS_Read_Hndlr - precd_SPRS->m_time_1st_cnct);
		*(uint16_t*)(pbuf_large + 6) = precd_SPRS->m_pass_phrs;
		pbuf_large += 8;
	}

	// ----------------------------------
	// pbuf_large、c_pbuf_large_tmnt を用いて Init_RI のレスポンスを作成（最新のものからストアしていく）

	const uint16_t  clen = KRI::ms_List.Get_Len();
	*(uint16_t*)(pbuf_large + 2) = clen;  // 全部の部屋数
	if (clen == 0)
	{
		*(uint16_t*)pbuf_large = 0;  // 送信される部屋数
		pbuf_large += 4;
	}
	else
	{
		uint16_t* const  c_pnum_rm_send = (uint16_t*)pbuf_large;
		pbuf_large += 4;

		uint  remn = clen;
		KRI_Elmt*  pelmt = KRI::ms_List.Get_LastEmnt();
		while (true)
		{
			const uint  cbytes_send = pelmt->m_bytes_send;
			if (pbuf_large + cbytes_send < c_pbuf_large_tmnt)
			{
				pelmt->Srlz_to(pbuf_large);
				pbuf_large += cbytes_send;
				if (--remn == 0) { break; }
			}
			else
			{ break; }
		}

		// 送信される部屋数を記録する
		*c_pnum_rm_send = uint16_t(clen - remn);
	}
	
	// Init_RI を送信したことを記録しておく
	precd_SPRS->mb_InitRI = false;

	this->Make_AsioWrtBuf_with_PayloadHdr(pdata_payload, pbuf_large - pdata_payload);
	return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Write;
}

// -------------------------------------------------------------

KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr  KClient_Chat::UP_Crt_Usr(
								uint16_t* const c_pdata_payload, const size_t cbytes_payload)
{
	if (m_pUInfo_Elmt)
	{
		// 既にユーザ登録をしている場合、不正接続とみなして即クローズさせる
		return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Close;
	}

	// 遅延リクエストのチェック（まず、発生することはないはずだけど、、）
	if (m_rrq_time_InitRI_CrtUsr)
	{
		if (m_time_WS_Read_Hndlr < m_rrq_time_InitRI_CrtUsr)
		{
			// 規定の時間より早く再リクエストを送ってきた場合は、不正クラアントとして切断する
			return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Close;
		}
		m_rrq_time_InitRI_CrtUsr = 0;
	}

	// 通信エラーのチェック（peyload の長さのチェック）
///===DEBUG===///
cout << "KClient_Chat::WS_Read_Hndlr: RECEIVE EN_UP_Crt_Usr" << endl;
	if ((*(c_pdata_payload + 1) << 1) != cbytes_payload)
	{
///===DEBUG===///
cout << "failed: bytes_payload is NOT expected value" << endl;
		return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Close;
	}

	// ----------------------------------
	// ユーザ登録を開始する
	m_pUInfo_Elmt = KUInfo::ms_List.Get_NewElmt();
	if (m_pUInfo_Elmt == NULL)
	{
		// まず、ユーザ登録数が上限を超えることはないと思うけれど、、、
		g_glog.WriteID_with_UnixTime(N_LogID::EN_KUInfo_Exausted);

		m_rrq_time_InitRI_CrtUsr
			= m_time_WS_Read_Hndlr + N_JSC::EN_SEC_Wait_Crt_Usr - N_JSC::EN_SEC_Margin_Force_toCLOSE;

		this->Set_AsioWrtBuf_with_PLHdr_to_RRQ(N_JSC::EN_DN_Crt_Usr | N_JSC::EN_BUSY_WAIT_SEC
												, N_JSC::EN_SEC_Wait_Crt_Usr);
		return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Write;
	}

	// KUInfo::ms_List から、新規要素を取得できたため、新規ユーザ登録を実行する
	// ----------------------------------
	// m_uID の設定
///===SuperUser設定を見直すこと===///
	if (m_pKClnt->m_precd_SPRS == NULL)
	{ return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Close; }

	if (m_pKClnt->m_precd_SPRS->mb_IsDev)
	{
		m_pUInfo_Elmt->m_uID = KUInfo::Crt_New_SuperUsrID();
///===DEBUG===///
cout << "succeeded: regist by SUPER User ID" << endl;
	}
	else
	{
		m_pUInfo_Elmt->m_uID = KUInfo::Crt_New_UsrID();
///===DEBUG===///
cout << "succeeded: regist by NORMAL User ID" << endl;
	}
	
	// ----------------------------------
	// m_uview の設定
	m_pUInfo_Elmt->m_uview = *(uint32_t*)(c_pdata_payload + 4);

	// ----------------------------------
	// uname & bytes の設定
	// +6: コマンド１、コンテナサイズ１、reserved２、uview２
	// 上の個数は、uint16 で数えた個数であるから、bytes で考えると x2 で -12
	memcpy(m_pUInfo_Elmt->ma_uname, c_pdata_payload + 6, cbytes_payload - 12);
	// m_bytes_KUInfo = bytes of uname + 10
	m_pUInfo_Elmt->m_bytes_KUInfo = uint16_t(cbytes_payload - 2);

	// ----------------------------------
	// uID を生成したことを通知
	*c_pdata_payload = N_JSC::EN_DN_Crt_Usr;
	*(uint32_t*)(c_pdata_payload + 1) = m_pUInfo_Elmt->m_uID;
	this->Make_AsioWrtBuf_with_PayloadHdr((uint8_t*)c_pdata_payload, 6);

	// 新しくユーザ登録ができたため、ログに記録しておく

	return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Write;
}
