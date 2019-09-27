#pragma once

#include <stdint.h>
#include <stddef.h>

////////////////////////////////////////////////////////////////

template<size_t bytes_buf_utf8>
struct  KUtf16_toUtf8
{
	uint16_t  m_len_Utf8;  // 変換後の「byte数」がここに設定される（末尾の \0 は含まない）
	uint8_t  ma_buf_Utf8[bytes_buf_utf8];  // 変換後の文字には、末尾に「\0」が付加される
	// utf8 は最大４バイトまでである
	const uint8_t* const  mc_pbuf_Utf8_end = ma_buf_Utf8 + bytes_buf_utf8 - 4;

	// --------------------------------------------
	// len には \0 を含まないこと（len は 2 bytes で１文字とカウントすること）
	// 戻り値： 正しく変換されたら true が返される。false となるのは以下の場合
	// 戻り値が false の場合, m_len_Utf8 = 0 で返される
	// 1) 文字数オーバー（ma_buf_Utf8 のバッファ不足）
	// 2) サロゲートペアの「前半 or 後半」がない場合
	bool  Cvt_toUtf8(const uint16_t* psrc, size_t len)
	{
		uint8_t*  pdst = ma_buf_Utf8;

		while (true)
		{
			const uint16_t  wchr = *psrc++;
			len--;
			uint32_t  utf32_chr;

			// まず、wchr がサロゲートペアであるかどうか判定する
			if (0xdc00 <= wchr && wchr < 0xe000)
			{
				// 1文字目が ローサロゲートであった場合はエラーとする
				m_len_Utf8 = 0;
				return  false;
			}
			if (0xd800 <= wchr && wchr < 0xdc00)
			{
				if (len == 0)
				{
					// ローサロゲートがない場合、エラーとする
					m_len_Utf8 = 0;
					return  false;
				}

				// サロゲートペアの処理（20 bits の文字として処理する）
				utf32_chr = 0x1'0000 + (uint32_t(wchr - 0xd800) << 10) + (*psrc++ - 0xdc00);
				len--;
			}
			else
			{
				utf32_chr = wchr;
			}
			// 以上により、utf32_chr に utf32 の値が設定された
			// --------------------------------------------

			if (utf32_chr < 0x80)  // 7 bits 以下
			{
				// 1 byte 文字
				*pdst++ = uint8_t(utf32_chr);
			}
			else if (utf32_chr < 0x800)  // 11 bits 以下
			{
				*pdst = uint8_t(0xc0 | (utf32_chr >> 6));
				*(pdst + 1) = uint8_t(0x80 | (utf32_chr & 0x3f));
				pdst += 2;
			}
			else if (utf32_chr < 0x1'0000)  // 16 bits 以下
			{
				*pdst = uint8_t(0xe0 | (utf32_chr >> 12));
				*(pdst + 1) = uint8_t(0x80 | ((utf32_chr >> 6) & 0x3f));
				*(pdst + 2) = uint8_t(0x80 | (utf32_chr & 0x3f));
				pdst += 3;
			}
			else if (utf32_chr < 0x10'0000)  // 現在の仕様では、20 bits 以下になる
			{
				*pdst = uint8_t(0xf0 | (utf32_chr >> 18));
				*(pdst + 1) = uint8_t(0x80 | ((utf32_chr >> 12) & 0x3f));
				*(pdst + 2) = uint8_t(0x80 | ((utf32_chr >> 6) & 0x3f));
				*(pdst + 3) = uint8_t(0x80 | (utf32_chr & 0x3f));
				pdst += 4;
			}
			else
			{
				// 21 bits 以上の場合、エラーとする
				m_len_Utf8 = 0;
				return  false;
			}

			if (len == 0) { break; }
			if (pdst > mc_pbuf_Utf8_end)
			{
				m_len_Utf8 = 0;
				return  false;
			}
		}

		// 変換が正しく終了したとき、ここにくる
		*pdst = 0;
		m_len_Utf8 = pdst - ma_buf_Utf8;
		return  true;
	}
};

