#include <iostream>
#include <boost/beast/core/detail/sha1.hpp>

#include "__common/_KString.h"
#include "KSPRS_rld.h"

namespace asio = boost::asio;

extern "C" void sha1_update_intel(char* o_pbase64, uint64_t key24chr_1, uint64_t key24chr_2, uint64_t key24chr_3);

/////////////////////////////////////////////////////////////////////

KWebSckt  KMng_WSckts::msa_WebSckt[N_WebSckt::EN_NUM_Sckt];
KWebSckt*  KMng_WSckts::ms_pWebSckt_begin = msa_WebSckt;
KWebSckt*  KMng_WSckts::ms_pWebSckt_end = msa_WebSckt + N_WebSckt::EN_NUM_Sckt - 1;
KWebSckt*  KMng_WSckts::ms_pWebSckt_next = msa_WebSckt;

// ---------------------------------------------------------
// WebSocket リクエストヘッダ受信用バッファ
uint8_t  KMng_WSckts::msa_buf_http_hdr[N_WebSckt::EN_BYTES_buf_http_hdr];
asio::mutable_buffers_1  KMng_WSckts::ms_bst_buf_http_hdr{ msa_buf_http_hdr, N_WebSckt::EN_BYTES_buf_http_hdr };

// ---------------------------------------------------------
// レスポンス　送信用バッファ
KString  KMng_WSckts::ms_kstr_http_respns{
	"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "
	"#234567890123456789012345678----" };  // ---- は、sha1_update_intel() によって、\r\n\r\n に書き換えられる

char* const  KMng_WSckts::msc_ptr_http_respns_accept_key = KMng_WSckts::ms_kstr_http_respns.FindPtrOf('#');

// ms_bst_buf_respns_str の送信サイズは固定
asio::mutable_buffers_1  KMng_WSckts::ms_bst_buf_respns_str{
				KMng_WSckts::ms_kstr_http_respns.mc_pstr, (size_t)KMng_WSckts::ms_kstr_http_respns.mc_len };


/////////////////////////////////////////////////////////////////////

void  KMng_WSckts::Init_Once()
{
	KWebSckt*  pskt = msa_WebSckt;
	pskt->m_next = pskt + 1;  // begin 要素の処理
	pskt++;
	for (int i = 0; i < N_WebSckt::EN_NUM_Sckt - 2; ++i)  // begin と end の分で -2
	{
		pskt->m_prev = pskt - 1;
		pskt->m_next = pskt + 1;
		pskt++;
	}
	pskt->m_prev = pskt - 1;  // end 要素の処理
}

// -----------------------------------------------------------

static constexpr uint64_t  SCE_Crt_ui64str(const char* const pstr)
{
	uint64_t  retval = 0;
	const char*  psrc = pstr + 8;
	for (int i = 0; i < 8; ++i)
	{
		psrc--;
		retval <<= 8;
		retval += (uint64_t)*psrc;
	}
	return  retval;
}

static constexpr uint64_t  sce_ui64str_WS_idxkey = SCE_Crt_ui64str("ket-Key:");

