#include <iostream>
//#include <thread>
//#include <mutex>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "__common/_KString.h"
#include "__common/LogID.h"

#include "KgLog/src/KgLog.h"
#include "KSPRS_rld.h"
#include "KServer.h"

// WebSocket RFC 6455
// https://triple-underscore.github.io/RFC6455-ja.html#section-7.1.1
// mdn
// https://developer.mozilla.org/ja/docs/WebSockets-840092-dup/Writing_WebSocket_servers


// key24chr_* に 24 bytes の websocket-key を設定すると、o_pbase64 に 28文字の accept と、\r\n\r\n が付加される
// o_pbase64 は、32 bytes のバッファを指していること
// SHA1_r1.o をリンクすること
extern "C" void sha1_update_intel(char* o_pbase64, uint64_t key24chr_1, uint64_t key24chr_2, uint64_t key24chr_3);
// アプリケーションの起動時に、一度だけ以下のルーチンをコールしておくこと
extern "C" void  sha1_init_once();


////////////////////////////////////////////////////////////////////////////////

namespace  asio = boost::asio;
using  boost::asio::ip::tcp;

enum  { NUM_LISTEN_PORT = 1234 };

namespace  N_gLog
{
	enum
	{
		EN_bytes_buf = 50 * 1000,
		EN_max_bytes_msg = 500,
		EN_bytes_file = 2 * 1'000'000,
		EN_bytes_fflush = 10 * 1000
	};
	constexpr char CE_str_fbody[] = "log_main";
}

KgLog  g_glog{ N_gLog::EN_bytes_buf, N_gLog::EN_max_bytes_msg, N_gLog::CE_str_fbody
				, N_gLog::EN_bytes_file, N_gLog::EN_bytes_fflush, KgLog::EN_FileType::Binary };

#define  GLOG_BIN_WriteStr_withUT(ce_str)  g_glog.WriteID_with_UnixTime(N_LogID::EN_STR, ce_str.mc_cpstr, ce_str.mc_len)

////////////////////////////////////////////////////////////////////////////////

asio::io_context*  g_pioc;

