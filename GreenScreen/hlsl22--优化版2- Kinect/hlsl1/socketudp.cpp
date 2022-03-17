#include <exception>
#include "socketudp.h"
//#include <glog/logging.h>
#define LOG(ERROR) std::cout<<"ERROR"
#define LOG(WARNING) std::cout<<"WARNING"
#define LOG(INFO) std::cout<<"INFO"

namespace othka
{
	namespace comm
	{
		SocketUDP::SocketUDP() : Socket(SOCK_DGRAM)
		{
		}

		SocketUDP::~SocketUDP()
		{
		}

		bool  SocketUDP::Listen(int port)
		{
			return Socket::Listen(port, INADDR_ANY);
		}

		bool SocketUDP::Listen(int port, const std::string& ip_address)
		{
			return Socket::Listen(port, ip_address);
		}

		bool SocketUDP::Connect(const std::string& ip_address, int port)
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

		bool SocketUDP::Connect(const std::string& ip_address, int port, int out_time)
		{
			SOCKET sock_handle;
			sock_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

	}
}

