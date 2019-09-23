#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../__common/LogID.h"
#include "../__common/_KString.h"
#include "../__common/_KFile_W.h"
//#include "../__common/KScktStr.h"

using  std::cout;
using  std::endl;

struct  KFile_AutoClose
{
	KFile_AutoClose(FILE* pfile) : mc_pfile{ pfile } {}
	~KFile_AutoClose() { fclose(mc_pfile); }
	FILE* const  mc_pfile;
};

struct  KStr_AutoDel
{
	KStr_AutoDel(char* parray_char) : mc_pary_char{ parray_char } {}
	~KStr_AutoDel() { delete[]  mc_pary_char; }
	char* const  mc_pary_char;
};

// ------------------------------------------------------------------
// txt ファイルを書き出すオブジェクト
enum { EN_bytes_Buf = 10 * 1000, EN_bytes_Pad = 1000 };
KFile_W  g_DstFile_W{ EN_bytes_Buf, EN_bytes_Pad };  // デストラクタがファイルを close してくれる
// STR系を fread で読み出すバッファ（最大サイズは EN_bytes_Pad と考える）
char  sa_read_buf[EN_bytes_Pad];
// ------------------------------------------------------------------


// sa_read_buf に、len バイト分だけ有効なデータが入れられて、以下の関数が呼び出される。
// sa_read_buf を利用するため、len 以上に読み込んでも、EN_bytes_Pad バイトまでならフォルトが発生しない。
// エラーがなければ NULL を返す。エラーがある場合は、エラーメッセージを返す。
// エラーメッセージがある場合には、それを printf して、ファイルの読み込みを中断する。
const char*  G_EN_ERR_BUF_Overflow(uint16_t len);
const char*  G_EN_UnixTime(uint16_t len);
const char*  G_EN__WRITE_UTF8__STR(uint16_t len);

const char*  G__PRINT__IP_v4(uint16_t len);
const char*  G__PRINT__IP_v6(uint16_t len);
const char*  G_EN_Chk_IP_many_times_cnct_v4(uint16_t len);
const char*  G_EN_Chk_IP_many_times_cnct_v6(uint16_t len);

const char*  G_EN__WRITE_UTF16__STR(uint16_t len);
const char*  G_EN_RCVD_Unhandle_opcode(uint16_t len);
const char*  G_EN_ERR_tooBig_Payload(uint16_t len);
const char*  G_EN_ERR_invalid_len_WS_pckt(uint16_t len);
const char*  G_EN_ERR_Miss_WriteBytes_onWS(uint16_t len);

const char*  G__PRINT__ui64(uint16_t len);
const char*  G__PRINT__ui16(uint16_t len);


