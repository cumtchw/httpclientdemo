#include "HttpSimpleConnect.h"
#include "../../api/APIErrorCode.h"
#include "../tools/PrintLog.h"
#include <fstream>
#include <sstream>


HttpSimpleConnect::HttpSimpleConnect()
{
#ifdef WIN32
	//此处一定要初始化一下，否则gethostbyname返回一直为空
	WSADATA wsa = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

HttpSimpleConnect::~HttpSimpleConnect()
{
#ifdef WIN32
	WSACleanup();
#endif
}
bool HttpSimpleConnect::socketHttpRequest(std::string host, int port, std::string request, std::string &response)
{
	int sockfd;
	struct sockaddr_in dst_addr;
	struct hostent *server;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(port);
	server = gethostbyname(host.c_str());
	if (server == NULL)
	{
		closesock(sockfd);
		return false;
	}
	memcpy((char *)&dst_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
	int ret = -1;
#ifdef WIN32
	unsigned long ul = 1;//非阻塞
	ret = ioctlsocket(sockfd, FIONBIO, (unsigned long*)&ul);
	if (ret == -1)
	{
		closesock(sockfd);
		return -1;
	}
#else

	int flags = fcntl(sockfd, F_GETFL, 0);
	//设置为非阻塞
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		closesock(sockfd);
		return false;
	}
#endif
	int error = -1;
	socklen_t len;
	len = sizeof(int);
	timeval tm;
	fd_set set;

	if (-1 == connect(sockfd, (struct sockaddr *)&dst_addr, sizeof(dst_addr)))
	{
		tm.tv_sec = SOCKET_TIMEOUT;
		tm.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		if (select(sockfd + 1, NULL, &set, NULL, &tm) > 0)//检查是否可写
		{
			getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
			if (error == 0) ret = true;
			else ret = false;
		}
		else
			ret = false;

		//判断connect超时
		if (!ret)
		{
			LogError("Connection HttpServer Error!");
			response = ERROR_CONNECT_FAILED;
			closesock(sockfd);
			return false;
		}
		//LogInfo("connection httpserver success!");
	}

	FD_ZERO(&set);
	FD_SET(sockfd, &set);
	if (select(sockfd + 1, NULL, &set, NULL, &tm) > 0)
	{
#ifdef WIN32
		if (send(sockfd, request.c_str(), request.size(), 0) <= 0)
#else
		if (write(sockfd, request.c_str(), request.size()) <= 0)
#endif
		{
			LogError("Http Request Send Failed");
			response = ERROR_HTTP_SEND;
			closesock(sockfd);
			return false;
		}
	}

	char *pBuf = new char[1024*1024*10];//避免太小，后期调整
	memset(pBuf, 0, 1024 * 1024 * 10);
	bool rc = 0;
	int offset = 0;

	FD_ZERO(&set);
	FD_SET(sockfd, &set);
	if (select(sockfd + 1, &set, NULL, NULL, &tm) > 0)
	{
		rc = RecvAll(sockfd, pBuf, 1024 * 1024 * 10);		if (false == rc)
		{
			LogInfo("Http Request Recv Failed");
			closesock(sockfd);
			delete pBuf;
			return false;
		}
		
		response = pBuf;
		closesock(sockfd);
		delete pBuf;
		return true;

	}
	else
	{
		LogError("socketHttpRequest recv select timeout");
		closesock(sockfd);
		delete pBuf;
		return false;
	}
}

bool HttpSimpleConnect::postData(std::string host, int port, std::string path, std::string post_content, std::string &response)
{
	//POST请求方式
	std::stringstream stream;
	stream << "POST " << path;
	stream << " HTTP/1.1\r\n";
	stream << "Host: " << host << ":" << port << "\r\n";
	stream << "User-Agent: HttpVideoServer V1.0\r\n";
	stream << "Content-Type:application/x-www-form-urlencoded\r\n";
	stream << "Content-Length:" << post_content.length() << "\r\n";
	stream << "Connection:close\r\n\r\n";
	stream << post_content.c_str();

	return socketHttpRequest(host, port, stream.str(), response);
}

bool HttpSimpleConnect::getData(std::string host, int port, std::string path, std::string get_content, std::string &response)
{
	//GET请求方式
	std::stringstream stream("");
	stream << "GET " << path << "?" << get_content;
	stream << " HTTP/1.1\r\n";
	stream << "Host: " << host << ":" << port << "\r\n";
	stream << "User-Agent: HttpVideoServer V1.0\r\n";
	stream << "Connection:close\r\n\r\n";

	return socketHttpRequest(host, port, stream.str(), response);
}


bool HttpSimpleConnect::parse_body(std::string in, std::string &returncode, std::string &body, int &bodylen)
{
	//解析返回返回码
	char code[5] = {};
	memset(code, 0, sizeof(code));
	strncpy(code, in.c_str() + 9, 3);
	returncode = code;

	//解析
	char *ptmp = NULL;
	/*从头域找到内容长度,如果没有找到则不处理*/
	ptmp = (char*)strstr(in.c_str(), "Content-Length:");
	if (ptmp != NULL)
	{
		char *ptmp2;
		ptmp += 15;
		ptmp2 = (char*)strstr(ptmp, "\r\n");
		if (ptmp2 != NULL)
		{
			char cLen[10] = {};
			strncpy(cLen, ptmp, ptmp2 - ptmp);
			bodylen = atoi(cLen);
		}

		//解析body
		ptmp = (char*)strstr(in.c_str(), "\r\n\r\n");
		if (ptmp == NULL)
		{
			return false;
		}
		ptmp += 4;/*跳过\r\n*/

		body = std::string(ptmp).substr(0, bodylen);

	}
	return true;
}


bool HttpSimpleConnect::SendAll(int &sock, char*buffer, int size)
{
	while (size > 0)
	{
		int SendSize = send(sock, buffer, size, 0);
		if (-1 == SendSize)
			return false;
		size = size - SendSize;//用于循环发送且退出功能
		buffer += SendSize;//用于计算已发buffer的偏移量
	}
	return true;
}

bool HttpSimpleConnect::RecvAll(int &sock, char*buffer, int size)
{
	int RecvSize = 0, offset = 0;
	do
	{
#ifdef WIN32
		RecvSize = recv(sock, buffer, size, 0);
#else
		RecvSize = read(sock, buffer, size);
#endif
		if (-1 == RecvSize) 
		{
			
#ifdef WIN32
			int err = GetLastError();
			LogError("RecvSize = %d, errno = %d, HttpClient recv message error!", RecvSize, err);
			if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR)
#else
			LogError("RecvSize = %d, errno = %d, HttpClient recv message error!", RecvSize, errno);
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
#endif
			{
				LogError("RecvSize = EAGAIN(11), No data to read!");
				continue;//errno = EAGAIN的话，重试一下read
			}
			break;
		}
		else if(0 == RecvSize) 
		{
#ifdef WIN32
			int err = GetLastError();
			if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR)
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
#endif
			{
				LogError("RecvSize = EAGAIN(11), No data to read!");
				continue;//errno = EAGAIN的话，重试一下read
			}
			break;//server is closed
		}
		else if(RecvSize > 0)
		{
			size = size - RecvSize;
			buffer += RecvSize;
			offset += RecvSize;
		}
				
	} while (RecvSize > 0);
	return true;
}


void HttpSimpleConnect::closesock(int &sockfd)
{
#ifdef WIN32
	closesocket(sockfd);
#else
	close(sockfd);
#endif
}