#include <exception>
#include "socket.h"
//#include <glog/logging.h>
#define LOG(ERROR) std::cout<<"ERROR"
#define LOG(WARNING) std::cout<<"WARNING"

namespace othka
{
	namespace comm
	{
		bool Socket::init_socket_ = false;

		Socket::Socket(int type)
			: sock_address_(),
			sock_handle_(INVALID_SOCKET),
			is_init_(false),
			sock_type_(type),
			is_blocking_(false)
		{
			memset(&sock_address_, 0, sizeof(sock_address_));
			sock_address_.sin_family = AF_INET;
			sock_address_.sin_port = 0;
			sock_address_.sin_addr.s_addr = 0;
			//主要是windows的版本加载具体的socket api
			if (!Socket::init_socket_) InitSocket();
		}

		Socket::~Socket()
		{
			Close();
		}

		void Socket::InitSocket()
		{
			Socket::init_socket_ = true;

#ifdef WIN32 //对于windows的api函数，一般建议用宏封一下
			WSADATA	WsaData;
			WORD wVersionRequested = MAKEWORD(2, 0);
			if (WSAStartup(wVersionRequested, &WsaData) != 0)
			{
				LOG(ERROR) << "Socket::InitSocket: WSAStartup error" << GetSocketError() << std::endl;
				return;
			}
#endif // WIN32

		}


		void Socket::SetSocket(SOCKET sock_handle, struct sockaddr_in sock_address, int type)
		{
			//主要是服务端accept客户端之后，设置客户端的socket，这个时候客户端的socket是已经初始好了，并已经进行了连接
			sock_type_ = type;
			sock_handle_ = sock_handle;
			sock_address_ = sock_address;
			is_init_ = true;
		}

		int Socket::GetPort()
		{
			return ntohs(sock_address_.sin_port);
		}

		std::string Socket::GetAddress()
		{
			return inet_ntoa(sock_address_.sin_addr);
		}

		std::vector<std::string> Socket::GetAvailableAddress()
		{
			hostent *host;
			char host_name[20];
			gethostname(host_name, 20);
			host = gethostbyname(host_name);
			std::string str_ip_addr;
			std::vector<std::string> str_ip_addr_vec;
			str_ip_addr = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
			for (int i = 0; host->h_addr_list[i]; i++)
			{
				str_ip_addr = inet_ntoa(*(struct in_addr*)host->h_addr_list[i]);
				str_ip_addr_vec.push_back(str_ip_addr);
			}
			return str_ip_addr_vec;
		}

		bool Socket::Init(int port, unsigned long ip_address)
		{
			SOCKET sock_handle;
			if (sock_type_ == SOCK_DGRAM)
			{
				sock_handle = socket(AF_INET, sock_type_, IPPROTO_UDP);
			}
			else if (sock_type_ == SOCK_STREAM)
			{
				sock_handle = socket(AF_INET, sock_type_, IPPROTO_TCP);
			}

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
				sock_address_.sin_addr.s_addr = ip_address;
				is_init_ = true;

				if (sock_type_ == SOCK_DGRAM)//如果是UDP类型，允许广播发送
				{
					int arg = 1;
					// 如果想要在套接字级别上设置选项，就必须把level设置为 SOL_SOCKET,optname为SO_BROADCAST允许发送广播数据
					if (setsockopt(sock_handle_, SOL_SOCKET, SO_BROADCAST, (char*)&arg, sizeof(int)) != 0)
					{
						LOG(WARNING) << "Socket::Init: setsockopt(SO_BROADCAST) error" << GetSocketError() << std::endl;
					}

				}
				else if (sock_type_ == SOCK_STREAM)
				{
					const char disable_delay = 1;
					//使用setsockopt TCP_NODELAY禁用 Nagle算法 https://blog.csdn.net/yahohi/article/details/6717500
					if (setsockopt(sock_handle_, IPPROTO_TCP, TCP_NODELAY, &disable_delay, sizeof(disable_delay)) != 0)
					{
						LOG(WARNING) << "Failed to set no socket delay, sending data can be delayed by up to 250ms.\n";
					}
				}
				return true;
			}
		}

		bool  Socket::Init(int port, const std::string& ip_address)
		{
			return Init(port, inet_addr(ip_address.c_str()));
		}

		bool Socket::IsInit()
		{
			return is_init_;
		}

		void Socket::Close()
		{
			if (sock_handle_ != INVALID_SOCKET)
			{
				//先使用shutdown
#ifdef WIN32
				shutdown(sock_handle_, SD_BOTH);
#else
				shutdown(sock_handle_, SHUT_RDWR);
#endif
				closesocket(sock_handle_);
			}
			sock_handle_ = INVALID_SOCKET;
			is_init_ = false;
		}

		bool Socket::Listen(int port, const std::string& ip_address)
		{
			return Listen(port, inet_addr(ip_address.c_str()));
		}

