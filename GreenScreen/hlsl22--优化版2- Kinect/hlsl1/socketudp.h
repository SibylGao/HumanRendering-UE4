#ifndef OTHKA_COMM_SOCKETUDP_H_
#define OTHKA_COMM_SOCKETUDP_H_

#include "socket.h"

namespace othka
{
	namespace comm
	{
		/// \brief    简单的udp 的封装
		class SocketUDP : public Socket
		{
		public:
			/// \brief   简单的udp的封装
			SocketUDP();

			/// \brief  udp的析构
			~SocketUDP();

			/// \brief    监听传入的连接
			/// \param    port      端口号
			bool Listen(int port);

			/// \brief    监听传入的连接
			/// \param    port        端口号
			/// \param    address     主机的ip地址
			bool Listen(int port, const std::string& ip_address);

			/// \brief    客户端用于构建一个连接
			/// \param    port          端口号
			/// \param    ip_address     主机ip地址
			bool Connect(const std::string& ip_address, int port);

			/// \brief    客户端用于构建一个连接
			/// \param    port          端口号
			/// \param    ip_address     主机ip地址
			/// \param	  out_time      响应超时时间(单位毫秒)
			bool Connect(const std::string& ip_address, int port, int out_time);

		};

	}
}

#endif // OTHKA_COMM_SOCKETUDP_H_
