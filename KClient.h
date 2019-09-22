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
	enum class  EN_Ret_WS_Read_Hndlr { EN_Write, EN_Raad, EN_Close };

	// 戻り値： write が必要な場合 true が返される
	// write が必要な場合は、m_asio_prvt_buf_write に設定を書き込んでおくこと
	virtual EN_Ret_WS_Read_Hndlr  WS_Read_Hndlr(uint16_t* ppayload, size_t bytes_payload) =0;
	// prvt_buf_write が Large になっていたら、元に戻す。Small の状態であったならば何もしない
	// コールされるのは以下の２ヶ所
	// 1) On_Async_Write_Hndlr_asWS()
	// 2) m_pClnt_Chat->WS_Read_Hndlr() == EN_CLOSE のとき
	virtual void  Reset_prvt_buf_write() =0;

	// KServer のコンストラクタで設定される
	KClient*  m_pKClnt = NULL;
};

extern KClient_Chat_Intf*  G_GetNext_Clnt_Chat();


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
	void  On_Async_Write_Hndlr_asWS(const boost::system::error_code& crerr, const size_t cbytes_wrtn);

private:
	// -----------------------------------------------
	KServer*  m_pSvr = NULL;
	KSmplListElmt<KClient>*  m_pThis_byListElmt = NULL;  // 改良の余地あり

	int  m_sckt_ID = 0;  // エラー発生時に書き出すのみに利用
	boost::asio::ip::tcp::socket  m_socket{ *::g_pioc };

	uint8_t  ma_ui8_prvt_buf[BYTES_PRIVATE_BUF];
	boost::asio::mutable_buffers_1  m_asio_prvt_buf_read{ ma_ui8_prvt_buf, BYTES_PRIVATE_BUF };
	boost::asio::mutable_buffers_1  m_asio_prvt_buf_write{ ma_ui8_prvt_buf, BYTES_PRIVATE_BUF };
	size_t  m_bytes_to_wrt;  // async_write() の橋渡しのみで利用される

	// -----------------------------------------------
	// SPRS で監視対象になっている場合 NULL でなくなる
	// KClient を close するときには、SPRS 側の WebSckt を NULL にすること
	KRecd_SPRS*  m_precd_SPRS = NULL;
	bool  mb_IsDevIP = false;

	// mu_IP の設定は、KServer::On_Accpt_Svr() で実行される
	// g_glog への記録は、ユーザ名が設定されたときに実行される
	bool  mb_Is_v4;
	union {
		uint32_t  m_IPv4;
		uint64_t  ma_IPv6[2];  // up -> down の順で記録
	} mu_IP;

	// -----------------------------------------------
	EN_STATUS  m_stt_cur = EN_STATUS::EN_No_WS;

///===DEVELOPMENT===///
	// リリース版では削除されるメンバ変数
	KClient_Chat_Intf*  m_pClnt_Chat;
};

