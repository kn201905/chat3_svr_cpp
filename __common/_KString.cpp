#include <stdint.h>

#include "_KString.h"

using  uint = unsigned int;

////////////////////////////////////////////////////////////////

void  N_toStr::UINT_to_6Dec_Spc(uint64_t num, char* o_pdst)
{
	if (num > 999'999)
	{
		num %= 100'000;  // 先頭の１文字は ? にする
		uint const  cd2 = num / 10'000;  num %= 10'000;
		*(uint16_t*)o_pdst = uint16_t((cd2 << 8) | 0x303f);  // 0x3f == '?'
		goto  PRINT_4dig;
	}

	if (num > 99'999)
	{
		uint const  cd1 = num / 100'000;  num %= 100'000;
		uint const  cd2 = num / 10'000;  num %= 10'000;
		*(uint16_t*)o_pdst = uint16_t(cd1 | (cd2 << 8) | 0x3030);
		goto  PRINT_4dig;
	}

	if (num > 9'999)
	{
		uint const  cd1 = num / 10'000;  num %= 10'000;
		*(uint16_t*)o_pdst = uint16_t((cd1 << 8) | 0x3020);
		goto  PRINT_4dig;
	}

	// 以下は４桁以下
	*(uint16_t*)o_pdst = 0x2020;

	if (num > 999)
	{
PRINT_4dig:
		uint const  cd1 = num / 1000;  num %= 1000;
		uint const  cd2 = num / 100;  num %= 100;
		*(uint32_t*)(o_pdst + 2) = uint32_t(cd1 | (cd2 << 8) | ((num / 10) << 16) | ((num % 10) << 24) | 0x3030'3030);
		return;
	}

	if (num > 99)
	{
		uint const  cd1 = num / 100;  num %= 100;
		*(uint32_t*)(o_pdst + 2) = uint32_t((cd1 << 8) | ((num / 10) << 16) | ((num % 10) << 24) | 0x3030'3020);
		return;
	}

	if (num > 9)
	{
		*(uint32_t*)(o_pdst + 2) = uint32_t(((num / 10) << 16) | ((num % 10) << 24) | 0x3030'2020);
		return;
	}

	*(uint32_t*)(o_pdst + 2) = uint32_t((num << 24) | 0x3020'2020);
}
