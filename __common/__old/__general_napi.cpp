
#include "__general_napi.h"
#include "_KString.h"
#include "LogID.h"

////////////////////////////////////////////////////////////////////////

const char*  G_Get_cstr_typeof(napi_env env, napi_value value)
{
	napi_valuetype val_type;
	if (napi_typeof(env, value, &val_type) != napi_ok) { return "ERR: napi_typeof()"; }

	switch (val_type)
	{
		case  napi_undefined:	return "napi_undefined";
		case  napi_null:		return  "napi_null";
		case  napi_boolean:		return  "napi_boolean";
		case  napi_number:		return  "napi_number";
		case  napi_string:		return  "napi_string";
		case  napi_symbol:		return  "napi_symbol";
		case  napi_object:		return  "napi_object";
		case  napi_function:	return  "napi_function";
		case  napi_external:	return  "napi_external";
		case  napi_bigint:		return  "napi_bigint";
		default:				return  "napi_???";
	}
}

// ------------------------------------------------------

napi_value  G_Get_JSstr_typeof(napi_env env, napi_value value)
{
	napi_value  ret_val;  // ret_val: 構造体へのポインタ
	if (napi_create_string_utf8(env, G_Get_cstr_typeof(env, value), NAPI_AUTO_LENGTH, &ret_val) != napi_ok)
	{ return  nullptr; }

	return  ret_val;
}

////////////////////////////////////////////////////////////////////////
// 戻り値 エラーコード ▶ 0: エラーなし、1: エラーあり

char16_t  KNAPI::msa_NAPIstr_IP[N_ScktStr::EN_MAX_LEN_IP];

#pragma GCC diagnostic ignored "-Wstrict-aliasing"

int  KNAPI::TrnsIP_frmNAPI_toKRecdIP(napi_env env, napi_value napiv_IP, KRecd_IP* const o_recd_IP)
{
	*(uint64_t*)(msa_NAPIstr_IP + 12) = 0;  // 13 文字目以降をゼロクリア（imm命令に移行予定）
	*(uint64_t*)(msa_NAPIstr_IP + 16) = 0;
	*(uint64_t*)(msa_NAPIstr_IP + 20) = 0;
	*(uint64_t*)(msa_NAPIstr_IP + 24) = 0;
	*(uint64_t*)(msa_NAPIstr_IP + 28) = 0;
	*(uint64_t*)(msa_NAPIstr_IP + 32) = 0;
	*(uint64_t*)(msa_NAPIstr_IP + 36) = 0;

	size_t  num_ret;
	if (napi_get_value_string_utf16(env, napiv_IP, msa_NAPIstr_IP, 40, &num_ret) != napi_ok)
	{
		constexpr KCnstString  ce_str{ "KScktStr::TrnsIP_frmNAPI_toEStr() ▶ fail: napi_get_value_string_utf16()" };
		mc_pgLog->WriteID_with_UnixTime(N_LogID::EN_ERR_STR, ce_str.mc_cpstr, ce_str.mc_len);
		return  1;
	}
	if (num_ret < 14 || num_ret > 39)
	{
		mc_pgLog->WriteID_with_UnixTime(N_LogID::EN_ERR_IP_ill_str, msa_NAPIstr_IP, num_ret * 2);
		return  1;
	}

	// -----------------------------------
	const uint64_t  cui64_1st = *(uint64_t*)msa_NAPIstr_IP;  // 1 - 4 文字目
	const uint64_t  cui64_2nd = *(uint64_t*)(msa_NAPIstr_IP + 4);  // 5 - 8 文字目
	o_recd_IP->ma_IP[0] = cui64_1st | (cui64_2nd << 8);

	// -----------------------------------
	const uint64_t  cui64_3rd = *(uint64_t*)(msa_NAPIstr_IP + 8);  // 9 - 12 文字目
	const uint64_t  cui64_4th = *(uint64_t*)(msa_NAPIstr_IP + 12);  // 13 - 16 文字目
	o_recd_IP->ma_IP[1] = cui64_3rd | (cui64_4th << 8);

	// -----------------------------------
	const uint64_t  cui64_5th = *(uint64_t*)(msa_NAPIstr_IP + 16);  // 17 - 20 文字目
	const uint64_t  cui64_6th = *(uint64_t*)(msa_NAPIstr_IP + 20);  // 21 - 24 文字目

	if (num_ret < 25)  // IPv4 の処理（または v6 でも文字列が短い場合）
	{
		o_recd_IP->ma_IP[2] = cui64_5th | (cui64_6th << 8);
	}
	else  // IPv6 の場合
	{
		o_recd_IP->ma_IP[2] = cui64_5th | (cui64_6th << 8) | N_ScktStr::EN_FLG_longIP;

		const uint64_t  cui64_7th = *(uint64_t*)(msa_NAPIstr_IP + 24);  // 25 - 28 文字目
		const uint64_t  cui64_8th = *(uint64_t*)(msa_NAPIstr_IP + 28);  // 29 - 32 文字目
		o_recd_IP->ma_IP[3] = cui64_7th | (cui64_8th << 8);

		const uint64_t  cui64_9th = *(uint64_t*)(msa_NAPIstr_IP + 32);  // 33 - 36 文字目
		const uint64_t  cui64_10th = *(uint64_t*)(msa_NAPIstr_IP + 36);  // 37 - 39 文字目
		o_recd_IP->ma_IP[4] = cui64_9th | (cui64_10th << 8);
	}

	return  0;
}


