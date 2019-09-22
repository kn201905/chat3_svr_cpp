#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "__common/_KString.h"
#include "__common/LogID.h"
#include "KgLog/src/KgLog.h"

#include "KServer.h"
#include "KSPRS_rld.h"


namespace asio = boost::asio;
using asio::ip::tcp;

extern KgLog  g_glog;


////////////////////////////////////////////////////////////////

KServer::KServer(asio::io_context& rioc, tcp::acceptor& raccptr)
	: mr_ioc{ rioc }
	, mr_accptr{ raccptr }
{
	{
		// ----------------------------------
		// 以下は改良の余地あり（本来は Client のコンストラクタでやるべきこと）
		KSmplListElmt<KClient>*  pcur = m_KClntList.m_pBegin;
		for (int i = 0; i < NUM_CLIENT; ++i)
		{
			pcur->m_pSvr = this;
			pcur->m_pThis_byListElmt = pcur;
			pcur->m_sckt_ID = m_next_sckt_ID;

///===DEVELOPMENT===///
			KClient_Chat_Intf*  pClnt_Chat_Intf = ::G_GetNext_Clnt_Chat();
			pClnt_Chat_Intf->m_pKClnt = pcur;
			pcur->m_pClnt_Chat = pClnt_Chat_Intf;

			pcur = pcur->m_pNext;
			m_next_sckt_ID++;
		}
		// ----------------------------------
	}
}

// -------------------------------------------------------------

void  KServer::Run()
{
	m_pKClnt_CurAccpt = m_KClntList.GetNext();
	mr_accptr.async_accept(m_pKClnt_CurAccpt->m_socket,
						boost::bind(&KServer::On_Accpt_Svr, this, asio::placeholders::error));
}

// -------------------------------------------------------------

void  KServer::On_Accpt_Svr(const boost::system::error_code& crerr)
{
	// ----------------------------------
	// ファイルディスクリプタが設定された後でないと、バッファサイズが指定できなようなので、
	// この段階でバッファサイズを設定し直す
	boost::asio::socket_base::receive_buffer_size  opt_rcv_set(KClient::BYTES_Sckt_Buf_RECEIVE);
	m_pKClnt_CurAccpt->m_socket.set_option(opt_rcv_set);

	boost::asio::socket_base::send_buffer_size  opt_send_set(KClient::BYTES_Sckt_Buf_SEND);
	m_pKClnt_CurAccpt->m_socket.set_option(opt_send_set);
	// ----------------------------------

	// pKClnt_onAccpted が、新規接続を受け取った KClient
	KSmplListElmt<KClient>*  pKClnt_onAccpted =  m_pKClnt_CurAccpt;

	m_pKClnt_CurAccpt = m_KClntList.GetNext();
	if (m_pKClnt_CurAccpt != NULL)
	{
		// 次のクライアントのための接続受け入れを開始させる
		mr_accptr.async_accept(m_pKClnt_CurAccpt->m_socket,
						boost::bind(&KServer::On_Accpt_Svr, this, asio::placeholders::error));
	}
	else
	{
		// Accept待機できる KClient が存在しない場合、
		// m_pKClnt_CurAccpt == NULL となり、async_accept() は発行されない
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_KClient_Exausted);
	}

///===DBG===///
std::cout << "KServer::On_Accpt_Svr() ▶ ID: " << pKClnt_onAccpted->m_sckt_ID;
	if (crerr)  // ここでエラーが発生することは想定できないけど、、、
	{
///===DBG===///
std::cout << ", async_accept failed: " << crerr.message() << std::endl;

		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_on_Async_Accept);
		this->Recycle_Clnt(pKClnt_onAccpted);
		return;  // エラーの場合、pKClnt_onAccpted の接続は破棄され、リサイクルに回される
	}

	// エラーなく接続を受け付けた場合の処理（DoS攻撃を想定して、async_accept() の段階ではログは残さない）
