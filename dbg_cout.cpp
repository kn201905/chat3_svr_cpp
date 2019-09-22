#include <iostream>

// -------------------------------------------------------------------

void  G_cout_ui64(uint64_t val)
{
	auto  tohex = [](uint8_t chr) -> char { return  chr < 10 ? chr + 0x30 : chr + 0x30 + 7 + 0x20; };
	char  hex[4] = { 0, 0, 0x20, 0 };
	std::string  str;

	uint8_t  ary_bytes[8];
	for (int i = 7; i >= 0; --i)
	{
		ary_bytes[i] = val & 0xff;
		val >>= 8;
	}

	for (int i = 0; i < 8; ++i)
	{
		hex[0] = tohex( ary_bytes[i] >> 4 );
		hex[1] = tohex( ary_bytes[i] & 0xf );
		str += hex;

		if ((i & 3) == 3) { str += ' '; }
	}
	std::cout << str << std::endl;
}

// -------------------------------------------------------------------

void  G_cout_addr(const void* const cptr)
{
	G_cout_ui64((uint64_t)cptr);
}

// -------------------------------------------------------------------

void  G_cout_xbytes(const uint8_t* psrc, const size_t bytes)
{
	auto  tohex = [](uint8_t chr) -> char { return  chr < 10 ? chr + 0x30 : chr + 0x30 + 7 + 0x20; };
	char  hex[4] = { 0, 0, 0x20, 0 };
	std::string  str;

	for (int i = 0; i < bytes; ++i)
	{
		uint8_t  a = *psrc++;
		hex[0] = tohex( a >> 4 );
		hex[1] = tohex( a & 0xf );
		str += hex;

		if ((i & 3) == 3) { str += ' '; }
	}
	std::cout << str << std::endl;

	return;
}

// -------------------------------------------------------------------

void  G_cout_16bytes(const uint8_t* psrc)
{
	auto  tohex = [](uint8_t chr) -> char { return  chr < 10 ? chr + 0x30 : chr + 0x30 + 7 + 0x20; };
	char  hex[4] = { 0, 0, 0x20, 0 };
	std::string  str;

	for (int i = 0; i < 16; ++i)
	{
		uint8_t  a = *psrc++;
		hex[0] = tohex( a >> 4 );
		hex[1] = tohex( a & 0xf );
		str += hex;

		if ((i & 3) == 3) { str += ' '; }
	}
	std::cout << str << std::endl;

	return;
}

// -------------------------------------------------------------------

void  G_cout_ui32(uint32_t srcval)
{
	auto  tohex = [](uint8_t chr) -> char { return  chr < 10 ? chr + 0x30 : chr + 0x30 + 7 + 0x20; };
	auto  cout_bytes = [&](uint8_t byte, char* pdst) {
		*pdst++ = tohex( byte >> 4);
		*pdst = tohex( byte & 0xf );
	};
	char  hexes[] = "00 00 00 00  ";

	cout_bytes( srcval >> 24, hexes);

	srcval &= 0xff'ffff;
	cout_bytes( srcval >> 16, hexes + 3);

	srcval &= 0xffff;
	cout_bytes( srcval >> 8, hexes + 6);

	cout_bytes( srcval & 0xff, hexes + 9);

	std::cout << hexes;
}

void  G_out_ui32_8(uint32_t* psrc_ui32)
{
	for (int i = 0; i < 4; ++i)
	{ G_cout_ui32(*psrc_ui32++); }
	std::cout << ' ';

	for (int i = 0; i < 4; ++i)
	{ G_cout_ui32(*psrc_ui32++); }
	std::cout << std::endl;
}

void  G_out_ui8_32(const uint8_t* psrc_ui8)
{
	for (int i = 0; i < 4; ++i)
	{
		uint32_t  val_ui32 = (*psrc_ui8++ << 24) + (*psrc_ui8++ << 16) + (*psrc_ui8++ << 8) + *psrc_ui8++;
		G_cout_ui32(val_ui32);
	}
	std::cout << ' ';

	for (int i = 0; i < 4; ++i)
	{
		uint32_t  val_ui32 = (*psrc_ui8++ << 24) + (*psrc_ui8++ << 16) + (*psrc_ui8++ << 8) + *psrc_ui8++;
		G_cout_ui32(val_ui32);
	}
	std::cout << std::endl;
}

void  G_out_ui8_32chr(const char* psrc)
{
	char  str[32 + 9 + 1];		// スペース 9文字 + \0
	char*  pdst = str;
	for (int i = 0; i < 32; ++i)
	{
		*pdst++ = *psrc++;
		if ((i & 3) == 3)
		{
			*pdst++ = 0x20;
			if (i == 15) { *pdst++ = 0x20; }
		}
	}
	*pdst = 0;

	std::cout << str << std::endl;
}
