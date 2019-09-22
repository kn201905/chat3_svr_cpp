#include <cstdint>  // uint64_t など
#include <string.h>  // memcpy

#include "_KString.h"
#include "KScktStr.h"
#include "LogID.h"


////////////////////////////////////////////////////////////////////////

int  KRecd_IP_Srlzr_toTxt::Ntfy_SrlzTo(uint8_t* const pbuf_dst) const
{
	auto  rearrange_chr = [](uint8_t* const pdst, const uint8_t* const psrc)
	{
		pdst[0] = psrc[0];
		pdst[1] = psrc[2];
		pdst[2] = psrc[4];
		pdst[3] = psrc[6];
		pdst[4] = psrc[1];
		pdst[5] = psrc[3];
		pdst[6] = psrc[5];
		pdst[7] = psrc[7];
	};

	// -----------------------------------
	// まずは、ゴミとなる部分も含めて、pbuf_dst に ma_IP を転送する
	const uint8_t*  pui8_src = (uint8_t*)m_pRecd_IP->ma_IP;
	uint8_t*  pui8_dst = pbuf_dst;
	for (int i = 0; i < 5; i++)
	{
		rearrange_chr(pui8_dst, pui8_src);
		pui8_src += 8;
		pui8_dst += 8;
	}

	// pbuf_dst に展開された値の妥当性のチェック（IP の文字烈の長さは最大でも 39文字までと想定している）
	const uint8_t*  pstr_IP = pbuf_dst;
	// longIP のフラグの消去も同時に行っておく
	const uint8_t* const  cptr_tmnt 
		= m_pRecd_IP->IsLongIP() ? (pbuf_dst[23] &= 0x7f, pbuf_dst + 40) : pbuf_dst + 24;

	size_t  len_IP;
	while (true)
	{
		const uint8_t  cchr = *pstr_IP++;
		if (cchr == 0)
		{
			len_IP = pstr_IP - pbuf_dst - 1;  // \0 の１文字分を引く
			while (true)
			{
				if (pstr_IP == cptr_tmnt) { break; }
				if (*pstr_IP++ != 0)
				{
					constexpr KCnstString
						ce_str{ "KRecd_IP_Srlzr_toTxt::Ntfy_SrlzTo() ▶ 0 の検出後に、何らかの文字を検出しました\n" };
					static_assert( ce_str.mc_len <= EN_Qry_MaxBytes );
					memcpy(pbuf_dst, ce_str.mc_cpstr, ce_str.mc_len);
					return  -ce_str.mc_len;
				}
			}
			break;
		}

		if (cchr < 0x20 || cchr > 0x7e)
		{
			constexpr KCnstString
				ce_str{ "KRecd_IP_Srlzr_toTxt::Ntfy_SrlzTo() ▶ 想定外のキャラクタコードを検出しました\n" };
			static_assert( ce_str.mc_len <= EN_Qry_MaxBytes );
			memcpy(pbuf_dst, ce_str.mc_cpstr, ce_str.mc_len);
			return  -ce_str.mc_len;
		}

		if (pstr_IP == cptr_tmnt)
		{
			constexpr KCnstString
				ce_str{ "KRecd_IP_Srlzr_toTxt::Ntfy_SrlzTo() ▶ 規定文字数を超える IP を検出しました\n" };
			static_assert( ce_str.mc_len <= EN_Qry_MaxBytes );
			memcpy(pbuf_dst, ce_str.mc_cpstr, ce_str.mc_len);
			return  -ce_str.mc_len;
		}
	}

	// a_decoded_IP に、len_IP 文字分の char が設定された
	if (len_IP == 0 || len_IP > 39)
	{
		constexpr KCnstString
			ce_str{ "KRecd_IP_Srlzr_toTxt::Ntfy_SrlzTo() ▶ 0文字、または、39文字を超える IP を検出しました\n" };
		static_assert( ce_str.mc_len <= EN_Qry_MaxBytes );
		memcpy(pbuf_dst, ce_str.mc_cpstr, ce_str.mc_len);
		return  -ce_str.mc_len;
	}

	return  len_IP;
}


