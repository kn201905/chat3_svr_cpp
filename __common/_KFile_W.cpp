#include <stdio.h>
#include <stdint.h>
#include <string.h>  // memcpy
#include <time.h>

#include "_KString.h"
#include "_KFile_W.h"

/////////////////////////////////////////////////////////////////////

KFile_W::KFile_W(size_t bytes_Buf, size_t bytes_Pad)
	: mc_ptop_buf{ new char[bytes_Buf + bytes_Pad] }
	, mc_ptmnt_buf{ mc_ptop_buf + bytes_Pad }
	, mc_bytes_pad{ bytes_Pad }
{
}

KFile_W::~KFile_W()
{
	if (m_pFile)
	{
		this->Flush();
		fclose(m_pFile);
	}

	delete[]  mc_ptop_buf;
}

/////////////////////////////////////////////////////////////////////

KFile_W::EN_Err  KFile_W::OpenFile_mode_a(const char* const cpstr_filename)
{
	m_pFile = fopen(cpstr_filename, "a");
	if (m_pFile == NULL) { return  (m_ErrStt = EN_Err::ERR); }

	return  EN_Err::OK;
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::OpenFile_mode_w(const char* const cpstr_filename)
{
	m_pFile = fopen(cpstr_filename, "w");
	if (m_pFile == NULL) { return  (m_ErrStt = EN_Err::ERR); }

	return  EN_Err::OK;
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Write_LF()
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }

	*m_ptr_next = '\n';
	return  this->Adv_ptr_next(1);
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Write_SPC()
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }

	*m_ptr_next = ' ';
	return  this->Adv_ptr_next(1);
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Write_MMDHM_onUnixTime(time_t time_unix)
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }

	const tm* const  cptm = localtime(&time_unix);
	const int  ctm_month = cptm->tm_mon + 1;
	const int  ctm_day = cptm->tm_mday;
	const uint64_t  ui64_hour = cptm->tm_hour;
	const uint64_t  ui64_min = cptm->tm_min;

	*(uint32_t*)m_ptr_next = KSTR4(ctm_month / 10, ctm_month % 10, ctm_day / 10, ctm_day % 10) + 0x3030'3030;
	*(uint64_t*)(m_ptr_next + 4) = 0x20'3030'3a30'302d
			+ ((ui64_min % 10) << 40) + ((ui64_min / 10) << 32) + ((ui64_hour % 10) << 16) + ((ui64_hour / 10) << 8);

	return  this->Adv_ptr_next(11);
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Write(const void* const cpsrc, size_t const bytes)
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }
	if (bytes > mc_bytes_pad)
	{
		constexpr KCnstString  ce_str{ "KFile_W::Write(void*, size_t) ▶ ERR: size_t > mc_bytes_pad" };
		this->Write_LF(ce_str.mc_cpstr, ce_str.mc_len);
		return  (m_ErrStt = EN_Err::ERR);
	}

	memcpy(m_ptr_next, cpsrc, bytes);
	return  this->Adv_ptr_next(bytes);
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Write_LF(const void* const cpsrc, size_t const bytes)
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }
	if (bytes + 1 > mc_bytes_pad)
	{
		constexpr KCnstString  ce_str{ "KFile_W::Write_LF(void*, size_t) ▶ ERR: size_t > mc_bytes_pad" };
		this->Write_LF(ce_str.mc_cpstr, ce_str.mc_len);
		return  (m_ErrStt = EN_Err::ERR);
	}

	memcpy(m_ptr_next, cpsrc, bytes);
	*(m_ptr_next + bytes) = '\n';
	return  this->Adv_ptr_next(bytes + 1);
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Write(const char* const cpcstr)
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }

	const char* const  cp_atNull = G_StrCpy(m_ptr_next, cpcstr, mc_bytes_pad);
	if (*cp_atNull != 0)
	{
		constexpr KCnstString  ce_str{ "KFile_W::Write(char*) ▶ ERR: char* の文字列長 > mc_bytes_pad" };
		this->Write_LF(ce_str.mc_cpstr, ce_str.mc_len);
		return  (m_ErrStt = EN_Err::ERR);
	}

	return  this->Adv_ptr_next(cp_atNull - m_ptr_next);
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Write_LF(const char* const cpcstr)
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }

	char* const  cp_atNull = G_StrCpy(m_ptr_next, cpcstr, mc_bytes_pad);
	if (*cp_atNull != 0)
	{
		constexpr KCnstString  ce_str{ "KFile_W::Write_LF(char*) ▶ ERR: char* の文字列長 > mc_bytes_pad" };
		this->Write_LF(ce_str.mc_cpstr, ce_str.mc_len);
		return  (m_ErrStt = EN_Err::ERR);
	}

	*cp_atNull = '\n';
	return  this->Adv_ptr_next(cp_atNull - m_ptr_next + 1);
}