using  fn_ptr = const char* (*)(uint16_t);
struct
{
	uint8_t  m_log_id = 0;
	// NULL である場合、ジャンプ処理は実行しない（mc_pstr_id_hdr のみの表示となる）
	fn_ptr  m_fn_ptr = NULL;
	const char*  mc_pstr_id_hdr = NULL;
	// 長さが固定長ではない場合、-1 を設定する
	int  m_fixed_len;
}
const static  sca_jump_list[N_LogID::EN_END_LOG_ID]
= {
	{ N_LogID::EN_ERR_BUF_Overflow, G_EN_ERR_BUF_Overflow, " ▶▶▶ ERR_BUF_Overflow\n", 0 },

	{ N_LogID::EN_UNIX_TIME, G_EN_UnixTime, "", 8 },
	{ N_LogID::EN_CLOSE_Normal, NULL, " CLOSE_Normal\n", 0 },
	{ N_LogID::EN_CLOSE_Abort, NULL, " CLOSE_Abort\n", 0 },

	{ N_LogID::EN_ERR_STR, G_EN__WRITE_UTF8__STR, " ▶ ERR_STR: ", -1 },
	{ N_LogID::EN_WARN_STR, G_EN__WRITE_UTF8__STR, " ▶ WARN_STR: ", -1 },
	{ N_LogID::EN_STR, G_EN__WRITE_UTF8__STR, " STR: ", -1 },

	{ N_LogID::EN_Push_devIP_v4, G__PRINT__IP_v4, " Push_devIP_v4: ", 4 },
	{ N_LogID::EN_Push_devIP_v6, G__PRINT__IP_v6, " Push_devIP_v6: ", 16 },

	{ N_LogID::EN_ChkIP_devIP_v4, G__PRINT__IP_v4, " --- ChkIP_devIP_v4: ", 4 },
	{ N_LogID::EN_ChkIP_devIP_v6, G__PRINT__IP_v6, " --- ChkIP_devIP_v6: ", 16 },

	{ N_LogID::EN_ChkIP_v4, G__PRINT__IP_v4, " ChkIP_v4: ", 4 },
	{ N_LogID::EN_ChkIP_v6, G__PRINT__IP_v6, " ChkIP_v6: ", 16 },

	{ N_LogID::EN_Chk_IP_many_times_cnct_v4, G_EN_Chk_IP_many_times_cnct_v4, " ▶ ChkIP_many_times_v4 / times: ", 6 },
	{ N_LogID::EN_Chk_IP_many_times_cnct_v6, G_EN_Chk_IP_many_times_cnct_v6, " ▶ ChkIP_many_times_v6 / times: ", 18 },

	{ N_LogID::EN_ERR_KClient_Exausted, NULL, " ▶▶▶ ERR: KClient Exausted\n", 0 },
	{ N_LogID::EN_ERR_on_Async_Accept, NULL, " ▶ ERR: async_accept() in KServer::On_Accpt_Svr\n", 0 },
	{ N_LogID::EN_ERR_Get_remote_endpoint, NULL, " ▶ ERR: get remote_endpoint() in KServer::On_Accpt_Svr\n", 0 },

	{ N_LogID::EN_ERR_on_Async_Read_Crt_WScktCnctn, NULL,
			" ▶ ERR: async_read() in KClient::Crt_WebScktConnection_Read_Hndlr()\n", 0 },
	{ N_LogID::EN_ERR_no_WebSocket_Key, NULL,
			" ▶ ERR: unfind WebSocket-Key in KClient::Crt_WebScktConnection_Read_Hndlr()\n", 0 },
	{ N_LogID::EN_ERR_on_Async_Write_Crt_WScktCnctn, NULL,
			" ▶ ERR: async_write() in KClient::Crt_WebScktConnection_Write_Hndlr()\n", 0 },

	{ N_LogID::EN_ERR_on_Async_Read_WebSckt, G__PRINT__ui64,
		" ▶ ERR: async_read() in KClient::On_Async_Read_Hndlr_asWS(), sckt_ID: ", 8 },
	{ N_LogID::EN_ERR_not_FIN_Recieved, G__PRINT__ui64,
		" ▶ ERR: 断片化された WSパケットを受信しました(FINフラグ:0) in KClient::On_Async_Read_Hndlr_asWS(), sckt_ID: ", 8 },
	{ N_LogID::EN_RCVD_CLOSE_opcode, G__PRINT__ui64,
		" ▶ WARN: CLOSEパケットを受信しました(opcode: 8) in KClient::On_Async_Read_Hndlr_asWS(), sckt_ID: ", 8 },
	{ N_LogID::EN_RCVD_PING_opcode, G__PRINT__ui64,
		" ▶ WARN: PINGパケットを受信しました(opcode: 9) in KClient::On_Async_Read_Hndlr_asWS(), sckt_ID: ", 8 },
	{ N_LogID::EN_RCVD_Unhandle_opcode, G_EN_RCVD_Unhandle_opcode,
		" ▶ WARN: 不明な opcode パケットを受信しました in KClient::On_Async_Read_Hndlr_asWS(), sckt_ID: ", 9 },
	{ N_LogID::EN_ERR_tooBig_Payload, G_EN_ERR_tooBig_Payload,
		" ▶ ERR: ペイロードサイズが大きすぎます in KClient::On_Async_Read_Hndlr_asWS(), sckt_ID: ", 16 },
	{ N_LogID::EN_ERR_invalid_len_WS_pckt, G_EN_ERR_invalid_len_WS_pckt,
		" ▶ ERR: ペイロードサイズが不正値になっています in KClient::On_Async_Read_Hndlr_asWS(), sckt_ID: ", 24 },

	{ N_LogID::EN_ERR_on_Async_Write_WebSckt, G__PRINT__ui64,
		" ▶ ERR: async_write() in KClient::On_Async_Write_Hndlr_asWS(), sckt_ID: ", 8 },
	{ N_LogID::EN_ERR_Miss_WriteBytes_onWS, G_EN_ERR_Miss_WriteBytes_onWS,
		" ▶ ERR: cbytes_wrtn != m_bytes_to_wrt in KClient::On_Async_Write_Hndlr_asWS(), sckt_ID: ", 24 },

	{ N_LogID::EN_REQ_LargeBuf, G__PRINT__ui16, " ◀ Info: REQ_LargeBuf, buf_ID: ", 2 },
	{ N_LogID::EN_REQ_LargeBuf_Exausted, NULL, " ▶▶▶ ERR: REQ_LargeBuf_Exausted", 0 },

	{ N_LogID::EN_NO_PMSN_InitRI_v4, G__PRINT__IP_v4, " EN_NO_PMSN_InitRI_v4: ", 4 },
	{ N_LogID::EN_NO_PMSN_InitRI_v6, G__PRINT__IP_v6, " EN_NO_PMSN_InitRI_v6: ", 16 },
};