int main()
{
	{
		constexpr KCnstString  ce_str{ "main: start" };
		GLOG_BIN_WriteStr_withUT(ce_str);
	}

	// -------------------------------------
	// 基幹オブジェクトの生成、準備
	sha1_init_once();

	// concurrency_hint に「１」を指定
	asio::io_context  ioc{ 1 };
	::g_pioc = &ioc;
	auto  work = boost::asio::make_work_guard(ioc);
	tcp::acceptor  accptr{ ioc, tcp::endpoint(tcp::v4(), ::NUM_LISTEN_PORT) };

	// KServer が構築されるときに、KClient が構築される。 KClient は、メンバ変数に m_socket{ *::g_pioc } を持っているため、
	// g_pioc が生成された後に KServer が構築される必要がある。
	KServer  chat_svr{ ioc, accptr };

	// -------------------------------------
	for (auto i : { SIGINT /*, SIGHUP , SIGTERM */ })
	{
		std::signal( i,
			[](int) {
				std::cout << "detect SIGINT, call g_pioc->stop()..." << std::endl;
//				constexpr KCnstString  ce_str{ "SIGINT is called." };
//				GLOG_BIN_WriteStr_withUT(ce_str);

				::g_pioc->stop();
			}
		);
	}


// -------------------------------------
// テストコード
	KMng_DevIP::Push("192.168.0.50");
	KMng_DevIP::Push("127.0.0.1");
	KMng_DevIP::Push("192.168.0.52");
	KMng_DevIP::Push("2001:200:dff:fff1:216:3eff:feb1:44d7");
//	KMng_DevIP::LogFlush();

#if false
	bool  bIsDevIP;
	asio::ip::address_v4  test_1 = asio::ip::make_address_v4("192.168.0.12");
	asio::ip::tcp::endpoint  ep_1{ test_1, 1234 };
	KSPRS_rld::Chk_IP(ep_1, &bIsDevIP);

	asio::ip::address_v6  test_2 = asio::ip::make_address_v6("2001:200:dff:fff1:216:3eff:feb1:44d8");
	asio::ip::tcp::endpoint  ep_2{ test_2, 1234 };
	KSPRS_rld::Chk_IP(ep_2, &bIsDevIP);

	asio::ip::address_v6  test_3 = asio::ip::make_address_v6("1:12:123:1234:0:3eff:feb1:44d8");
	asio::ip::tcp::endpoint  ep_3{ test_3, 1234 };
	KSPRS_rld::Chk_IP(ep_3, &bIsDevIP);

	KSPRS_rld::Chk_IP(ep_1, &bIsDevIP);

	asio::ip::address_v4  test_4 = asio::ip::make_address_v4("192.168.0.13");
	asio::ip::tcp::endpoint  ep_4{ test_4, 1234 };
	KSPRS_rld::Chk_IP(ep_4, &bIsDevIP);

	asio::ip::address_v4  test_5 = asio::ip::make_address_v4("192.168.0.14");
	asio::ip::tcp::endpoint  ep_5{ test_5, 1234 };
	KSPRS_rld::Chk_IP(ep_5, &bIsDevIP);

	asio::ip::address_v4  test_dev1 = asio::ip::make_address_v4("127.0.0.1");
	asio::ip::tcp::endpoint  ep_dev1{ test_dev1, 1234 };
	KSPRS_rld::Chk_IP(ep_dev1, &bIsDevIP);

	asio::ip::address_v4  test_6 = asio::ip::make_address_v4("12.34.56.78");
	asio::ip::tcp::endpoint  ep_6{ test_6, 1234 };
	for (int i = 0; i < 200; ++i)
	{ KSPRS_rld::Chk_IP(ep_6, &bIsDevIP); }

	asio::ip::address_v6  test_7 = asio::ip::make_address_v6("12:123:1234:0:a:ab:abc:abcd");
	asio::ip::tcp::endpoint  ep_7{ test_7, 1234 };
	for (int i = 0; i < 200; ++i)
	{ KSPRS_rld::Chk_IP(ep_7, &bIsDevIP); }

	KSPRS_rld::DBG__List_recds_SPRS();
#endif
// -------------------------------------

//	KMng_WSckts::Init_Once();


#if false
	asio::spawn(*sp_ioc, [sp_ioc](asio::yield_context yield) {
		G_cout_ThID("main: spawn start");

		tcp::acceptor acceptor(*sp_ioc, tcp::endpoint(tcp::v4(), 1234));
        while (true) {
            tcp::socket socket(*sp_ioc);

			G_cout_ThID("main: pre async_accept");
            acceptor.async_accept(socket, yield);
			G_cout_ThID("main: post async_accept");

            asio::spawn(*sp_ioc,
				[&](asio::yield_context yc) {
					Detect_NewCnctn(yc, std::move(socket));
				}
			);
			G_cout_ThID("main: while loop end");
        }
	});
	G_cout_ThID("main: post spawn");
#endif

//	boost::this_thread::sleep( boost::posix_time::seconds( 3 ) );

	chat_svr.Run();
	{
		constexpr KCnstString  ce_str{ "main: chat_svr.Run()" };
		GLOG_BIN_WriteStr_withUT(ce_str);
	}

///===DBG===///
std::cout << "main: called chat_svr.Run()" << std::endl;

	ioc.run();  // イベントループを起動
///===DBG===///
std::cout << "main: exited ioc.run()" << std::endl;

	{
		constexpr KCnstString  ce_str{ "main: exited ioc.run()" };
		GLOG_BIN_WriteStr_withUT(ce_str);
	}
	g_glog.Signal_ThrdStop();

	return  0;
}


/*
asio::deadline_timer timer; 
timer.expires_from_now(boost::posix_time::seconds(10));
timer.async_wait(
    [&socket_](const boost::system::error_code &ec) {
        if (ec == boost::asio::error::operation_aborted) {
            // キャンセルされた場合
            cout << “cancel” << endl ;
        } else {
            // タイムアウト時はソケットをキャンセルし readをやめる
            socket_.cancel();
            cout << “timeout” << endl ;
        }            
    });
async_read(socket_, boost::asio::buffer(buffer_), yield);
// readが先におわったので、タイマーをキャンセル
timer.cancel();
*/