		bool Socket::Listen(int port, unsigned long ip_address)
		{
			Init(port, ip_address);

			if (sock_type_ == SOCK_STREAM) // 如果是TCP，允许程序崩溃之后重启立即连接到机器人
			{
				int arg = 1;
				if (setsockopt(sock_handle_, SOL_SOCKET, SO_REUSEADDR, (char*)&arg, sizeof(int)) != 0)
				{
					LOG(WARNING) << "Socket::Listen: setsockopt(SO_REUSEADDR) warning:" << GetSocketError() << std::endl;
				}
			}
			// 绑定
			if (bind(sock_handle_, (struct sockaddr*) &sock_address_, sizeof(sock_address_)) != 0)
			{
				LOG(ERROR) << "Socket::Listen: bind error" << GetSocketError() << std::endl;
				closesocket(sock_handle_);
				return false;
			}

			if (sock_type_ == SOCK_STREAM)
			{
				//监听
				if (listen(sock_handle_, SOMAXCONN) == SOCKET_ERROR)
				{
					//TODO：监听失败的话
					closesocket(sock_handle_);
					WSACleanup();
					LOG(ERROR) << "Socket::Listen: listen error" << GetSocketError() << std::endl;
					return false;
				}
			}
			else if (sock_type_ == SOCK_DGRAM)//如果是UDP类型，
			{
				//TODO:不知道这个要不要设置，https://blog.csdn.net/xkou/article/details/1432349
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
				BOOL bNewBehavior = FALSE;
				DWORD dwBytesReturned = 0;
				WSAIoctl(sock_handle_, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior,
					NULL, 0, &dwBytesReturned, NULL, NULL);
			}
			return true;
		}

		bool Socket::WaitRecv(int delay)
		{
			if (sock_handle_ == INVALID_SOCKET)
			{
				WSACleanup();
				LOG(ERROR) << "Socket::WaitRecv: wait error" << GetSocketError() << std::endl;
				return false;
			}

			fd_set read;
			struct timeval tv_delay;

			FD_ZERO(&read);
			FD_SET(sock_handle_, &read);

			tv_delay.tv_sec = 0;
			tv_delay.tv_usec = delay;
			// 对于文件的写和文件异常不考虑
			int result = select(sock_handle_ + 1, &read, nullptr, nullptr, &tv_delay);
			if (result == SOCKET_ERROR)//0：等待超时，没有可读写或错误的文件 
			{
				LOG(ERROR) << "Socket::WaitRecv: wait error" << GetSocketError() << std::endl;
				return false;
			}

			return FD_ISSET(sock_handle_, &read) > 0;
		}

		bool Socket::Send(const char* buffer, size_t buffer_size)
		{
			size_t sent_size;
			size_t max_size;
			int received_bytes_num;

			if (buffer_size == 0)
			{
				LOG(ERROR) << "Socket::Send: Invalid buffer size" << std::endl;
				return false;
			}
			if (buffer == nullptr)
			{
				LOG(ERROR) << "Socket::Send: Unable to send null buffer" << std::endl;
				return false;
			}
			if (!is_init_)
			{
				LOG(ERROR) << "Socket::Send: socket must connected before Send()" << std::endl;
				return false;
			}

			max_size = buffer_size;
			sent_size = 0;

			while (sent_size < max_size)
			{

				switch (sock_type_)
				{
				case SOCK_DGRAM:
					received_bytes_num = sendto(sock_handle_, buffer + sent_size, max_size - sent_size,
						0, (struct sockaddr *)&sock_address_, sizeof(struct sockaddr));
					break;

				case SOCK_STREAM:
					received_bytes_num = send(sock_handle_, buffer + sent_size, max_size - sent_size, 0);
					std::cout << "max_size: " << max_size << std::endl;
					std::cout << "sent_size: " << sent_size << std::endl;
					std::cout << "received_bytes_num: " << received_bytes_num << std::endl;
					break;

				default:
					LOG(ERROR) << "Socket::Send: unknown socket type" << std::endl;
				}

				if ((received_bytes_num == -1) && (!OperationOk()))
				{
					LOG(ERROR) << "Socket::Send: send error" << GetSocketError() << std::endl;
					return false;
				}

				if (received_bytes_num != -1) sent_size += received_bytes_num;

			}
			return true;
		}