class
{
	const char*  m_pstr_id_hdr = NULL;
	size_t  m_len = 0;
public:
	void  Init(const char* const pstr_id_hdr) { m_pstr_id_hdr = pstr_id_hdr; m_len = CE_StrLen(pstr_id_hdr); }
	const char* const&  mr_pstr_id_msg = m_pstr_id_hdr;
	const size_t&  mr_len = m_len;
}
static  sa_logID_hdr[N_LogID::EN_END_LOG_ID];

// ------------------------------------------------------------------

// 戻り値は、変換後のバイト数（io_pstr から示されるバッファに、変換結果が格納される）
// len はバイト数を表すことに注意。かつ、len は偶数であると想定している
size_t  G_IP_str_utf16_to_utf8(char* io_pstr, uint16_t len);

/////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("引数が１個ではありません。\n");
		return  0;
	}

	// sca_jump_list の初期化チェック
	if (sca_jump_list[N_LogID::EN_END_LOG_ID - 1].m_log_id == 0)
	{
		printf("sca_jump_list[] に初期化されていない要素があります。\n");
		return  0;
	}

	// sa_logID_msg の初期化
	for (int i = 0; i < N_LogID::EN_END_LOG_ID; i++)
	{
		const char*  pstr = sca_jump_list[i].mc_pstr_id_hdr;
		if (pstr == NULL)
		{
			cout << "mc_pstr_id_hdr に NULL を検出しました。" << endl;
			return  0;
		}
		sa_logID_hdr[i].Init( pstr );
	}

	// src 側のファイルの準備
	FILE* const  pc_file_src = fopen(argv[1], "r");
	if (pc_file_src == NULL)
	{
		printf("src側の fopen() に失敗しました。\n");
		return  0;
	}
	KFile_AutoClose  file_autoclose_src{ pc_file_src };

	// dst 側のファイルの準備
	const size_t  clen_fname_src = strlen(argv[1]);
	char* const  pc_fname_dst = new char[clen_fname_src + 5];
	KStr_AutoDel  str_autodel_fname_dst{ pc_fname_dst };
	strcpy(pc_fname_dst, argv[1]);
	strcpy(pc_fname_dst + clen_fname_src, ".txt");

	if(g_DstFile_W.OpenFile_mode_w(pc_fname_dst) == KFile_W::EN_Err::ERR)
	{
		printf("dest側の fopen() に失敗しました。\n");
		return  0;
	}

	// ------------------------------------
	// N_LogID.h の ID（uint8_t）に従って、バイナリ情報をテキストに変換して書き出していく

	constexpr uint8_t  ce_BREAK_read_logID = 255;
	static_assert(N_LogID::EN_END_LOG_ID < ce_BREAK_read_logID);

	int  cnt_decode = 0;
	while (true)
	{
		cnt_decode++;

		// ---------------------------------------
		// logID の読み出し（１バイト）
		const uint8_t  clogID = ([&]() -> uint8_t {
			uint8_t  log_id;
			const size_t  cbytes_read = fread(&log_id, 1, 1, pc_file_src);
			if (cbytes_read == 0)
			{
				if (feof(pc_file_src))
				{
					printf("ファイルのテキストへの変換が正常終了しました。\n");
					return  ce_BREAK_read_logID;
				}

				printf("LogID の fread() に失敗しました。\n");
				return  ce_BREAK_read_logID;
			}
			return  log_id;
		})();
		if (clogID == ce_BREAK_read_logID) { break; }

		if (clogID >= N_LogID::EN_END_LOG_ID)
		{
			cout << "不明な LogID を検出しました。LogID: " << (int)clogID << endl;
			cout << "cnt_decode: " << cnt_decode << endl;
			break;
		}

		// ---------------------------------------
		// ↓ デバッグ用
		if (sca_jump_list[clogID].m_log_id != clogID)
		{
			printf("sa_jump_list[] が正しく設定されていません。LogID: %d\n", clogID);
			break;
		}

		// ---------------------------------------
		// len を取り出す（２バイト）
		const uint16_t  clen = [&]() ->uint16_t {
			uint16_t  len;
			const size_t  cbytes_read = fread(&len, 1, 2, pc_file_src);
			if (cbytes_read != 2)
			{
				cout << "len の fread() に失敗しました。cbytes_read: " << cbytes_read << endl;
				return  0xffff;
			}
			return  len;
		}();
		if (clen == 0xffff) { break; }

		// sa_read_buf の準備（len の分だけ読み出す）
		if (clen > 0)
		{
			if (clen > EN_bytes_Pad)
			{
				printf("大きすぎる len を検出しました。LogID: %d, len: %d / EN_bytes_Pad: %d\n", clogID, clen, EN_bytes_Pad);
				break;
			}

			const size_t  cbytes_read = fread(sa_read_buf, 1, clen, pc_file_src);
			if (cbytes_read < clen)
			{
				printf("sa_read_buf への fread() に失敗しました。LogID: %d, len: %d, cbytes_read: %d\n"
						, clogID, clen, (int)cbytes_read);
				break;
			}
		}

		// g_DstFile_W に、LogID_msg のヘッダ書き込みを行う
		if (sa_logID_hdr[clogID].mr_len > 0)
		{
			KFile_W::EN_Err  cret = g_DstFile_W.Write(sa_logID_hdr[clogID].mr_pstr_id_msg, sa_logID_hdr[clogID].mr_len);
			if (cret != KFile_W::EN_Err::OK)
			{
				cout << "g_DstFile_W に、LogID ヘッダ書き込みに失敗しました。LogID: " << clogID << endl;
				break;
			}
		}

		// m_fixed_len のチェック
		const int  cfixed_len = sca_jump_list[clogID].m_fixed_len;
		if (cfixed_len >= 0)
		{
			if (cfixed_len != clen)
			{
				cout << "len の長さが規定と異なっています。" << endl << sca_jump_list[clogID].mc_pstr_id_hdr << endl
					<< "規定の len: " << sca_jump_list[clogID].m_fixed_len << ", 検出された len: " << clen << endl;
				break;
			}
		}

		// ジャンプ
		fn_ptr  fp_dst_jump = sca_jump_list[clogID].m_fn_ptr;
		if (fp_dst_jump)
		{
			const char* const  cret_str = fp_dst_jump(clen);
			if (cret_str)
			{
				std::cout << cret_str;
				break;
			}
		}
	}

	return 0;
}

