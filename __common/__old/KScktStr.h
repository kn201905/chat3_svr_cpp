#pragma once

#include "__general.h"

//////////////////////////////////////////////////////////////

namespace N_ScktStr
{
	enum
	{
		EN_MAX_LEN_IP = 40,  // 想定としては 14 - 22 文字 or 39 文字
		EN_MAX_LEN_SockID = 24  // 想定としては 20 文字
	};
	enum : uint64_t { EN_FLG_longIP = 0x8000'0000'0000'0000 };
	enum : uint8_t { EN_FLG_longIP_8 = 0x80, EN_MASK_longIP_8 = 0x7f };
}

// -----------------------------------------------------------

struct  KRecd_IP
{
	// IPv4 なら 14 - 22文字、IPv6 なら 39文字まで
	// ma_IP[3] の bit63 が、1 であるとき、残りの ma_IP[4], ma_IP[5] を検査する
	uint64_t  ma_IP[5];

	// ========================================
	bool  IsEqualTo(const KRecd_IP* cpcmp) const;
	uint64_t  IsLongIP() const
	{ return  ma_IP[2] & N_ScktStr::EN_FLG_longIP; }
	// KgLog に出力することを考えて、戻り値を uint16_t にしている
	uint16_t  Get_bytes_toLog() const
	{ return  ma_IP[2] & N_ScktStr::EN_FLG_longIP ? 40 : 24; }
};

// -----------------------------------------------------------
// KRecd_IP の utf8 へのシリアライザ
// 本来は、KRecd_IP に継承させるべき。しかし vtable 領域がもったいないため、現在の実装にしている

class  KRecd_IP_Srlzr_toTxt final : public  KSrlzr_toTxt
{
	enum { EN_Qry_MaxBytes = 120 };
public:
	KRecd_IP_Srlzr_toTxt() = default;
	KRecd_IP_Srlzr_toTxt(const KRecd_IP* pRecd_IP) : m_pRecd_IP{ pRecd_IP } {}

	void  SetPtr(const KRecd_IP* pRecd_IP) { m_pRecd_IP = pRecd_IP; }
private:
	// 24 or 40 bytes のバッファを指したポインタを設定して、KFile_W::Write() などをコールすること
	const KRecd_IP*  m_pRecd_IP = NULL;

public:
	// エラーメッセージを考慮して、EN_Qry_MaxBytes としている（エラーがなければ 39 bytes）
	static constexpr size_t  MSCE_Qry_MaxBytes() { return EN_Qry_MaxBytes; }
	virtual size_t  Qry_MaxBytes() const override { return EN_Qry_MaxBytes; }
	virtual int  Ntfy_SrlzTo(uint8_t* pbuf_dst) const override;
};

// -----------------------------------------------------------

struct  KRecd_sockID
{
	uint64_t  ma_sockID[3];
};

//////////////////////////////////////////////////////////////

inline bool  KRecd_IP::IsEqualTo(const KRecd_IP* cpcmp) const
{
	if (this->ma_IP[0] == cpcmp->ma_IP[0] && this->ma_IP[1] == cpcmp->ma_IP[1])
	{
		const uint64_t  ui64_at2 = this->ma_IP[2];
		if (ui64_at2 == cpcmp->ma_IP[2])
		{
			if ((ui64_at2 & N_ScktStr::EN_FLG_longIP) == 0)
			{
				return  true;
			}
			else
			{
				if (this->ma_IP[3] == cpcmp->ma_IP[3] && this->ma_IP[4] == cpcmp->ma_IP[4])
				{
					return  true;
				}
			}
		}
	}
	return  false;
}

