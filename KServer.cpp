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

extern KClient_Chat_Intf*  G_GetNext_KClnt_Chat();

KServer::KServer(asio::io_context& rioc, tcp::acceptor& raccptr)
	: mr_ioc{ rioc }
	, mr_accptr{ raccptr }
{
	{
		// ----------------------------------
		// 以下は改良の余地あり（本来は Client のコンストラクタでやるべきこと）
		KSmplListElmt<KClient>*  pkclnt_elmt = m_KClntList.m_pBegin;
		for (int i = 0; i < NUM_CLIENT; ++i)
		{
			pkclnt_elmt->m_pSvr = this;
			pkclnt_elmt->m_pThis_byListElmt = pkclnt_elmt;
			pkclnt_elmt->m_sckt_ID = m_next_sckt_ID;

///===DEVELOPMENT===///
			KClient_Chat_Intf*  pKClnt_Chat_Intf = ::G_GetNext_KClnt_Chat();
			pKClnt_Chat_Intf->m_pKClnt = pkclnt_elmt;
			pkclnt_elmt->m_pKClnt_Chat = pKClnt_Chat_Intf;

			pkclnt_elmt = pkclnt_elmt->m_pNext;
			m_next_sckt_ID++;
		}
		// ----------------------------------
	}
}

// -------------------------------------------------------------

void  KServer::Run()
{
	m_pKClnt_Elmt_UnderAccpting = m_KClntList.Get_NewElmt();
	mr_accptr.async_accept(m_pKClnt_Elmt_UnderAccpting->m_socket,
						boost::bind(&KServer::On_Accpt_Svr, this, asio::placeholders::error));
}

// -------------------------------------------------------------

void  KServer::On_Accpt_Svr(const boost::system::error_code& crerr)
{
	// pKClnt_onAccpted が、新規接続を受け取った KClient
	KSmplListElmt<KClient>* const  c_pKClnt_Elmt_onAccpted = m_pKClnt_Elmt_UnderAccpting;

	// 次のクライアントの Accept 準備を行う
	m_pKClnt_Elmt_UnderAccpting = m_KClntList.Get_NewElmt();
	if (m_pKClnt_Elmt_UnderAccpting != NULL)
	{
		// 次のクライアントのための接続受け入れを開始させる
		mr_accptr.async_accept(m_pKClnt_Elmt_UnderAccpting->m_socket,
						boost::bind(&KServer::On_Accpt_Svr, this, asio::placeholders::error));
	}
	else
	{
		// Accept待機できる KClient が存在しない場合、
		// m_pKClnt_Elmt_UnderAccpting == NULL となり、async_accept() は発行されない
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_KClient_Exausted);
	}

///===DBG===///
std::cout << "KServer::On_Accpt_Svr() ▶ ID: " << c_pKClnt_Elmt_onAccpted->m_sckt_ID;
	if (crerr)  // ここでエラーが発生することは想定できないけど、、、
	{
///===DBG===///
std::cout << ", async_accept FAILED: ▶▶▶" << crerr.message() << std::endl;

		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_on_Async_Accept);
		this->Recycle_Clnt(c_pKClnt_Elmt_onAccpted);
		return;  // エラーの場合, pKClnt_Elmt_onAccpted の接続は破棄され、リサイクルに回される
	}
