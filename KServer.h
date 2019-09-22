#pragma once

#include "KClient.h"

class  KServer
{
public:
	// 同時接続数が以下を超えた場合、async_accept() も実行されなくなる
	// Recycle_Clnt() により、KClient に余裕が出ると、自動的に async_accept() の実行が開始される
	enum  { NUM_CLIENT = 2000 };

	// 唯一のコンストラクタ
	KServer(boost::asio::io_context& rioc, boost::asio::ip::tcp::acceptor& raccptr);

	void  Run();
	void  On_Accpt_Svr(const boost::system::error_code& err);
	void  Recycle_Clnt(KSmplListElmt<KClient>* const pclnt_abort);

	boost::asio::io_context&  mr_ioc;
	boost::asio::ip::tcp::acceptor&  mr_accptr;
	KSmplList<KSmplListElmt<KClient>, NUM_CLIENT>  m_KClntList;

	// 空きの KClient がない場合、m_pKClnt_CurAccpt は NULL に設定される
	KSmplListElmt<KClient>*  m_pKClnt_CurAccpt = NULL;

	// ソケットが何個作成されたかをカウントする（エラー発生時の記録として利用しているのみ）
	int  m_next_sckt_ID = 1;
};
