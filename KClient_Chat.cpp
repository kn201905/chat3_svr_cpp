#include <iostream>

#include <stdint.h>
#include <stddef.h>
#include <boost/asio.hpp>

#include "KServer.h"
#include "KClient_Chat.h"
#include "KSPRS_rld.h"  // InitRI の許可判断に必要

#include "__common/LogID.h"
#include "KgLog/src/KgLog.h"


extern KgLog  g_glog;

using  std::cout;
using  std::endl;


KSmplList2<KRI, KRI::EN_NUM_KRI>  KRI::ms_List;

////////////////////////////////////////////////////////////////

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

KClient_Chat  ga_KClnt_Chat[KServer::NUM_CLIENT];
static KClient_Chat*  s_pKClnt_Chat_onGetNext = ga_KClnt_Chat;

KClient_Chat_Intf*  G_GetNext_Clnt_Chat()
{
	KClient_Chat_Intf*  ret_val = s_pKClnt_Chat_onGetNext;
	s_pKClnt_Chat_onGetNext++;
	return  ret_val;
}

////////////////////////////////////////////////////////////////

void  KClient_Chat::Reset_prvt_buf_write()
{
	if (m_pInfo_LargeBuf)
	{
		KLargeBuf::Return_Buf(m_pInfo_LargeBuf);
		m_pInfo_LargeBuf = NULL;
	}
}

// -------------------------------------------------------------
// 受信したものに対してレスポンスを帰す場合は、KClient::m_asio_prvt_buf_write に値を設定して、EN_Write を返す

KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr  KClient_Chat::WS_Read_Hndlr(uint16_t* pdata_payload, size_t bytes_payload)
{
	m_time_WS_Read_Hndlr = ::time(NULL);

	const uint16_t  cop = *pdata_payload++;
	switch (cop)
	{
		case  N_JSC::EN_UP_Init_RI:
			cout << "RECEIVE Init_RI" << endl;
			return  this->Rcv_InitRI();
		
		default:
			cout << "RECEIVE Unknown OP" << endl;
	}

	return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Raad;
//	return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Write;
}

// -------------------------------------------------------------

void  KClient_Chat::SetPayload_RRQ(uint16_t rereq_jsc, uint16_t sec_wait)
{
	this->Reset_prvt_buf_write();  // 念の為

	uint8_t*  pbuf = m_pKClnt->ma_ui8_prvt_buf;
	m_pKClnt->m_asio_prvt_buf_write.data_ = pbuf;
	m_pKClnt->m_asio_prvt_buf_write.size_ = 6;

	// ペイロードヘッダを書き込む
	*pbuf++ = 0x82;  // FIN: 0x80, バイナリフレーム 0x2
	*pbuf++ = 0x04;  // ペイロード長 4bytes: 遅延対象となったコマンド（rereq_jsc）+ sec_wait
	*(uint16_t*)pbuf = rereq_jsc;
	pbuf += 2;
	*(uint16_t*)pbuf = sec_wait;
}

// -------------------------------------------------------------

KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr  KClient_Chat::Rcv_InitRI()
{
	static_assert(sizeof(KUInfo) == 30, "KUInfo");

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
	if (m_rrq_time_InitRI)
	{
		if (m_time_WS_Read_Hndlr < m_rrq_time_InitRI)
		{
			// 規定の時間より早く再リクエストを送ってきた場合は、不正クラアントとして切断する
			return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Close;
		}
		m_rrq_time_InitRI = 0;
	}

	// ----------------------------------
	// Large バッファの準備をする
	KLargeBuf::KInfo* const  pbufInfo = KLargeBuf::ReqBuf_Single();
	if (pbufInfo == NULL)
	{
		// バッファの準備に失敗した場合は、JSクライアントに再送要求を出すように指示をする
		m_rrq_time_InitRI = m_time_WS_Read_Hndlr + N_JSC::EN_SEC_Wait_Init_RI - N_JSC::EN_SEC_Margin_Force_toCLOSE;
		this->SetPayload_RRQ(N_JSC::EN_DN_Init_RI | N_JSC::EN_BUSY_WAIT_SEC, N_JSC::EN_SEC_Wait_Init_RI);
		return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Write;
	}

	// Large バッファに返信内容を書き込んでいく
	// 特別な資格がないクライアントに対しては、最大バイト数は EN_BYTES_BUF とする。



	return  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Write;
}