///===DBG===///
std::cout << ", accept succeeded." << std::endl;

	// -------------------------------------
	// まず、多重接続のチェックを行う
	boost::system::error_code  ec;
	const asio::ip::tcp::endpoint  cremote_ep = pKClnt_onAccpted->m_socket.remote_endpoint(ec);
	if (ec)  // ここでエラーが発生することは想定できないけど、、、
	{
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_Get_remote_endpoint);
		this->Recycle_Clnt(pKClnt_onAccpted);
		return;  // エラーの場合、pKClnt_onAccpted の接続は破棄され、リサイクルに回される
	}

	KRecd_SPRS* const  precd_SPRS = KSPRS_rld::Chk_IP(cremote_ep);
	if (precd_SPRS == NULL)
	{
		// KRecd_SPRS のリソースが尽きた場合（ログは KSPRS_rld::Chk_IP() にて記録済み）
		// pKClnt_onAccpted の接続は破棄され、リサイクルに回される
		this->Recycle_Clnt(pKClnt_onAccpted);
		return;
	}

	// 以前に接続状態にあった KClient があれば、以前のものはリサイクルに回し、pKClnt_onAccpted に切り替える
	if (precd_SPRS->m_pKClient)
	{
		// Recycle_Clnt() 内で、precd_SPRS->m_pKClient = NULL に設定される
		this->Recycle_Clnt(precd_SPRS->m_pKClient->m_pThis_byListElmt);
	}

	// pKClnt_onAccpted が監視対象になったことを設定
	precd_SPRS->m_pKClient = pKClnt_onAccpted;
	pKClnt_onAccpted->m_precd_SPRS = precd_SPRS;

	if (precd_SPRS->m_times_cnct > N_SPRS::EN_Limit_Cnct)
	{
		// 接続回数が制限を超えている場合は、async_read() は発行せず、即 KClient を解放させる
		this->Recycle_Clnt(pKClnt_onAccpted);
		return;
	}
	// ここまでで、多重接続のチェックは終了
	// -------------------------------------

	// WebSocket の構築を許可する
	// IP アドレスを書き写す（g_glog へ記録は、ユーザー名が設定されたときに行う）
	{
		if (precd_SPRS->mb_Is_v4)
		{
			pKClnt_onAccpted->mb_Is_v4 = true;
			pKClnt_onAccpted->mu_IP.m_IPv4 = precd_SPRS->mu_KIP.m_v4.m_IPv4;
		}
		else
		{
			pKClnt_onAccpted->mb_Is_v4 = false;
			pKClnt_onAccpted->mu_IP.ma_IPv6[0] = precd_SPRS->mu_KIP.m_v6.m_IPv6_up;
			pKClnt_onAccpted->mu_IP.ma_IPv6[1] = precd_SPRS->mu_KIP.m_v6.m_IPv6_down;
		}
	}

	pKClnt_onAccpted->Crt_WebScktConnection();
}

// -------------------------------------------------------------

void  KServer::Recycle_Clnt(KSmplListElmt<KClient>* const pclnt_recycled)
{
	if (pclnt_recycled->m_socket.is_open())
	{
		boost::system::error_code  ec;
		pclnt_recycled->m_socket.close(ec);
	}

	// 新しいファイルディスクリプタなどの割当をするために、ソケットを生成し直す
	{
		tcp::socket  disposal_sckt(std::move(pclnt_recycled->m_socket));
		tcp::socket  new_sckt{ *::g_pioc };
		pclnt_recycled->m_socket = std::move(new_sckt);
	}

	// 万一 write buffer を使っていた場合、ここで閉じる
	pclnt_recycled->m_pClnt_Chat->Reset_prvt_buf_write();

	m_KClntList.MoveToEnd(pclnt_recycled);
	pclnt_recycled->m_sckt_ID = m_next_sckt_ID;
	m_next_sckt_ID++;

	// SPRS の監視対象になっていた場合、Recycle されたことを通知する
	if (pclnt_recycled->m_precd_SPRS)
	{
		pclnt_recycled->m_precd_SPRS->m_pKClient = NULL;
		pclnt_recycled->m_precd_SPRS = NULL;
	}

	pclnt_recycled->m_stt_cur = KClient::EN_STATUS::EN_No_WS;

	// 空きの Client がなくて、async_accept() が発行されていない場合の処理
	if (m_pKClnt_CurAccpt == NULL)
	{
		// m_pKClnt_CurAccpt == pclnt_recycled となるはず
		m_pKClnt_CurAccpt = m_KClntList.GetNext();

		mr_accptr.async_accept(m_pKClnt_CurAccpt->m_socket,
						boost::bind(&KServer::On_Accpt_Svr, this, asio::placeholders::error));
	}
}