// ------------------------------------------------------------------
const char*  G_EN_ERR_BUF_Overflow(const uint16_t len)
{
	return  "ERR: EN_ERR_BUF_Overflow を検知しました\n";
}

// ------------------------------------------------------------------
const char*  G_EN_UnixTime(const uint16_t len)
{
	// UnixTime を書き出した後、改行させない
	g_DstFile_W.Write_MMDHM_onUnixTime(*(time_t*)sa_read_buf);
	return  NULL;
}

// ------------------------------------------------------------------

const char*  G_EN__WRITE_UTF8__STR(const uint16_t len)
{
	g_DstFile_W.Write_LF(sa_read_buf, len);
	return  NULL;
}

// ------------------------------------------------------------------

static void  S_PRINT_IP_v4(char* pread_buf)
{
	std::string  str;
	uint8_t*  psrc = (uint8_t*)pread_buf;
	str = std::to_string(*psrc++) + ".";
	str += std::to_string(*psrc++) + ".";
	str += std::to_string(*psrc++) + ".";
	str += std::to_string(*psrc);

	g_DstFile_W.Write(str.data(), str.size());
}

const char*  G__PRINT__IP_v4(const uint16_t len)
{
	S_PRINT_IP_v4(sa_read_buf);
	g_DstFile_W.Write_LF();
	return  NULL;
}