		bool Socket::Send(const std::string& buffer)
		{
			return Send(buffer.c_str(), buffer.size());
		}

		
		unsigned Socket::Recv(char* buffer, size_t buffer_size)
		{
			int status;

			if (buffer_size == 0)
			{
				LOG(ERROR) << "Socket::Recv: Invalid buffer size" << std::endl;
				return -1;
			}
			if (buffer == nullptr)
			{
				LOG(ERROR) << "Socket::Recv: Invalid buffer pointer" << std::endl;
				return -1;
			}
			if (!is_init_)
			{
				LOG(ERROR) << "Socket::Recv: Listen() or Connect() method is mandatory before Recv()" << std::endl;
				return -1;
			}

			switch (sock_type_)
			{
			case SOCK_DGRAM:
			{
				int len = sizeof(struct sockaddr);
				status = recvfrom(sock_handle_, buffer, buffer_size, 0, (struct sockaddr *)&sock_address_, (socklen_t *)&len);
				break;
			}
			case SOCK_STREAM:
			{
				//int time_out = 1000;//ms
				//setsockopt(sock_handle_, SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(int));
				status = recv(sock_handle_, buffer, buffer_size, 0);
				break;
			}
			default:
				LOG(ERROR) << "Socket::Recv: unknown socket type" << std::endl;
			}

			if ((status == -1) && (OperationOk()))
			{
				return 0;
			}

			return status;
		}

		bool Socket::Recv(std::string& buffer, int max_size)
		{
			int status;
			char buf[2000];
			bool is_receive = false;
			if (max_size < -1)
			{
				LOG(ERROR) << "Socket::Recv: Invalid buffer size" << std::endl;
				return false;
			}
			if (!is_init_)
			{
				LOG(ERROR) << "Socket::Recv: Listen() or Connect() method is mandatory before Recv()" << std::endl;
				return false;
			}

			buffer = "";

			do
			{
				int buffer_size = sizeof(buf) - 1;
				if ((max_size != -1) && (buffer_size > max_size))
				{
					buffer_size = max_size;
				}
				switch (sock_type_)
				{
				case SOCK_DGRAM:
				{
					timeval tv = { 20, 0 };//ms
					setsockopt(sock_handle_, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(timeval));
					int len = sizeof(struct sockaddr);
					status = recvfrom(sock_handle_, buf, buffer_size, 0, (struct sockaddr *)&sock_address_, (socklen_t *)&len);
					break;
				}
				case SOCK_STREAM:
				{
					int time_out=100;//ms
					setsockopt(sock_handle_, SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(int));
					status = recv(sock_handle_, buf, buffer_size, 0);
					break;
				}
				default:
					LOG(ERROR) << "Socket::Recv: unknown socket type" << std::endl;
				}

				if ((status == -1) && (!OperationOk()))
				{
					//LOG(ERROR) << "Socket::Recv: recv[from] error" << GetSocketError() << std::endl;
					return false;
				}

				if (status != -1)
				{
					buf[status] = '\0';
					buffer.append(buf, status);
					//buffer += buf;
					is_receive = true;
					if (max_size != -1)
					{
						max_size -= status;
						if (max_size == 0) status = -1;
					}
				}
			} while ((status != -1) && (is_blocking_ == false));

			return is_receive;
		}

		bool Socket::Blocking(bool blocking)
		{
			int result;

			if (sock_handle_ == INVALID_SOCKET)
			{
				sock_handle_ = socket(AF_INET, sock_type_, 0);
				if (sock_handle_ == 0)
				{
					LOG(ERROR) << "Socket::Blocking: unable to create the socket" << GetSocketError() << std::endl;
					return false;
				}
			}

#ifdef __linux__
			int flags;
			flags = fcntl(sock_handle_, F_GETFL);
			if (blocking)
				flags = (flags & ~O_NONBLOCK);
			else
				flags = (flags | O_NONBLOCK);
			result = fcntl(sock_handle_, F_SETFL, flags);
#else
			u_long iMode = !blocking;
			result = ioctlsocket(sock_handle_, FIONBIO, &iMode);//imode是1的时候是非阻塞
			if (result != 0) result = -1;
#endif

			if (result == -1)
			{
				LOG(ERROR) << "Socket::Blocking: ioctl error" << GetSocketError() << std::endl;
				return false;
			}

			is_blocking_ = blocking;
			return true;
		}

		int Socket::GetSocketError()
		{
#ifdef __linux__
			return errno;
#else
			return WSAGetLastError();
#endif
		}

		bool Socket::OperationOk()
		{
			int err;

			err = GetSocketError();
#ifdef __linux__
			if (err == EAGAIN || err == EWOULDBLOCK) return true;
#else
			if (err == WSAEWOULDBLOCK) return true;
#endif
			return false;
		}


		Socket::Exception::Exception(int number, std::string const& message) throw()
			: m_number(number), m_message(message), m_system(0), m_whatMsg()
		{
			SetWhatMsg();
		}

		Socket::Exception::Exception(int number, std::string const& message, int system) throw()
			: m_number(number), m_message(message), m_system(system), m_whatMsg()
		{
			SetWhatMsg();
		}

		Socket::Exception::~Exception() throw()
		{
		}

		void Socket::Exception::SetWhatMsg()
		{
			std::ostringstream message;

			message << m_message;
			if (m_system != 0) message << " (system error " << m_system << ")";
			m_whatMsg = message.str();
		}

		const char* Socket::Exception::what() const throw()
		{
			return m_whatMsg.c_str();
		}

		int Socket::Exception::GetNumber() const throw()
		{
			return m_number;
		}

	}
}
