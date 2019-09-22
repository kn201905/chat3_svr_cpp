#include <iostream>
#include <thread>
#include <future>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/exception/all.hpp>

namespace asio = boost::asio;
using boost::asio::ip::tcp;

/////////////////////////////////////////////////////////////////////////////////////

uint8_t*  G_cout_16bytes(uint8_t* psrc)
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

	return  psrc;
}

/////////////////////////////////////////////////////////////////////////////////////

int main() {
	try {
		asio::io_context ioc;
		tcp::socket socket(ioc);

//		boost::asio::socket_base::receive_buffer_size opt_set(5000);
//		socket.set_option(opt_set);

		// 接続
		socket.connect(tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 1234));

		asio::socket_base::send_buffer_size  send_buf_size;
		socket.get_option(send_buf_size);
		std::cout << "send_buffer_size: " << send_buf_size.value() << std::endl;

		// websocket 接続要求
		std::string send_msg = "GET / HTTP/1.1\nHost: test.com\nUpgrade: websocket\n"
				"Connection: upgrade\nSec-WebSocket-Version: 13\nSec-WebSocket-Key: E4WSEcseoWr4csPLS2QJHA==";
		asio::write(socket, asio::buffer(send_msg));

		// websocket 接続要求レスポンス
		asio::streambuf  receive_buffer;

		uint8_t  ui8_buf[5000];
		boost::asio::mutable_buffers_1  recv_buf{ ui8_buf, 5000 };

		const size_t  cread_len = asio::read(socket, recv_buf, asio::transfer_at_least(1));
		std::string  recv_str((char*)ui8_buf, cread_len);

		std::cout << "bytes: " << cread_len << std::endl;
		std::cout << "response: " << recv_str << std::endl;
//		G_cout_16bytes(ui8_buf);

#if false

		std::string input;
		while (std::cin) {
			std::cout << ">> " << std::flush;

			std::getline(std::cin, input);
			if (*input.data() == '0') { break; }

			input += "\n";
			asio::write(socket, asio::buffer(input));

			asio::read(socket, recv_buf, asio::transfer_exactly(1));
			const int clen = *ui8_buf;
			asio::read(socket, recv_buf, asio::transfer_exactly(clen));
			std::string  recv_str((char*)ui8_buf, clen);

			std::cout << "len: " << clen << " / val: " << recv_str << std::endl;



			asio::read(socket, recv_buf, asio::transfer_exactly(1));
			const int clen2 = *ui8_buf;
			asio::read(socket, recv_buf, asio::transfer_exactly(3999));
			std::cout << "len2: " << clen2 << std::endl;
		}
#endif

		socket.close();
	} catch (...) {
		std::cerr << "error: " << boost::current_exception_diagnostic_information() << std::endl;
		return 1;
	}
	return 0;
}
