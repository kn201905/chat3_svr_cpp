#pragma once

#include "KSmplList.hpp"

extern boost::asio::io_context*  g_pioc;

class  KServer;
class  KClient;
class  KRecd_SPRS;

////////////////////////////////////////////////////////////////
// KClient_Chat_Intf は、本来不必要なクラス。コンパイル時間を短縮するためだけに使っているもの
// KClient_Chat.h を include しなくてもコンパイルできるように作成したクラス
// リリース版では、KClient_Chat_Intf を削除すること

///===DEVELOPMENT===///
class  KClient_Chat_Intf
{
	friend class  KServer;

public:
	enum class  EN_Ret_WS_Read_Hndlr { EN_Write, EN_Read, EN_Close };

	// 戻り値： write が必要な場合 true が返される
	// write が必要な場合は、m_asio_prvt_buf_write に設定を書き込んでおくこと
	virtual EN_Ret_WS_Read_Hndlr  WS_Read_Hndlr(uint16_t* ppayload, size_t bytes_payload) =0;
	// prvt_buf_write が Large になっていたら、元に戻す。Small の状態であったならば何もしない
	// コールされるのは以下の２ヶ所
	// 1) On_Async_Write_Hndlr_asWS()
	// 2) m_pClnt_Chat->WS_Read_Hndlr() == EN_CLOSE のとき
	virtual void  Reset_prvt_buf_write() =0;

	// バッファを解放したりする
	virtual void  Clean_KClntChat() =0;

	// KServer のコンストラクタで設定される
	// コード実行中、不変。開発中のコンパイル時間を短縮するためだけに利用するメンバ変数
	KClient*  m_pKClnt = NULL;
};


////////////////////////////////////////////////////////////////
// KClient が、uID を受け取ると KKUInfo が生成される

struct  KUInfo  // paddingなしで 30 bytes（KSmplList2 で 46 bytes）
{
	enum  {
		EN_NUM_KUInfo = 3000,  // 46 bytes * 3000 = 138 kB

		EN_MAX_LEN_uname = 10,
	};

	// データ送信時に、memcpy で利用される。また、ユーザ名の文字数も算出できる
	// Clean() により、m_bytes_KUInfo と m_uID が 0 に設定される（一応の措置。意味ないかも）
	uint16_t  m_bytes_KUInfo = 0;  // KUInfo 全体のバイト数（利用している部分のみ）
	uint32_t  m_uID = 0;
	uint32_t  m_uview = 0;
	uint16_t  ma_uname[EN_MAX_LEN_uname] = {};

	// --------------------------------------------
	static KSmplList2<KUInfo, EN_NUM_KUInfo>  ms_List;

	static uint32_t  Crt_New_SuperUsrID();
	static uint32_t  Crt_New_UsrID();
} __attribute__ ((gcc_struct, packed));  // memcpy を利用するため、packed にしている

using  KUInfo_Elmt = KSmplListElmt<KUInfo>;


////////////////////////////////////////////////////////////////

class  KClient
{
///===DEVELOPMENT===///
	// リリース版では、KClient_Chat と KClient を統合すること（開発中はコンパイル時間を短縮させるため分離している）
	friend class  KClient_Chat;

public:
	// Apache では 8KB をデフォルトとしているらしい
	// 最小で 150 bytes あたり、Firefoxでは 450 bytes 程度のヘッダが送られてきた
	enum  {
		BYTES_PRIVATE_BUF = 1500,
		// -8: ペイロードヘッダ / -7: 64bits 単位でマスク処理を行うため、最大７バイトはみ出す
		BYTES_MAX_Payload = BYTES_PRIVATE_BUF - 8 - 7,

		BYTES_Sckt_Buf_RECEIVE = 4096,
		BYTES_Sckt_Buf_SEND = 16384,
	};

	// 以下の範囲にないヘッダが送られてきたら無効とする
	enum { EN_MIN_BYTES_http_hdr = 120, EN_MAX_BYTES_http_hdr = 1000 };

	enum class  EN_STATUS { EN_No_WS, EN_Async_Read, EN_Async_Write };

	// -----------------------------------------------
	friend class  KServer;
	friend int  S_Trim(time_t ctime_trim, KRecd_SPRS* const cprecd_top, const KRecd_SPRS* const cprecd_tmnt
						, KRecd_SPRS** const cpprecd_dty, KRecd_SPRS* const cprecd_next);

	// -----------------------------------------------
	void  Crt_WebScktConnection();
	void  Crt_WebScktConnection_Read_Hndlr(const boost::system::error_code& crerr, size_t bytes_read);
	void  Crt_WebScktConnection_Write_Hndlr(const boost::system::error_code& crerr, size_t cbytes_wrtn);

	// 以下は websocket としての応答を実行する
	void  On_Async_Read_Hndlr_asWS(const boost::system::error_code& crerr, size_t bytes_read);
	// WS が確立されたあとは、時刻情報の要求が多いため、静的変数で保持しておく
	// SPRS中で、WS が確立されていないときは古い情報となるが、多少の誤差は覚悟でこの値を利用する
	static time_t  ms_time_atRead_asWS;
	void  On_Async_Write_Hndlr_asWS(const boost::system::error_code& crerr, const size_t cbytes_wrtn);

	void  Clean();

private:
	// -----------------------------------------------
	KServer*  m_pSvr = NULL;
	KSmplListElmt<KClient>*  m_pThis_byListElmt = NULL;  // 改良の余地あり

	int  m_sckt_ID = 0;  // エラー発生時に書き出すのみに利用
	boost::asio::ip::tcp::socket  m_socket{ *::g_pioc };

	uint8_t  ma_ui8_prvt_buf[BYTES_PRIVATE_BUF];
	const boost::asio::mutable_buffers_1  mc_asio_prvt_buf_read{ ma_ui8_prvt_buf, BYTES_PRIVATE_BUF };
	boost::asio::mutable_buffers_1  m_asio_prvt_buf_write{ ma_ui8_prvt_buf, BYTES_PRIVATE_BUF };
	size_t  m_bytes_to_wrt;  // async_write() の橋渡しのみで利用される

	// -----------------------------------------------
	// この範囲のメンバ変数は Clean() で初期化される（Clean() は KServer.Recycle_Clnt() からコールされる）
	// Clean() の際は、KClient_Chat.Clean_KClntChat() もコールされる

	// SPRS で監視対象になっている場合 NULL でなくなる（SPRS と相互に接続される）
	// KClient を close するときには、SPRS 側の WebSckt を NULL にすること
	KRecd_SPRS*  m_precd_SPRS = NULL;
	// 今のところ、SuperUser 判定は、DevIP のみ
	// 今後は、ログイン時のパスワードなどの設定で, SuperUser フラグを ON にする
	bool  mb_IsSuperUser = false;

	// ユーザ名が設定されると、NULL でなくなる
	// SPRS で監視中は, UInfo の uname と uview の柄の変更はできない
	KUInfo_Elmt*  m_pKUInfo_Elmt = NULL;

	EN_STATUS  m_stt_cur = EN_STATUS::EN_No_WS;
	// -----------------------------------------------

	// mu_IP の設定は、KServer::On_Accpt_Svr() で実行される
	// g_glog への記録は、ユーザ名が設定されたときに実行される
	bool  mb_Is_v4;
	union {
		uint32_t  m_IPv4;
		uint64_t  ma_IPv6[2];  // up -> down の順で記録
	} mu_IP;

///===DEVELOPMENT===///
	// リリース版では削除されるメンバ変数
	// 設定は KServer でなされる。一度設定されたら不変。
	KClient_Chat_Intf*  m_pKClnt_Chat;
};

