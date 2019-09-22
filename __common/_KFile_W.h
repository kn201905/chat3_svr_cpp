#pragma once
#include <stdio.h>
//#include "__general.h"  // KSrlzr_toTxt の利用
#include "_KString.h"

class  KFile_W
{
	// 確保されるメモリは Buf + pad となる
	// １回の書き込みは、pad サイズを超えないこと
	enum { EN_bytes_Buf = 10 * 1000, EN_bytes_Pad = 1000 };

public:
	enum class EN_Err : int { OK,  ERR };

	// ========================================
	// bytes_Buf で確保されたバッファがフルになると、ディスクへの書き出しが実行される
	KFile_W(size_t bytes_Buf = EN_bytes_Buf, size_t bytes_Pad = EN_bytes_Pad);
	// デストラクタが実行されるとき、Flush() もコールされる
	~KFile_W();

	// ----------------------------------------
	// 指定されたファイル名のファイルを、mode "a" で開く
	EN_Err  OpenFile_mode_a(const char* cpstr_filename);
	// 指定されたファイル名のファイルを、mode "w" で開く
	EN_Err  OpenFile_mode_w(const char* cpstr_filename);

	// 改行だけを書き込む
	EN_Err  Write_LF();
	// 空白を１文字書き込む
	EN_Err  Write_SPC();

	//「MMDD-HH:MM_」の 11文字を utf-8 で書き込む
	EN_Err  Write_MMDHM_onUnixTime(time_t time_unix);

	EN_Err  Write(const void* cpsrc, size_t bytes);
	EN_Err  Write_LF(const void* cpsrc, size_t bytes);

	EN_Err  Write(const char* cpcstr);  // \0 で終わる文字列であること
	EN_Err  Write_LF(const char* cpcstr);  // \0 で終わる文字列であること

	EN_Err  Write(const KCnstString& rcnst_str)
	{ return  this->Write(rcnst_str.mc_cpstr, rcnst_str.mc_len); }

//	EN_Err  Write(const KSrlzr_toTxt* psrlzr);
//	EN_Err  Write_LF(const KSrlzr_toTxt* psrlzr);

	// Blk にデータが残っている場合、それを全て書き出す
	EN_Err  Flush();

	// 直接メモリバッファに書き込む場合は、以下を利用する（KSrlzr_toKFileW を利用することを推奨）
	char*  Get_ptr_next() { return  m_ptr_next; }
	// m_ptr_next を bytes だけ進めて、必要があれば Blk の内容を書き出す
	EN_Err  Adv_ptr_next(size_t bytes);

	// １度に書き込みできる最大サイズが返される
	size_t  Get_MaxBytes() { return  mc_bytes_pad; }

private:
	// ========================================
	char* const  mc_ptop_buf;
	char* const  mc_ptmnt_buf;
	const size_t  mc_bytes_pad;

	EN_Err  m_ErrStt = EN_Err::OK;
	char*  m_ptr_next = mc_ptop_buf;  // 次に書き込む位置

	FILE*  m_pFile = NULL;
};
