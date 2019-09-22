#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "__common/_KString.h"
#include "__common/LogID.h"
#include "KgLog/src/KgLog.h"

#include "KClient.h"
#include "KServer.h"

///===DEBUG===///
extern  void  G_cout_xbytes(const uint8_t* psrc, const size_t bytes);


namespace asio = boost::asio;
using asio::ip::tcp;

//extern std::shared_ptr<asio::io_context>  g_sp_ioc;
extern KgLog  g_glog;

extern "C" void sha1_update_intel(uint8_t* o_pbase64, uint64_t key24chr_1, uint64_t key24chr_2, uint64_t key24chr_3);


////////////////////////////////////////////////////////////////

static constexpr KCnstString  sce_kcnststr_http_respns{
	"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "
	"#234567890123456789012345678----" };  // ---- は、sha1_update_intel() によって、\r\n\r\n に書き換えられる

static constexpr int  sce_len_http_respns = sce_kcnststr_http_respns.mc_len;
static constexpr int  sce_idx_http_respns_accept_key = sce_kcnststr_http_respns.FindIdxOf('#');

////////////////////////////////////////////////////////////////
// WebSocket の構築を開始するメソッド

void  KClient::Crt_WebScktConnection()
{
	// WebSocket Key を受け取るための async_read()
//	m_asio_prvt_buf.size_ = BYTES_PRIVATE_BUF;
	m_socket.async_read_some(m_asio_prvt_buf_read,
						boost::bind(&KClient::Crt_WebScktConnection_Read_Hndlr, this,
						asio::placeholders::error, asio::placeholders::bytes_transferred));
}

// -------------------------------------------------------------

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

void  KClient::Crt_WebScktConnection_Read_Hndlr(const boost::system::error_code& crerr, const size_t cbytes_read)
{
	if (crerr || cbytes_read < EN_MIN_BYTES_http_hdr || cbytes_read > EN_MAX_BYTES_http_hdr)
	{
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_on_Async_Read_Crt_WScktCnctn);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}

	// ----------------------------------
	// Sec-WebSocket-Key: の探索
	const uint8_t*  psrc = ma_ui8_prvt_buf;
	const uint8_t* const  cptmnt_hdr_rcvd = ma_ui8_prvt_buf + cbytes_read;
	{
		// ８文字 "ket-Key:" + 24文字（base64）＝ 32文字
		const uint8_t* const  cpend = cptmnt_hdr_rcvd - 32;
		while (true)
		{
			if (*(const uint64_t*)psrc == sce_ui64str_WS_idxkey) { break; }
			if (psrc == cpend)
			{
				// Sec-WebSocket-Key が見つからなかった場合の処理
				g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_no_WebSocket_Key);
				// 自分自身をリサイクルに回してしまう（接続破棄）
				m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
				return;
			}
			psrc++;
		}
	}
	psrc += 8;
	// 0x20 をスキップする
	{
		const uint8_t* const  cpend = cptmnt_hdr_rcvd - 24;
		while (true)
		{
			if (*psrc != 0x20) { break; }
			if (psrc == cpend)
			{
				// Sec-WebSocket-Key が見つからなかった場合の処理
				g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_no_WebSocket_Key);
				// 自分自身をリサイクルに回してしまう（接続破棄）
				m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
				return;
			}
			psrc++;
		}
	}
	//「==」があるかどうかの確認
	if (*(uint16_t*)(psrc + 22) != 0x3d3d)
	{
		// Sec-WebSocket-Key が見つからなかった場合の処理
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_no_WebSocket_Key);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}
	// psrc から 24バイトが WebSocket のキーとなるはず

	// ----------------------------------
	// websocket 受け入れのレスポンスヘッダの生成
	const uint64_t  key24chr_1 = *(uint64_t*)psrc;
	const uint64_t  key24chr_2 = *(uint64_t*)(psrc + 8);
	const uint64_t  key24chr_3 = *(uint64_t*)(psrc + 16);

	// レスポンスヘッダ 129 bytes のコピー
	sce_kcnststr_http_respns.WriteTo(ma_ui8_prvt_buf);
	// WebSocket Accept Key の書き込み
	sha1_update_intel(ma_ui8_prvt_buf + sce_idx_http_respns_accept_key, key24chr_1, key24chr_2, key24chr_3);

///===DEBUG===///
char  str_http_respns[130];
memcpy(str_http_respns, ma_ui8_prvt_buf, 129);
str_http_respns[129] = 0;
std::cout << "http_response ->" << str_http_respns << std::endl;

	// レスポンスヘッダを送信して、返信を待つ
	m_asio_prvt_buf_write.size_ = sce_len_http_respns;
	m_socket.async_write_some(m_asio_prvt_buf_write,
						boost::bind(&KClient::Crt_WebScktConnection_Write_Hndlr, this,
						asio::placeholders::error, asio::placeholders::bytes_transferred));
}

// -------------------------------------------------------------

