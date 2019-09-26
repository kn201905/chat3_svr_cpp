#pragma once
#include <stdint.h>

namespace  N_LogID
{
	// 以下は、バイナリデータの書き込みの際に利用される ID。テキストでの書き込みの場合は利用されない
	// ---------------------------------------------------------------
	// パディングエリアを超えるサイズのデータが送られてきたときのフラグ
	// データは truncate されて記録される
	enum : uint16_t { EN_FLG_DataSz_PADover = 0x8000 };

	enum : uint8_t
	{
		// 書き込みバッファがオーバーフローした（通常は起こり得ないはず）
		// KgLog のバッファが、すべて dirty になったということ
		EN_ERR_BUF_Overflow = 0,
	
		EN_UNIX_TIME,
		// KgLog において EN_CLOSE_* が書き込まれていない場合は、余程の状況になっていると考えられる
		EN_CLOSE_Normal,  // Signal_ThrdStop() を受けて、KgLog が終了された
		EN_CLOSE_Abort,  // Signal_ThrdStop() が呼び出されずに、KgLog が終了された

		EN_ERR_STR,		// ログでエラーのチェックをする際に、この ID で検索することを想定
		EN_WARN_STR,	// ログで警告のチェックをする際に、この ID で検索することを想定
		EN_STR,			// 何らかの文字列を格納したい場合

		EN_Push_devIP_v4,
		EN_Push_devIP_v6,

		EN_ChkIP_devIP_v4,
		EN_ChkIP_devIP_v6,

		EN_ChkIP_v4,
		EN_ChkIP_v6,

		EN_Chk_IP_many_times_cnct_v4,  // 回数 ２bytes + IPアドレス
		EN_Chk_IP_many_times_cnct_v6,  // 回数 ２bytes + IPアドレス

		EN_ERR_KClient_Exausted,	// KServer::On_Accpt_Svr()
		EN_ERR_on_Async_Accept,		// KServer::On_Accpt_Svr()
		EN_ERR_Get_remote_endpoint,	// KServer::On_Accpt_Svr()

		EN_ERR_on_Async_Read_Crt_WScktCnctn,	// KClient::Crt_WebScktConnection_Read_Hndlr()
		EN_ERR_no_WebSocket_Key,				// KClient::Crt_WebScktConnection_Read_Hndlr()
		EN_ERR_on_Async_Write_Crt_WScktCnctn,	// KClient::Crt_WebScktConnection_Write_Hndlr()

		// 以下のコードは、m_sckt_ID の 8 bytes が付加される
		EN_ERR_on_Async_Read_WebSckt,	// KClient::On_Async_Read_Hndlr_asWS()
		EN_ERR_not_FIN_Recieved,		// KClient::On_Async_Read_Hndlr_asWS()
		EN_RCVD_CLOSE_opcode,			// KClient::On_Async_Read_Hndlr_asWS()
		EN_RCVD_PING_opcode,			// KClient::On_Async_Read_Hndlr_asWS()
		EN_RCVD_Unhandle_opcode,		// KClient::On_Async_Read_Hndlr_asWS()
		EN_ERR_tooBig_Payload,			// KClient::On_Async_Read_Hndlr_asWS()
		EN_ERR_invalid_len_WS_pckt,		// KClient::On_Async_Read_Hndlr_asWS()

		EN_ERR_on_Async_Write_WebSckt,	// KClient::On_Async_Write_Hndlr_asWS()
		EN_ERR_Miss_WriteBytes_onWS,	// KClient::On_Async_Write_Hndlr_asWS()

		// KLargeBuf の使用頻度チェック用
		EN_REQ_LargeBuf,				// 後に uint16_t の ID が続く
		EN_REQ_LargeBuf_Exausted,

		EN_NO_PMSN_InitRI_v4,	// -> IPv4 4 bytes
		EN_NO_PMSN_InitRI_v6,	// -> IPv6 16 bytes

		EN_KUInfo_Exausted,		// KUinfo のリストを使い切った（まず、ありえないけど、、）

		// 固定長のものを先に書き込んでしまう
		EN_Crt_Usr_v4,		// 4 bytes +  KUInfo->m_bytes_KUInfo bytes
		EN_Crt_Usr_v6,		// 8 bytes(up) + 8 bytes(down) +  KUInfo->m_bytes_KUInfo bytes

		EN_END_LOG_ID
	};
	// ---------------------------------------------------------------
}