// ------------------------------------------------------------------

static void  S_PRINT_IP_v6(char* pread_buf)
{
	std::stringstream ss;
	const uint8_t*  psrc = (uint8_t*)pread_buf;

	for (int i = 0; i < 8; i++)
	{
		uint16_t  val = (*psrc << 8) + *(psrc + 1);
		psrc += 2;

		if (i < 7)
		{ ss << std::hex << val << ":"; }
		else
		{ ss << std::hex << val; }
	}
	std::string  str = ss.str();

	g_DstFile_W.Write(str.data(), str.size());
}

const char*  G__PRINT__IP_v6(const uint16_t len)
{
	S_PRINT_IP_v6(sa_read_buf);
	g_DstFile_W.Write_LF();
	return  NULL;
}

// ------------------------------------------------------------------

const char*  G_EN_Chk_IP_many_times_cnct_v4(uint16_t len)
{
	// 接続回数の表示
	std::string  str = std::to_string(*(uint16_t*)sa_read_buf);
	g_DstFile_W.Write(str.data(), str.size());

	// IP_v4 の表示
	g_DstFile_W.Write( KCnstString(" , IP_v4: ") );
	S_PRINT_IP_v4(sa_read_buf + 2);
	g_DstFile_W.Write_LF();
	return  NULL;
}

// ------------------------------------------------------------------

const char*  G_EN_Chk_IP_many_times_cnct_v6(uint16_t len)
{
	// 接続回数の表示
	std::string  str = std::to_string(*(uint16_t*)sa_read_buf);
	g_DstFile_W.Write(str.data(), str.size());

	// IP_v6 の表示
	g_DstFile_W.Write( KCnstString(" , IP_v6: ") );
	S_PRINT_IP_v6(sa_read_buf + 2);
	g_DstFile_W.Write_LF();
	return  NULL;
}

// ------------------------------------------------------------------

const char*  G_EN__WRITE_UTF16__STR(const uint16_t len)
{
	if (len & 1)
	{
		printf ("ERR: len の値が奇数になっています。len: %d", len);
		return  "ERR: G_EN__WRITE_UTF16__STR()";
	}

	// utf-8 文字列に変換して、ログに書き出す
	size_t  utf8_len = G_IP_str_utf16_to_utf8(sa_read_buf, len);
	g_DstFile_W.Write(sa_read_buf, utf8_len);

	return  NULL;
}