void  KClient::Crt_WebScktConnection_Write_Hndlr(const boost::system::error_code& crerr, size_t cbytes_wrtn)
{
	if (crerr || cbytes_wrtn != sce_len_http_respns)
	{
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_on_Async_Write_Crt_WScktCnctn);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}
	// この時点から WebSocket としての役割が開始されることとなる
	// ----------------------------------

	// async_read
//	m_asio_prvt_buf_read.size_ = BYTES_PRIVATE_BUF;
	m_stt_cur = EN_STATUS::EN_Async_Read;
	m_socket.async_read_some(m_asio_prvt_buf_read,
						boost::bind(&KClient::On_Async_Read_Hndlr_asWS, this,
						asio::placeholders::error, asio::placeholders::bytes_transferred));
}


////////////////////////////////////////////////////////////////

// EN_RCVD_Unhandle_opcode:		sckt_ID 8 bytes + opcode 1 bytes = 9 bytes
// EN_ERR_tooBig_Payload:		sckt_ID 8 bytes + payload size 8 bytes = 16 bytes
// EN_ERR_invalid_len_WS_pckt:	sckt_ID 8 bytes + cbytes_read 8 bytews + payload size 8 bytes = 24 bytes
// EN_ERR_Miss_WriteBytes_onWS:	sckt_ID 8 bytes + m_bytes_to_wrt 8 bytes + cbytes_wrtn 8 bytes = 24 bytes
static uint8_t  sa_buf_for_ErrLog[48];

void  KClient::On_Async_Read_Hndlr_asWS(const boost::system::error_code& crerr, const size_t cbytes_read)
{
///===DEBUG===///
std::cout << "KClient::On_Async_Read_Hndlr_asWS() ▶ ID: " << m_sckt_ID << ", bytes_read: " << cbytes_read << std::endl;

	if (crerr)
	{
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_on_Async_Read_WebSckt, &m_sckt_ID, 8);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}

	// ----------------------------------
	// WebSocket ペイロードの読み取り開始

	// WS パケットは、ヘッダ部分で６バイト以上はあるはず（ヘッダ 2bytes ＋ マスクキー 4bytes）
	if (cbytes_read < 6)
	{
		*(uint64_t*)sa_buf_for_ErrLog = m_sckt_ID;
		*(uint64_t*)(sa_buf_for_ErrLog + 8) = cbytes_read;
		*(uint64_t*)(sa_buf_for_ErrLog + 16) = 0;
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_invalid_len_WS_pckt, sa_buf_for_ErrLog, 24);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}

	// WebSocket のヘッダ読み込み
	const uint16_t  c1st_16bit = *(uint16_t*)ma_ui8_prvt_buf;  // リトルエンディアンになることに注意
	if (!(c1st_16bit & 0x80))  // FINフラグチェック
	{
		// ペイロードが分割されるようなことはないはず
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_not_FIN_Recieved, &m_sckt_ID, 8);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}

	// RSV, opcode, MASKビット は無視する（処理速度を最重要視）
	// 運用してみて、ping, pong が必要であれば実装する予定

	// opcode のチェック
	if ((c1st_16bit & 0xf) != 2)  // バイナリフレーム以外は破棄する
	{
		switch (c1st_16bit & 0xf)
		{
			case 0x8:  // CLOSE
				g_glog.WriteID_with_UnixTime(N_LogID::EN_RCVD_CLOSE_opcode, &m_sckt_ID, 8);
				break;

			case 0x9:  // PING
				g_glog.WriteID_with_UnixTime(N_LogID::EN_RCVD_PING_opcode, &m_sckt_ID, 8);
				break;

			default:
				*(uint64_t*)sa_buf_for_ErrLog = m_sckt_ID;
				sa_buf_for_ErrLog[8] = c1st_16bit & 0xf;
				g_glog.WriteID_with_UnixTime(N_LogID::EN_RCVD_Unhandle_opcode, sa_buf_for_ErrLog, 9);
		}

		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}

	// ペイロードサイズのチェック
	const uint16_t  clen_payload = c1st_16bit & 0x7f00;
	if (clen_payload == 0x7f00)
	{
		*(uint64_t*)sa_buf_for_ErrLog = m_sckt_ID;
		*(uint64_t*)(sa_buf_for_ErrLog + 8) = *(uint64_t*)(ma_ui8_prvt_buf + 2);
		// On_Async_Read_Hndlr_onRglr() が受け取るペイロードは、高々 1kbytes 程度まで
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_tooBig_Payload, sa_buf_for_ErrLog, 16);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}

	uint16_t  bytes_payload;
	uint8_t*  pkey_payload;
	{
		if (clen_payload == 0x7e00)
		{
			// ペイロードサイズが 16bit の場合
			bytes_payload = *(uint16_t*)(ma_ui8_prvt_buf + 2);
			pkey_payload = ma_ui8_prvt_buf + 4;

			if (cbytes_read != bytes_payload + 8)
			{
				*(uint64_t*)sa_buf_for_ErrLog = m_sckt_ID;
				*(uint64_t*)(sa_buf_for_ErrLog + 8) = cbytes_read;
				*(uint64_t*)(sa_buf_for_ErrLog + 16) = bytes_payload;
				g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_invalid_len_WS_pckt, sa_buf_for_ErrLog, 24);
				// 自分自身をリサイクルに回してしまう（接続破棄）
				m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
				return;
			}
		}
		else
		{
			// ペイロードサイズが 125 bytes 以下の場合
			bytes_payload = (clen_payload & 0x7f00) >> 8;
			pkey_payload = ma_ui8_prvt_buf + 2;

			if (cbytes_read != bytes_payload + 6)
			{
				*(uint64_t*)sa_buf_for_ErrLog = m_sckt_ID;
				*(uint64_t*)(sa_buf_for_ErrLog + 8) = cbytes_read;
				*(uint64_t*)(sa_buf_for_ErrLog + 16) = bytes_payload;
				g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_invalid_len_WS_pckt, sa_buf_for_ErrLog, 24);
				// 自分自身をリサイクルに回してしまう（接続破棄）
				m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
				return;
			}
		}
	}

	// クライアントからのデータは必ずマスクされているため、マスクを解除する
	if (bytes_payload > BYTES_MAX_Payload)
	{
		*(uint64_t*)sa_buf_for_ErrLog = m_sckt_ID;
		*(uint64_t*)(sa_buf_for_ErrLog + 8) = bytes_payload;
		// On_Async_Read_Hndlr_onRglr() が受け取るペイロードは、高々 1kbytes 程度まで
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_tooBig_Payload, sa_buf_for_ErrLog, 16);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}

	// マスク解除実行
	uint8_t* const  c_pdata_payload = pkey_payload + 4;
	{
		uint64_t  mask_key = *(uint32_t*)pkey_payload;
		mask_key |= (mask_key << 32);

		uint64_t*  psrc = (uint64_t*)c_pdata_payload;
		const uint64_t* const  cpend = (uint64_t*)(c_pdata_payload + bytes_payload - 1);

		while (true)
		{
			*psrc ^= mask_key;
			if (++psrc > cpend) { break; }
		}
	}

