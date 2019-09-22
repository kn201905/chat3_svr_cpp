#pragma once
#include <cstddef>

/////////////////////////////////////////////////////////////////////
// utf8 へのテキストシリアライザ
// Qry_MaxBytes() で返した結果に対して、Ntfy_SrlzTo() で応答が返ってくる
// このクラスを利用する場合、処理速度は優先されないことに留意

struct  KSrlzr_toTxt
{
	virtual size_t  Qry_MaxBytes() const =0;
	// Qry_MaxBytes() で指定されたバッファが確保できない場合、Ntfy_SrlzTo() はコールされない
	// 戻り値の絶対値で、実際に書き込むバイト数を返す（マイナス値は、エラーがあったことを示す）
	virtual int  Ntfy_SrlzTo(unsigned char* pbuf_dst) const =0;
};