// ------------------------------------------------------------------
// 戻り値は、変換後のバイト数（pstr から示されるバッファに、変換結果が格納される）
// len はバイト数を表すことに注意。かつ、len は偶数であると想定している

size_t  G_IP_str_utf16_to_utf8(char* io_pstr, uint16_t len)
{
	const char* const  pc_bgn = io_pstr;
	char* pdst = io_pstr;

	while (len > 0)
	{
		const char  chr_1 = *io_pstr++;
		const char  chr_2 = *io_pstr++;
		len -= 2;

		if (chr_1 < 0x20 || chr_1 > 0x7e || chr_2 != 0)
		{
			// 想定外の文字が検出された場合、特定のキャラクタにして出力しておく
			*pdst++ = 0xd0;
			*pdst++ = 0xa8;
		}
		else
		{
			*pdst++ = chr_1;
		}
	}

	return  pdst - pc_bgn;
}

// ------------------------------------------------------------------

const char*  G__PRINT__ui64(uint16_t len)
{
	// 64bit 値の表示
	std::string  str = std::to_string(*(uint64_t*)sa_read_buf);
	g_DstFile_W.Write(str.data(), str.size());
	g_DstFile_W.Write_LF();
	return  NULL;
}

// ------------------------------------------------------------------

const char*  G_EN_RCVD_Unhandle_opcode(uint16_t len)
{
	// sckt_ID の表示
	std::string  str = std::to_string(*(uint64_t*)sa_read_buf)
					+ ", opcode: " + std::to_string(*(uint8_t*)(sa_read_buf + 8));
	g_DstFile_W.Write(str.data(), str.size());
	g_DstFile_W.Write_LF();
	return  NULL;
}

// ------------------------------------------------------------------

const char*  G_EN_ERR_tooBig_Payload(uint16_t len)
{
	// sckt_ID の表示
	std::string  str = std::to_string(*(uint64_t*)sa_read_buf)
					+ ", payload size: " + std::to_string(*(uint64_t*)(sa_read_buf + 8));
	g_DstFile_W.Write(str.data(), str.size());
	g_DstFile_W.Write_LF();
	return  NULL;
}

// ------------------------------------------------------------------

const char*  G_EN_ERR_invalid_len_WS_pckt(uint16_t len)
{
	// sckt_ID の表示
	std::string  str = std::to_string(*(uint64_t*)sa_read_buf)
					+ ", cbytes_read: " + std::to_string(*(uint64_t*)(sa_read_buf + 8))
					+ ", payload size: " + std::to_string(*(uint64_t*)(sa_read_buf + 16));
	g_DstFile_W.Write(str.data(), str.size());
	g_DstFile_W.Write_LF();
	return  NULL;
}

// ------------------------------------------------------------------

const char*  G_EN_ERR_Miss_WriteBytes_onWS(uint16_t len)
{
	// sckt_ID の表示
	std::string  str = std::to_string(*(uint64_t*)sa_read_buf)
					+ ", m_bytes_to_wrt: " + std::to_string(*(uint64_t*)(sa_read_buf + 8))
					+ ", cbytes_wrtn: " + std::to_string(*(uint64_t*)(sa_read_buf + 16));
	g_DstFile_W.Write(str.data(), str.size());
	g_DstFile_W.Write_LF();
	return  NULL;
}

// ------------------------------------------------------------------

const char*  G__PRINT__ui16(uint16_t len)
{
	// 16 bit 値の表示
	std::string  str = std::to_string(*(uint16_t*)sa_read_buf);
	g_DstFile_W.Write(str.data(), str.size());
	g_DstFile_W.Write_LF();
	return  NULL;
}