///===DBG===///
std::cout << ", accept succeeded." << std::endl;


	// -------------------------------------
	// まず、多重接続のチェックを行う
	boost::system::error_code  ec;
	const asio::ip::tcp::endpoint  cremote_ep = c_pKClnt_Elmt_onAccpted->m_socket.remote_endpoint(ec);
	if (ec)  // ここでエラーが発生することは想定できないけど、、、
	{
///===DBG===///
std::cout << "remote_endpoint() FAILED: ▶▶▶" << ec.message() << std::endl;

		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_Get_remote_endpoint);
		this->Recycle_Clnt(c_pKClnt_Elmt_onAccpted);
		return;  // エラーの場合、pKClnt_onAccpted の接続は破棄され、リサイクルに回される
	}

	KRecd_SPRS* const  precd_SPRS = KSPRS_rld::Chk_IP(cremote_ep);
	if (precd_SPRS == NULL)
	{
		// KRecd_SPRS のリソースが尽きた場合（ログは KSPRS_rld::Chk_IP() にて記録済み）
		// pKClnt_onAccpted の接続は破棄され、リサイクルに回される
		this->Recycle_Clnt(c_pKClnt_Elmt_onAccpted);
		return;
	}

	if (precd_SPRS->m_times_cnct > N_SPRS::EN_Limit_Cnct)
	{
		// この場合は DoS攻撃も考えられるため、速やかに c_pKClnt_Elmt_onAccpted を解放させる
		if (precd_SPRS->m_pKClient)
		{ this->Recycle_Clnt(precd_SPRS->m_pKClient->m_pThis_byListElmt); }

		this->Recycle_Clnt(c_pKClnt_Elmt_onAccpted);
		return;
	}
	// ----------------------------------
	// 以降は precd_SPRS->m_times_cnct <= N_SPRS::EN_Limit_Cnct であり、DoS攻撃の可能性の低い接続

	// ファイルディスクリプタが設定された後でないと、バッファサイズが指定できなようなので、
	// この段階でバッファサイズを設定し直す
	{
		boost::asio::socket_base::receive_buffer_size  opt_rcv_set(KClient::BYTES_Sckt_Buf_RECEIVE);
		c_pKClnt_Elmt_onAccpted->m_socket.set_option(opt_rcv_set);

		boost::asio::socket_base::send_buffer_size  opt_send_set(KClient::BYTES_Sckt_Buf_SEND);
		c_pKClnt_Elmt_onAccpted->m_socket.set_option(opt_send_set);
	}

	// 以前に接続状態にあった KClient があれば、以前のものはリサイクルに回し、pKClnt_onAccpted に切り替える
	// さらに、以前に uname の登録などをしていれば、その情報を引き継ぐ
	if (precd_SPRS->m_pKClient)
	{
		KClient* const  c_pPre_KClient = precd_SPRS->m_pKClient;
		if (c_pPre_KClient->m_pKUInfo_Elmt)
		{
			// SPRS監視期間中、既に uname を持っていたら、それを引き継ぐ
			c_pKClnt_Elmt_onAccpted->m_pKUInfo_Elmt = c_pPre_KClient->m_pKUInfo_Elmt;
			c_pPre_KClient->m_pKUInfo_Elmt = NULL;
		}

		// Recycle_Clnt() 内で、precd_SPRS->m_pKClient = NULL に設定されるため、後で設定し直す必要がある
		this->Recycle_Clnt(c_pPre_KClient->m_pThis_byListElmt);
	}

	// pKClnt_onAccpted が監視対象になったことを設定
	precd_SPRS->m_pKClient = c_pKClnt_Elmt_onAccpted;
	c_pKClnt_Elmt_onAccpted->m_precd_SPRS = precd_SPRS;

	// ここまでで、多重接続のチェックは終了
	// -------------------------------------

	// WebSocket の構築を許可する
	// IP アドレスをコピーしておく（g_glog へ記録は、ユーザー名が設定されたときに行う）
	{
		if (precd_SPRS->mb_Is_v4)
		{
			c_pKClnt_Elmt_onAccpted->mb_Is_v4 = true;
			c_pKClnt_Elmt_onAccpted->mu_IP.m_IPv4 = precd_SPRS->mu_KIP.m_v4.m_IPv4;
		}
		else
		{
			c_pKClnt_Elmt_onAccpted->mb_Is_v4 = false;
			c_pKClnt_Elmt_onAccpted->mu_IP.ma_IPv6[0] = precd_SPRS->mu_KIP.m_v6.m_IPv6_up;
			c_pKClnt_Elmt_onAccpted->mu_IP.ma_IPv6[1] = precd_SPRS->mu_KIP.m_v6.m_IPv6_down;
		}
	}

	// 今のところ、SuperUser 判定は、DevIP のみ
	// 今後は、ログイン時のパスワードなどの設定で, SuperUser フラグを ON にする
	if (precd_SPRS->mb_IsDev)
	{ c_pKClnt_Elmt_onAccpted->mb_IsSuperUser = true; }

	c_pKClnt_Elmt_onAccpted->Crt_WebScktConnection();
}

// -------------------------------------------------------------

void  KServer::Recycle_Clnt(KSmplListElmt<KClient>* const pkclnt_elmt_abort)
{
	// m_sckt_ID の割り振りがあるため、ソケットの処理は KServer で行うことにする
	if (pkclnt_elmt_abort->m_socket.is_open())
	{
		boost::system::error_code  ec;
		pkclnt_elmt_abort->m_socket.close(ec);
	}

	// 新しいファイルディスクリプタなどの割当をするために、ソケットを生成し直す
	{
		tcp::socket  disposal_sckt(std::move(pkclnt_elmt_abort->m_socket));
		tcp::socket  new_sckt{ *::g_pioc };
		pkclnt_elmt_abort->m_socket = std::move(new_sckt);

		// pkclnt_elmt_abort のソケットは新しいものに変わった
		pkclnt_elmt_abort->m_sckt_ID = m_next_sckt_ID;
		m_next_sckt_ID++;
	}

	// KClient 内の情報を、初期設定に戻す
	pkclnt_elmt_abort->Clean();

	// pkclnt_elmt_abort を破棄
	m_KClntList.MoveToEnd(pkclnt_elmt_abort);

	// -------------------------------------
	// 空きの Client がなくて、async_accept() が発行されていなかった場合の処理
	if (m_pKClnt_Elmt_UnderAccpting == NULL)
	{
		// m_pKClnt_CurAccpt == pclnt_recycled となるはず
		m_pKClnt_Elmt_UnderAccpting = m_KClntList.Get_NewElmt();

		mr_accptr.async_accept(m_pKClnt_Elmt_UnderAccpting->m_socket,
						boost::bind(&KServer::On_Accpt_Svr, this, asio::placeholders::error));
	}
}

