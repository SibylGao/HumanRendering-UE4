#ifndef OTHKA_COMM_SOCKET_H_
#define OTHKA_COMM_SOCKET_H_

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <exception>


//#ifdef WIN32
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN 
#define _WINSOCKAPI_ 
#endif
#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
//#endif

#ifndef _MSC_VER
#include <stdlib.h> // getenv
#endif
#pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
#pragma warning(disable : 4996)

//#include <othka/utils/platform.h>

#define ARG_ACCEPT int
//#include <othka/BASE/link_pragmas.h>
namespace othka
{
	namespace comm
	{

		/// \brief  定义一个简单的socket管理
		class Socket
		{
		public:
			class Exception;

			/// \brief    socket的构造
			/// \param    sock_handle    socket的句柄
			/// \param    sock_address   socket的地址
			/// \param    type          socket的类型，如SOCK_STREAM和SOCK_DGRAM，主要用于tcp的设置SOCK_STREAM，目前默认SOCK_DGRAM指UDP
			void SetSocket(SOCKET sock_handle, struct sockaddr_in sock_address, int type);

			/// \brief    初始化一个socket
			/// \param    port          端口号
			/// \param    ip_address     主机地址
			bool Init(int port, const std::string& ip_address);

			/// \brief    初始化一个socket
			/// \param    port          端口号
			/// \param    ip_address     主机地址
			bool Init(int port, unsigned long ip_address);

			/// \brief 是否创建socket
			bool IsInit();

			/// \brief    关闭连接
			void Close();

			/// \brief    服务端进行监听
			/// \param    port          端口号
			/// \param    ip_address     主机的ip地址
			bool Listen(int port, const std::string& ip_address);

			/// \brief    服务端进行监听
			/// \param    port          端口号
			/// \param    ip_address     主机的ip地址
			bool Listen(int port, unsigned long ip_address);

			/// \brief    等待接收数据
			/// \param    delay       延迟的时间 微妙
			/// \details  如果可以接收数据则返回true
			bool WaitRecv(int delay);

			/// \brief    发送一个字符串
			/// \param    buffer        具体发送的字符串
			bool Send(const std::string& buffer);

			/// \brief    发送具体的buffer
			/// \param    buffer         buffer地址
			/// \param    bufferSize     buffer大小
			bool Send(const char* buffer, size_t bufferSize);

			/// \brief    接收一个字符串
			/// \param    buffer       具体接收的字符串
			/// \param    sizeMax     接收的最大字符数
			bool Recv(std::string& buffer, int sizeMax = -1);

			/// \brief     接收一个buffer
			/// \param    buffer      buffer的地址
			/// \param    bufferSize         buffer的大小
			unsigned Recv(char* buffer, size_t bufferSize);

			/// \brief    设置阻塞模式
			/// \param    blocking   true表示阻塞，false非阻塞
			bool Blocking(bool blocking);

			/// \brief  返回连接的端口号
			int GetPort();

			/// \brief  返回连接的地址
			std::string GetAddress();

			std::vector<std::string> GetAvailableAddress();

		protected:
			/// \brief    构建一个socket 类型是 SOCK_STREAM 或者 SOCK_DGRAM.
			Socket(int type);

			/// \brief  socket的析构
			virtual ~Socket();

			/// \brief  得到最新的错误
			int GetSocketError();

			struct sockaddr_in sock_address_;//!<具体socket的内存区域

			SOCKET sock_handle_;//!<socket的句柄

			bool is_init_;//!<socket是否创建
		private:
			/// \brief socket的初始化，该函数的功能是加载一个Winsocket库版本
			void InitSocket();
			/// \brief 得到WSAEWOULDBLOCK 说明目前操作还是ok的，只不过缓冲区不够，所以下次还是继续发送
			bool OperationOk();
			static bool init_socket_;//!<用于表明socket是否初始化,windows加载版本，对于所有的socket只需要执行一次
			int sock_type_;
			bool is_blocking_;
		};

		/// \brief  对网络异常进行简单封装
		class Socket::Exception : public std::exception
		{
		public:

			Exception(int number, std::string const& message) throw();

			Exception(int number, std::string const& message, int system) throw();

			~Exception() throw();

			const char* what() const throw();

			int GetNumber() const throw();

		private:
			void SetWhatMsg();
			int m_number;
			std::string m_message;
			int m_system;
			std::string m_whatMsg;
		};

	}
}

#endif //OTHKA_COMM_SOCKET_H_
