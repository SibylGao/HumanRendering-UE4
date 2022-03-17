#include <exception>
#include "sockettcp.h"
//#include <glog/logging.h>
#define LOG(ERROR) std::cout<<"ERROR"
#define LOG(WARNING) std::cout<<"WARNING"

namespace othka
{
	namespace comm
	{
		SocketTCP::SocketTCP() : Socket(SOCK_STREAM)
		{
		}

		bool SocketTCP::Connect(const std::string& ip_address, int port)
		{

			bool is_init = Socket::Init(port, inet_addr(ip_address.c_str()));
			if (!is_init)
			{
				LOG(ERROR) << "Failed to create tcp socket.exit!\n";
				return false;
			}

			//创建连接 ，连接成功，返回0，连接失败，返回-1
			if (connect(sock_handle_, (sockaddr *)&sock_address_, sizeof(sockaddr)) == -1)
			{
				//LOG(ERROR) << "SocketTCP::Connect: unable to connect" << GetSocketError() << std::endl;
				return false;
			}
			else
			{
				//客户端连接成功
				LOG(INFO) << "Connected to server." << std::endl;
				return true;
			}

			return true;
		}

		bool SocketTCP::Connect(const std::string& ip_address, int port, int out_time)
		{
			SOCKET sock_handle;
			sock_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sock_handle == INVALID_SOCKET)
			{
				//socket 创建失败
				LOG(ERROR) << "Socket::Init: unable to create the socket" << GetSocketError() << std::endl;
				return false;
			}
			else
			{
				sock_handle_ = sock_handle;
				memset(&sock_address_, 0, sizeof(sock_address_));
				sock_address_.sin_family = AF_INET;
				sock_address_.sin_port = htons(port);
				sock_address_.sin_addr.s_addr = inet_addr(ip_address.c_str());
				is_init_ = true;
				//设置发送超时
				if (SOCKET_ERROR == setsockopt(sock_handle_, SOL_SOCKET, SO_SNDTIMEO, (char *)&out_time, sizeof(int)))
				{
					LOG(ERROR) << "Set SO_SNDTIMEO error!" << std::endl;
				}
				if (SOCKET_ERROR == setsockopt(sock_handle_, SOL_SOCKET, SO_RCVTIMEO, (char *)&out_time, sizeof(int)))
				{
					LOG(ERROR) << "Set SO_RCVTIMEO error!" << std::endl;
				}
			}
			if (!is_init_)
			{
				LOG(ERROR) << "Failed to create tcp socket.exit!\n";
				return false;
			}

			//创建连接 ，连接成功，返回0，连接失败，返回-1
			if (connect(sock_handle_, (sockaddr *)&sock_address_, sizeof(sockaddr)) == -1)
			{
				LOG(ERROR) << "SocketTCP::Connect: unable to connect" << GetSocketError() << std::endl;
				return false;
			}
			else
			{
				//客户端连接成功
				LOG(INFO) << "Connected to server." << std::endl;
				return true;
			}

			return true;
		}

		bool SocketTCP::MakeConnect(SocketTCP* sock_accept)
		{
			ARG_ACCEPT arg;
			SOCKET	sock_client;
			sockaddr_in	sock_address;//客户端

			arg = sizeof(sock_address);
			sock_client = accept(sock_handle_, (sockaddr *)&sock_address, &arg);
			if (sock_client == INVALID_SOCKET)
			{
				LOG(ERROR) << "Failed to accept for client connection" << GetSocketError() << std::endl;
				return false;
			}
			else
			{
				if (sock_accept->IsInit())
				{
					sock_accept->Close();
				}
				sock_accept->SetSocket(sock_client, sock_address, SOCK_STREAM);
				const char disable_delay = 1;
				//使用setsockopt TCP_NODELAY禁用 Nagle算法 https://blog.csdn.net/yahohi/article/details/6717500
				if (setsockopt(sock_client, IPPROTO_TCP, TCP_NODELAY, &disable_delay, sizeof(disable_delay)) != 0)
				{
					LOG(WARNING) << "Failed to set no socket delay, sending data can be delayed by up to 250ms.\n";
				}
				return true;
			}

			return true;
		}

		bool SocketTCP::Listen(int port)
		{
			return Socket::Listen(port, INADDR_ANY);
		}

		bool SocketTCP::Listen(int port, const std::string& ip_address)
		{
			return Socket::Listen(port, ip_address);
		}

	}
}
