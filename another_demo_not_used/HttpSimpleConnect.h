#pragma once
#include <string>
#ifdef WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include <string.h>//mem

#include <sys/types.h>//sock
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>  
#include <fcntl.h> 
#endif
#define SOCKET_TIMEOUT 3 //5s

class HttpSimpleConnect
{
public:
	HttpSimpleConnect();
	virtual ~HttpSimpleConnect();

public:
	bool postData(std::string host, int port, std::string path, std::string post_content, std::string &response);
	bool getData(std::string host, int port, std::string path, std::string get_content, std::string &response);

	bool parse_body(std::string in, std::string &returncode, std::string &body, int &bodylen);
private:
	bool socketHttpRequest(std::string host, int port, std::string request, std::string &response);
	bool SendAll(int &sock, char*buffer, int size);
	bool RecvAll(int &sock, char*buffer, int size);
	void closesock(int &sockfd);
};