// ------------------------------------------------------------------

#if false

KFile_W::EN_Err  KFile_W::Write(const KSrlzr_toTxt* psrlzr)
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }

	if (psrlzr->Qry_MaxBytes() > mc_bytes_pad)
	{
		constexpr KCnstString  ce_str{ "KFile_W::Write(KSrlzr_toTxt*) ▶ ERR: Qry_MaxBytes() > mc_bytes_pad" };
		this->Write_LF(ce_str.mc_cpstr, ce_str.mc_len);
		return  (m_ErrStt = EN_Err::ERR);
	}

	const int  cbytes_wrt = psrlzr->Ntfy_SrlzTo((uint8_t*)m_ptr_next);
	if (cbytes_wrt > 0)
	{
		return  (m_ErrStt = this->Adv_ptr_next(cbytes_wrt));
	}
	else  // エラーがあった場合の処理（cbytes_wrt < 0）
	{
		m_ErrStt = this->Adv_ptr_next(-cbytes_wrt);
		// Ntfy_SrlzTo() におけるエラーは、KFile_W クラスの障害ではないため、
		// Ntfy_SrlzTo() のエラーは KFile_W のエラーフラグに反映する必要がない
		return  EN_Err::ERR;
	}
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Write_LF(const KSrlzr_toTxt* psrlzr)
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }

	if (psrlzr->Qry_MaxBytes() > mc_bytes_pad - 1)
	{
		constexpr KCnstString  ce_str{ "KFile_W::Write_LF(KSrlzr_toTxt*) ▶ ERR: Qry_MaxBytes() > mc_bytes_pad" };
		this->Write_LF(ce_str.mc_cpstr, ce_str.mc_len);
		return  (m_ErrStt = EN_Err::ERR);
	}

	const size_t  cbytes_wrt = psrlzr->Ntfy_SrlzTo((uint8_t*)m_ptr_next);
	if (cbytes_wrt > 0)
	{
		*(m_ptr_next + cbytes_wrt) = '\n';
		return  (m_ErrStt = this->Adv_ptr_next(cbytes_wrt + 1));
	}
	else  // エラーがあった場合の処理（cbytes_wrt < 0）
	{
		*(m_ptr_next - cbytes_wrt) = '\n';
		m_ErrStt = this->Adv_ptr_next(-cbytes_wrt + 1);
		// Ntfy_SrlzTo() におけるエラーは、KFile_W クラスの障害ではないため、
		// Ntfy_SrlzTo() のエラーは KFile_W のエラーフラグに反映する必要がない
		return  EN_Err::ERR;
	}
}

#endif

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Flush()
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }

	if (m_ptr_next == mc_ptop_buf) { return  EN_Err::OK; }

	const size_t  cbytes_write = m_ptr_next - mc_ptop_buf;
	if (fwrite(mc_ptop_buf, 1, cbytes_write, m_pFile) < cbytes_write)
	{ return  (m_ErrStt = EN_Err::ERR); }

	m_ptr_next = mc_ptop_buf;
	return  EN_Err::OK;
}

// ------------------------------------------------------------------

KFile_W::EN_Err  KFile_W::Adv_ptr_next(size_t bytes)
{
	if (m_ErrStt == EN_Err::ERR) { return  EN_Err::ERR; }

	m_ptr_next += bytes;
	if (m_ptr_next < mc_ptmnt_buf) { return  EN_Err::OK; }

	// mc_ptmnt_buf <= m_ptr_next のとき、ファイルへの書き出しを実行する
	const size_t  cbytes_write = m_ptr_next - mc_ptop_buf;
	if (fwrite(mc_ptop_buf, 1, cbytes_write, m_pFile) < cbytes_write)
	{ return  (m_ErrStt = EN_Err::ERR); }

	m_ptr_next = mc_ptop_buf;
	return  EN_Err::OK;
}