// -----------------------------------------------------------------------

char16_t  KNAPI::msa_NAPIstr_SockID[N_ScktStr::EN_MAX_LEN_SockID] = {};  // NAPI からの受け取り用の 48bytesバッファ

int  KNAPI::TrnsSockID_frmNAPI_toKRecdSockID(napi_env env, napi_value napiv_sockID, KRecd_sockID* const o_pRecd_sockID)
{
	*(uint64_t*)(msa_NAPIstr_SockID + 20) = 0;  // 21 - 24 文字目をクリア（sockID は 20文字）

	// sockID を C用に変換
	size_t  num_ret;
	if (napi_get_value_string_utf16(env, napiv_sockID, msa_NAPIstr_SockID, 24, &num_ret) != napi_ok)
	{
		constexpr KCnstString  ce_str{
			"KScktStr::TrnsSockID_frmNAPI_toKRecdSockID() ▶ fail: napi_get_value_string_utf16()" };
		mc_pgLog->WriteID_with_UnixTime(N_LogID::EN_ERR_STR, ce_str.mc_cpstr, ce_str.mc_len);
		return  1;
	}

	// -----------------------------------
	const uint64_t  cui64_1st = *(uint64_t*)msa_NAPIstr_SockID;  // 1 - 4 文字目
	const uint64_t  cui64_2nd = *(uint64_t*)(msa_NAPIstr_SockID + 4);  // 5 - 8 文字目
	o_pRecd_sockID->ma_sockID[0] = cui64_1st | (cui64_2nd << 8);

	// -----------------------------------
	const uint64_t  cui64_3rd = *(uint64_t*)(msa_NAPIstr_SockID + 8);  // 9 - 12 文字目
	const uint64_t  cui64_4th = *(uint64_t*)(msa_NAPIstr_SockID + 12);  // 13 - 16 文字目
	o_pRecd_sockID->ma_sockID[1] = cui64_3rd | (cui64_4th << 8);

	// -----------------------------------
	const uint64_t  cui64_5th = *(uint64_t*)(msa_NAPIstr_SockID + 16);  // 17 - 20 文字目
	const uint64_t  cui64_6th = *(uint64_t*)(msa_NAPIstr_SockID + 20);  // 21 - 24 文字目
	o_pRecd_sockID->ma_sockID[2] = cui64_5th | (cui64_6th << 8);

	if (num_ret != 20)
	{
		constexpr KCnstString  ce_str{
			"KScktStr::TrnsSockID_frmNAPI_toKRecdSockID() ▶ socket.id が 20文字ではありませんでした" };
		mc_pgLog->WriteID_with_UnixTime(N_LogID::EN_ERR_STR, ce_str.mc_cpstr, ce_str.mc_len);
		return  1;
	}

	return  0;
}

#pragma GCC diagnostic warning "-Wstrict-aliasing"
