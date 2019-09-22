#pragma once

#include <node_api.h>
#include "../KgLog/src/KgLog.h"
#include "KScktStr.h"

// NAPI アクセス関連は、インクルードファイルの都合で別ファイルとした

/////////////////////////////////////////////////////////

const char*  G_Get_cstr_typeof(napi_env env, napi_value value);
napi_value  G_Get_JSstr_typeof(napi_env env, napi_value value);


// ------------------------------------------------------
// NAPI アクセスによるエラーを記録するために、KgLog を保持させておく

class  KNAPI
{
public:
	KNAPI(KgLog* pgLog) : mc_pgLog{ pgLog } {}

	// ========================================
	// 戻り値 エラーコード ▶ 0: エラーなし、1: エラーあり
	int  TrnsIP_frmNAPI_toKRecdIP(napi_env env, napi_value napiv_IP, KRecd_IP* o_pRecd_IP);

	// 戻り値 エラーコード ▶ 0: エラーなし、1: エラーあり
	int  TrnsSockID_frmNAPI_toKRecdSockID(napi_env env, napi_value napiv_sockID, KRecd_sockID* o_pRecd_sockID);

private:
	// ========================================
	KgLog* const  mc_pgLog;

	static char16_t  msa_NAPIstr_IP[N_ScktStr::EN_MAX_LEN_IP];  // NAPI からの受け取り用の 80bytesバッファ
	static char16_t  msa_NAPIstr_SockID[N_ScktStr::EN_MAX_LEN_SockID];  // NAPI からの受け取り用の 48bytesバッファ
};