KWebSckt*  KMng_WSckts::Chk_IP(asio::yield_context& ryc, asio::ip::tcp::socket& crskt)
{
	// WebSocket 用のリソースが尽きている場合の処理
	if (ms_pWebSckt_next == NULL) { return  NULL; }

	// KSPRS_rld によるチェック
	boost::system::error_code  ec;
	const asio::ip::tcp::endpoint  cremote_ep = crskt.remote_endpoint(ec);
	if (ec) { return  NULL; }

	bool  bIsDevIP;
	KRecd_SPRS* const  precd_SPRS = KSPRS_rld::Chk_IP(cremote_ep, &bIsDevIP);

	// DevIP の場合は、必ず WebSocket を生成する（DevIP のときは、precd_SPRS == NULL に注意）
	if (bIsDevIP == false)
	{
		if (precd_SPRS == NULL)
		{
			// バッファ不足のエラー等が発生したと考えられる
			return  NULL;
		}

		if (precd_SPRS->m_pWebSckt)
		{
			// 直前に接続していた WebSocket は破棄する
			KMng_WSckts::DisposeWSckt(precd_SPRS->m_pWebSckt);
	//		precd_SPRS->m_pWebSckt = NULL;  // DisposeWSckt() で実行されるため不要
		}

		if (precd_SPRS->m_times_cnct > N_SPRS::EN_Limit_Cnct)
		{
			// 接続回数が制限を超えているため、WebSocket は作成しない
			return  NULL;
		}
	}

	// -------------------------------------
	// WebSocket の生成を許可
	auto const  clen_hdr = asio::async_read(crskt, ms_bst_buf_http_hdr, boost::asio::transfer_at_least(1), ryc[ec]);
	if (ec) { return  NULL; }

	if (clen_hdr < N_WebSckt::EN_MIN_BYTES_http_hdr || clen_hdr > N_WebSckt::EN_MAX_BYTES_http_hdr)
	{ return  NULL; }

	// ----------------------------------
	// Sec-WebSocket-Key: の探索
	const uint8_t*  psrc = msa_buf_http_hdr;
	{
		// ８文字 "ket-Key:" + 24文字（base64）＝ 32文字
		const uint8_t* const  pend = msa_buf_http_hdr + clen_hdr - 32;
		while (true)
		{
			if (*(const uint64_t*)psrc == sce_ui64str_WS_idxkey) { break; }
			if (psrc == pend) { return  NULL; }  // Sec-WebSocket-Key が見つからなかった
			psrc++;
		}
	}
	psrc += 8;
	{
		int  bytes_rmn = int(clen_hdr) - (psrc - msa_buf_http_hdr);
		while (true)
		{
			if (bytes_rmn < 24) { return  NULL; }
			// bytes_rmn >= 24
			if (*psrc != 0x20) { break; }
			psrc++;
			bytes_rmn--;
		}
	}
	//「==」があるかどうかの確認
	if (*(uint16_t*)(psrc + 22) != 0x3d3d) { return  NULL; }
	// psrc から 24バイトが WebSocket のキーとなるはず

	// ----------------------------------
	// websocket 受け入れのレスポンスヘッダの生成
	const uint64_t  key24chr_1 = *(uint64_t*)psrc;
	const uint64_t  key24chr_2 = *(uint64_t*)(psrc + 8);
	const uint64_t  key24chr_3 = *(uint64_t*)(psrc + 16);
	sha1_update_intel(msc_ptr_http_respns_accept_key, key24chr_1, key24chr_2, key24chr_3);

#if true
std::cout << "http_response ->" << ms_kstr_http_respns.mc_pstr << std::endl;
#endif

	// レスポンスヘッダを送信して、返信を待つ
	asio::async_write(crskt, ms_bst_buf_respns_str, ryc[ec]);
	if (ec) { return  NULL; }



	auto const  ctest = asio::async_read(crskt, ms_bst_buf_http_hdr, boost::asio::transfer_at_least(1), ryc[ec]);
	if (ec) { return  NULL; }

	return  NULL;
}

// -----------------------------------------------------------

void  KMng_WSckts::DisposeWSckt(KWebSckt* pskt)
{
	// -----------------------
	// リスト組み換え（begin, end も変更することに注意）
	if (pskt->m_prev == NULL)
	{
		// pskt が先頭である場合
		ms_pWebSckt_begin = pskt->m_next;
		ms_pWebSckt_begin->m_prev = NULL;

		ms_pWebSckt_end->m_next = pskt;
		pskt->m_prev = ms_pWebSckt_end;
		pskt->m_next = NULL;
		ms_pWebSckt_end = pskt;
	}
	else if (pskt->m_next != NULL)
	{
		// pskt が途中にある場合
		KWebSckt* const  pprev = pskt->m_prev;
		KWebSckt* const  pnext = pskt->m_next;
		pprev->m_next = pnext;
		pnext->m_prev = pprev;

		ms_pWebSckt_end->m_next = pskt;
		pskt->m_prev = ms_pWebSckt_end;
		pskt->m_next = NULL;
		ms_pWebSckt_end = pskt;
	}
	// pskt が最後尾である場合は、何もしなくてよい

	if (ms_pWebSckt_next == NULL) { ms_pWebSckt_next = ms_pWebSckt_end; }

	// -----------------------
	// pskt の破棄処理

	// 監視対象になってる場合の処理
	if (pskt->m_precd_SPRS)
	{
		// SPRS に WebSocket が dispose されたことを通知する
		pskt->m_precd_SPRS->m_pWebSckt = NULL;
		pskt->m_precd_SPRS = NULL;
	}

	pskt->mb_IsDevIP = false;

	// boost の socket が開いていたら閉じる
	if (pskt->m_pboost_sckt->is_open())
	{
		boost::system::error_code  ec;
		pskt->m_pboost_sckt->close(ec);
	}
	pskt->m_pboost_sckt = NULL;
}


