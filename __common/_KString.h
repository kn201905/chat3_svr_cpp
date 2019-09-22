#pragma once

#include <cstddef>
//#define _GNU_SOURCE
#include <string.h>  // mempcpy()

//////////////////////////////////////////////////////////////////////////
// 文字列バッファサイズのカウント

// CE_StrBuf は「/0」を含む
constexpr int  CE_StrBuf(const char* const cpcstr)
{
	const char*  pbuf = cpcstr;
	while (*pbuf++ != 0);
	return  (pbuf - cpcstr);
}

// CE_StrLen_wo0 は「/0」はカウントに入れない
constexpr int  CE_StrLen(const char * const cpcstr)
{
	const char*  pbuf = cpcstr;
	while (*pbuf != 0) { pbuf++; }
	return  (pbuf - cpcstr);
}

// 関数の引数に constexpr を利用したい場合は以下を利用する
// 1. constexpr 関数の戻り値を定数化するには、constexpr 変数で受ける必要がある
// 2. 引数に const char* const を利用すると、constexpr としてコンパイルされない
// 3. CE_PSTR には、constexpr const char* const を渡したほうがよい（cpp ファイルで static 宣言したものを想定）
//　　constexpr がなくてもコンパイル時コンパイルをしてくれるが、エディタ上で警告が表示される
// 注意：Debug 版では、オーバーヘッドが大きくなるため注意
#define CE_F_StrBuf(CE_PSTR) ([&](){constexpr int ret_val = CE_StrBuf(CE_PSTR); return ret_val;}())
#define CE_F_StrLen(CE_PSTR) ([&](){constexpr int ret_val = CE_StrLen(CE_PSTR); return ret_val;}())

//////////////////////////////////////////////////////////////////////////

#define  KSTR4(a, b, c, d) ((((uint32_t)(d)) << 24) | (((uint32_t)(c)) << 16) | (((uint32_t)(b)) << 8) | (a))

// 戻り値は、pdst にコピーした最後の文字のアドレス（通常であれば、\0 が入っているアドレス）
inline char*  G_StrCpy(char* pdst, const char* psrc, size_t sz_buf)
{
	while (true)
	{
		if ((*pdst = *psrc) == 0) { return  pdst; }
		if (--sz_buf == 0) { return  pdst; }
		psrc++;
		pdst++;
	}
}

//////////////////////////////////////////////////////////////////////////

struct  KCnstString
{
	constexpr KCnstString(const char* const  pstr)
		: mc_cpstr{ pstr }
		, mc_len{ CE_StrLen(pstr) }
	{}

	const char* const  mc_cpstr;
	const int  mc_len;

	// 戻り値は、pdst + mc_len（\0 は書き込まれないことに注意）
	char*  WriteTo(void* pdst) const
	{ return  (char*)(memcpy(pdst, mc_cpstr, mc_len)) + mc_len; }

	// 0 以上の値が返される。chr が見つからなかった場合は -1 が返される
	constexpr int  FindIdxOf(const char chr) const {
		const char*  psrc = mc_cpstr;
		while (true)
		{
			const char  chr_src = *psrc++;
			if (chr_src == 0) { return  -1; }
			if (chr_src == chr) { return  (psrc - mc_cpstr) - 1; }
		}
	}
};

//////////////////////////////////////////////////////////////////////////

// ヒープにメモリを確保して、動的に文字列を書き換えたい場合のクラス（文字数は固定）
struct  KString
{
	KString(const char* const  pstr)
		: mc_len{ CE_StrLen(pstr) }
		, mc_pstr{ new char[mc_len + 1] } {
		memcpy(mc_pstr, pstr, mc_len + 1);
	}
	~KString() {
		delete[]  mc_pstr;
	}

	KString(const KString&) = delete;
	KString& operator=(const KString&) = delete;
	KString& operator=(KString&&) = delete;

	const int  mc_len;
	char* const  mc_pstr;

	// 戻り値は、pdst + mc_len（\0 は書き込まれないことに注意）
	char*  WriteTo(void* pdst) const
	{ return  (char*)(memcpy(pdst, mc_pstr, mc_len)) + mc_len; }

	// 0 以上の値が返される。chr が見つからなかった場合は -1 が返される
	int  FindIdxOf(const char chr) const {
		const char*  psrc = mc_pstr;
		while (true)
		{
			const char  chr_src = *psrc++;
			if (chr_src == 0) { return  -1; }
			if (chr_src == chr) { return  (psrc - mc_pstr) - 1; }
		}
	}

	char*  FindPtrOf(const char chr) const {
		char*  psrc = mc_pstr;
		while (true)
		{
			const char  chr_src = *psrc;
			if (chr_src == 0) { return  NULL; }
			if (chr_src == chr) { return  psrc; }
			psrc++;
		}
	}
};

//////////////////////////////////////////////////////////////////////////

namespace  N_toStr
{
	// 先頭を空白で埋めるもの → _Spc
	void  UINT_to_6Dec_Spc(uint64_t num, char* o_pdst);
}