///===DEBUG===///
std::cout << "KClient::On_Async_Read_Hndlr_asWS() ▶ succeeded to read payload." << std::endl;

	switch (m_pClnt_Chat->WS_Read_Hndlr((uint16_t*)c_pdata_payload, bytes_payload))
	{
	case  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Write:
		m_stt_cur = EN_STATUS::EN_Async_Write;
		m_bytes_to_wrt = m_asio_prvt_buf_write.size_;
		m_socket.async_write_some(m_asio_prvt_buf_write,
							boost::bind(&KClient::On_Async_Write_Hndlr_asWS, this,
							asio::placeholders::error, asio::placeholders::bytes_transferred));
		break;

	case  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Raad:
		m_stt_cur = EN_STATUS::EN_Async_Read;
		m_socket.async_read_some(m_asio_prvt_buf_read,
							boost::bind(&KClient::On_Async_Read_Hndlr_asWS, this,
							asio::placeholders::error, asio::placeholders::bytes_transferred));
		break;

	case  KClient_Chat_Intf::EN_Ret_WS_Read_Hndlr::EN_Close:
		m_pClnt_Chat->Reset_prvt_buf_write();
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
	}
}

// -------------------------------------------------------------

void  KClient::On_Async_Write_Hndlr_asWS(const boost::system::error_code& crerr, const size_t cbytes_wrtn)
{
	// write バッファの役割は終えたため、バッファを元に戻しておく
	m_pClnt_Chat->Reset_prvt_buf_write();

	if (crerr)
	{
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_on_Async_Read_WebSckt, &m_sckt_ID, 8);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;
	}

///===DEBUG===///
std::cout << "KClient::On_Async_Write_Hndlr() ▶ ID: " << m_sckt_ID << ", bytes_written: " << cbytes_wrtn << std::endl;

	if (cbytes_wrtn != m_bytes_to_wrt)
	{
		*(uint64_t*)sa_buf_for_ErrLog = m_sckt_ID;
		*(uint64_t*)(sa_buf_for_ErrLog + 8) = m_bytes_to_wrt;
		*(uint64_t*)(sa_buf_for_ErrLog + 8) = cbytes_wrtn;
		g_glog.WriteID_with_UnixTime(N_LogID::EN_ERR_Miss_WriteBytes_onWS, sa_buf_for_ErrLog, 24);
		// 自分自身をリサイクルに回してしまう（接続破棄）
		m_pSvr->Recycle_Clnt(m_pThis_byListElmt);
		return;		
	}

	// async_read
	m_stt_cur = EN_STATUS::EN_Async_Read;
	m_socket.async_read_some(m_asio_prvt_buf_read,
						boost::bind(&KClient::On_Async_Read_Hndlr_asWS, this,
						asio::placeholders::error, asio::placeholders::bytes_transferred));
}

