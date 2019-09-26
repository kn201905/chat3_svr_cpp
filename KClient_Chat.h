#pragma once

#include "KServer.h"
#include "KClient.h"
#include "KSmplList.hpp"

namespace  N_BUFFER
{
	enum {
		EN_BYTES_Large = 51200,		// 50 KB
		EN_NUM_Large = 40,
	};
}

namespace  N_JSC
{
	enum {
		// クライアントにリクエスト再送要求を出したとき、指示された秒数を下回る間隔で再送を要求してくるクライアントは
		// 切断する。しかし、以下の秒数だけは、指定された秒数より下回っても許可することにする。
		EN_SEC_Margin_Force_toCLOSE = 3,

		EN_SEC_Wait_Init_RI = 5,
		EN_SEC_Wait_Crt_Usr = 60,	// 普通は起こり得ないと思うけど、、、
	};

	enum : uint16_t {
		EN_UP_Init_RI = 1,
		EN_DN_Init_RI,
		EN_UP_Crt_Usr,
		EN_DN_Crt_Usr,
	};

	enum : uint16_t {
		EN_CMD_MASK = 0xff,
		EN_BUSY_WAIT_SEC = 0x100,  // wait sec 付きの返信であったことを表す

		EN_DN_Init_RI__WARN_multi_cnct = 0xffff,
	};
}

////////////////////////////////////////////////////////////////

class  KLargeBuf
{
	enum  {
		EN_BYTES_BUF = 50 * 1024,  // 50 kB
		ENN_NUM_BUF = 100,  // バッファの個数
	};

	friend class  KLargeBuf_Initr;
public:
	class  KInfo {
		friend class  KLargeBuf;
		friend class  KLargeBuf_Initr;
		// 利用されているときは１以上の数値となる。数値は、何個のバッファを連続で使用しているかを表す
		uint8_t  m_stt = 0;
		uint16_t  m_ID = 0;  // ID は１以上の数とする（使用された個数と考えるため）
		uint8_t*  m_pbuf = NULL;
		uint8_t*  m_pbuf_tmnt = NULL;

	public:
		uint8_t*  GetBuf() const { return  m_pbuf; }
		uint8_t*  GetBuf_tmnt() const { return  m_pbuf_tmnt; }
	};

	// バッファが確保できなかった場合、NULL が返される
	static KInfo*  ReqBuf_Single();
	static void  Return_Buf(KInfo* pinfo_buf);

private:
	static KInfo  msa_Info[ENN_NUM_BUF];
	static const KInfo* const  msc_pInfo_tmnt;

	static uint8_t  msa_buf[EN_BYTES_BUF * ENN_NUM_BUF];
};

class  KLargeBuf_Initr
{
public:
	KLargeBuf_Initr();
};

////////////////////////////////////////////////////////////////

struct  KUInfo  // paddingなしで 30 bytes（KSmplList2 で 46 bytes）
{
	enum  {
		EN_NUM_KUInfo = 3000,  // 46 bytes * 3000 = 138 kB

		EN_MAX_LEN_uname = 10,
	};

	// データ送信時に、memcpy で利用される。また、ユーザ名の文字数も算出できる
	// 利用していない場合、0 に設定すること
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


// pack なしで 568 bytes。おそらく、4 bytes 単位で pack されると思われる。
// uint16_t と uint8_t は、まとめて 4 byte の領域になるもよう。
struct  KRI
{
	enum  {
		EN_MAX_CAPA = 15,
		EN_MAX_LEN_rm_prof = 50,

		EN_NUM_KRI = 500,  // 同時作成できる部屋数を指定
	};

	uint16_t  m_bytes_send = 0;  // send するバイト数

	uint32_t  m_rmID = 0;
	uint16_t  m_topicID = 0;  // 特殊部屋は、topicID で表すことにする

	uint8_t  m_capa = 0;
	uint8_t  m_num_usrs_cur = 0;  // 現在の参加人数

	uint16_t  m_bytes_rm_prof = 0;  // memcpy で利用される（念の為、256bytes 以上も想定して uint16_t）
	uint16_t  ma_rm_prof[EN_MAX_LEN_rm_prof] = {};

