#pragma once
#include <cstddef>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "__common/_KString.h"

namespace  N_WebSckt
{
	enum { EN_NUM_Sckt = 2000 };  // 同時接続数
	// Apache では 8KB をデフォルトとしているらしい
	// 最小で 150 あたり、Firefoxでは 450 程度のヘッダが送られてきた
	// EN_MAX_BYTES_http_hdr + 128（128は SHA-1 のパディングのため）以上にすること
	enum { EN_BYTES_buf_http_hdr = 4000 };
	// 以下の範囲にないヘッダが送られてきたら無効とする
	enum { EN_MIN_BYTES_http_hdr = 120, EN_MAX_BYTES_http_hdr = 1000 };

	// 正確には 130 bytes のはず
	enum { EN_BYTES_http_respns = 150 };
}

//////////////////////////////////////////////////////////////

class  KRecd_SPRS;

class  KWebSckt
{
	friend class  KMng_WSckts;

public:
	// SPRS で監視対象になっている場合 NULL でなくなる
	// WebSckt を close するときには、SPRS 側の WebSckt を NULL にすること
	KRecd_SPRS*  m_precd_SPRS = NULL;

private:
	KWebSckt*  m_prev = NULL;  // begin 要素は NULL
	KWebSckt*  m_next = NULL;  // end 要素は NULL
	
	bool  mb_IsDevIP = false;
	boost::asio::ip::tcp::socket*  m_pboost_sckt = NULL;
};

//////////////////////////////////////////////////////////////

class  KMng_WSckts
{
public:
	static void  Init_Once();
	// 戻り値が NULL だったときは、crskt をクローズして応答しない
	static KWebSckt*  Chk_IP(boost::asio::yield_context& ryc, boost::asio::ip::tcp::socket& crskt);
	static void  DisposeWSckt(KWebSckt* pskt);

private:
	static KWebSckt  msa_WebSckt[N_WebSckt::EN_NUM_Sckt];
	static KWebSckt*  ms_pWebSckt_begin;
	static KWebSckt*  ms_pWebSckt_end;
	static KWebSckt*  ms_pWebSckt_next;  // リストがフルになっている場合は NULL となる

	// --------------------------------------
	// WebSocket リクエストヘッダ受信用バッファ
	// １回の通信でヘッダを取得できない場合、通信不良を起こすクライアントと判断して切断する
	static uint8_t  msa_buf_http_hdr[N_WebSckt::EN_BYTES_buf_http_hdr];
	// 上のバッファの wrapper（バッファへのアドレスと、サイズのみを保持する）
	static boost::asio::mutable_buffers_1  ms_bst_buf_http_hdr;

	// WebSocket レスポンス送信関連
	static KString  ms_kstr_http_respns;
	static char* const  msc_ptr_http_respns_accept_key;
	// 以下は boost::asio::buffer で返されるオブジェクト型
	static boost::asio::mutable_buffers_1  ms_bst_buf_respns_str;

	//========================================
};
