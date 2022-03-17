#ifndef OTHKA_COMM_SOCKETTCP_H_
#define OTHKA_COMM_SOCKETTCP_H_

#include "socket.h"

namespace othka
{
	namespace comm
	{
		/// \brief  提供一个简单的socket的封装
		class SocketTCP : public Socket
		{
		public:
			/// \brief    简单的TCP的封装
			SocketTCP();

			/// \brief    客户端用于构建一个连接
			/// \param    port          端口号
			/// \param    ip_address     主机ip地址
			bool Connect(const std::string& ip_address, int port);

			/// \brief    客户端用于构建一个连接
			/// \param    port          端口号
			/// \param    ip_address     主机ip地址
			/// \param	  out_time      响应超时时间(单位毫秒)
			bool Connect(const std::string& ip_address, int port, int out_time);

			/// \brief    接受传入的客户端socket构建连接
			/// \param    sock_accept    传入客户端的socket
			bool MakeConnect(SocketTCP* sock_accept);

			/// \brief    监听传入的连接
			/// \param    port    端口号
			bool Listen(int port);
			bool Listen(int port, const std::string& ip_address);

		};

	}
}


#endif // OTHKA_COMM_SOCKETTCP_H_