	// エラーを検出するために、利用していないところは NULL に設定することにする
	KUInfo_Elmt*  ma_pUInfo[EN_MAX_CAPA];


	// --------------------------------------------
	// 書き込みバイト数の計算ミスを防ぐために uint8_t* を受け取るようにしている
	void  Srlz_to(uint8_t* psrc);

	// --------------------------------------------
	static KSmplList2<KRI, EN_NUM_KRI>  ms_List;
};


////////////////////////////////////////////////////////////////
// KClient_Chat は、コンパイル時間を短縮するためだけにあるクラス
// リリース版では、KClient と KClient_Chat と統合すること

class  KClient_Chat : public KClient_Chat_Intf
{
public:
	virtual EN_Ret_WS_Read_Hndlr  WS_Read_Hndlr(uint16_t* pdata_payload, size_t bytes_payload) override;
	// リリース版では、Reset_prvt_buf_write() は再々コールされるため、inline にすること
	virtual void  Reset_prvt_buf_write() override;

	// m_pUInfo をクリアしたり、バッファを解放したりする
	virtual void  Recycle() override;

//	KClient*  m_pKClnt = NULL;
private:
	EN_Ret_WS_Read_Hndlr  UP_InitRI();
	EN_Ret_WS_Read_Hndlr  UP_Crt_Usr(uint16_t* pdata_payload, size_t bytes_payload);

	// --------------------------------------------
	KUInfo_Elmt*  m_pUInfo_Elmt = NULL;

	KLargeBuf::KInfo*  m_pInfo_LargeBuf = NULL;
	// リソース不足のときに遅延評価する必要が多いため、WS_Read_Hndlr() がコールされたときの時刻を記録しておく
	// WS_Write の場合は、こちらの都合で send するため、クライアントが遅延リクエストを送ることはない
	time_t  m_time_WS_Read_Hndlr;

	// 遅延リクエストを指示した場合、ここに記録される（遅延の必要がない場合は 0 に設定される）
	time_t  m_rrq_time_InitRI_CrtUsr = 0;  // InitRI と CrtUsr は最初の１回だけであるから、まとめて扱っても問題ないはず

	// --------------------------------------------
	//【注意】pdata_payload の前 4 bytes に書き込みが行われることに要注意
	// 1) pdata_payload - 2 or pdata_payload - 4 のところにヘッダを書き込む
	// 2) size は 64 kB まで
	inline void  Make_AsioWrtBuf_with_PayloadHdr(uint8_t* pdata_payload, size_t size_payload);
	// 何らかのエラーで、規定秒数後に、再送信の設定を行う
	// リクエスト＋エラーと、規定秒数をペイロードで送る(ペイロードは 4 bytes)
	// 送信にはプライベートバッファが利用されることに注意。念の為に、Reset_prvt_buf_write() はコールされる
	void  Set_AsioWrtBuf_with_PLHdr_to_RRQ(uint16_t rereq_jsc, uint16_t sec_wait);
};

// -------------------------------------------------------------

// size は、ペイロードデータ部分のバイト数を表す
inline void  KClient_Chat::Make_AsioWrtBuf_with_PayloadHdr(uint8_t* pdata_payload, size_t size)
{
	if (size <= 125)
	{
		// 0x82: FIN(0x80) | バイナリフレーム(0x2)
		uint8_t*  pbuf_top = pdata_payload - 2;
		*(uint16_t*)pbuf_top = uint16_t(0x82 | (size << 8));
		m_pKClnt->m_asio_prvt_buf_write.data_ = pbuf_top;
		m_pKClnt->m_asio_prvt_buf_write.size_ = size + 2;
	}
	else
	{
		uint8_t*  pbuf_top = pdata_payload - 4;
		*(uint32_t*)pbuf_top = uint32_t(0x82 | (126 << 8) | ((size & 0xff00) << 16) | ((size & 0xff) << 24));
		m_pKClnt->m_asio_prvt_buf_write.data_ = pbuf_top;
		m_pKClnt->m_asio_prvt_buf_write.size_ = size + 4;
	}
}
