#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h_http_client.h"
#define BUFFER_SIZE 1024
/**********************************************************************
Description:
    根据IP地址创建套接字。
param:
    host：服务器IP地址
    port：服务器端口号
    pclient:返回参数，里面包含根据IP地址创建的套接字
return：
    success:0
    fail:负数
***********************************************************************/
int http_client_create(http_client *pclient,const char *host, int port)
{
	struct hostent *he;
	if(pclient == NULL)
	{
	    return -1;
	}
	memset(pclient,0,sizeof(http_client));

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif  /*  _WIN32  */

	if((he = gethostbyname(host))==NULL)
	{
#if _WIN32
		int err = WSAGetLastError();
		printf("%d\n",err);
#else
		//int err = getlasterror();
#endif
		return -2;
    }
	
    pclient->remote_port = port;
    strcpy(pclient->remote_ip,inet_ntoa( *((struct in_addr *)he->h_addr)));
    pclient->_addr.sin_family = AF_INET;
    pclient->_addr.sin_port = htons(pclient->remote_port);
    pclient->_addr.sin_addr = *((struct in_addr *)he->h_addr);
    if((pclient->socket = socket(AF_INET,SOCK_STREAM,0))==-1)
    {
    	return -3;
    }

    return 0;
}
/*********************************************
Description:
    地址和套接字连接服务器。
param：
    pclient:包含IP地址和套接字。
return：
*********************************************/
int http_client_conn(http_client *pclient)
{
	printf("connecting to %s:%d ... \n", inet_ntoa(pclient->_addr.sin_addr), ntohs(pclient->_addr.sin_port));
	if(pclient->connected)
		return 1;
	if(connect(pclient->socket, (struct sockaddr *)&pclient->_addr,sizeof(struct sockaddr))==-1)
	{
		
		#ifdef _WIN32
        WSACleanup();
        #endif
		return -1;
    }
    pclient->connected = 1;
    return 0;
}
/*********************************************************
Description:
    接收数据函数，
param:
    pclient:
    lpbuff:接收到的数据。
return：
    接收到的数据长度
*********************************************************/
int http_client_recv(http_client *pclient,char **lpbuff,int size)
{
	int recvnum = 0,tmpres = 0;
	char buff[BUFFER_SIZE];
	*lpbuff = NULL;
	while(recvnum <= 0)
	{
		tmpres = recv(pclient->socket, buff, BUFFER_SIZE,0);
		if(tmpres <= 0)
		{
		    break
		}
        recvnum += tmpres;
        if(*lpbuff == NULL)
        {
        	*lpbuff = (char*)malloc(recvnum+1);
			if (*lpbuff == NULL)
			{
				return -2;
			}
			memset(*lpbuff, 0, recvnum + 1);
        }
        else
        {
        	*lpbuff = (char*)realloc(*lpbuff,recvnum);
        	if(*lpbuff == NULL)
        		return -2;
        }
        memcpy(*lpbuff + recvnum - tmpres,buff,tmpres);
    }
	return recvnum;
}

int http_client_send(http_client *pclient,char *buff,int size)
{
	int sent=0,tmpres=0;
	while(sent < size)
	{
        tmpres = send(pclient->socket,buff+sent,size-sent,0);
        if(tmpres == -1)
        {
        	return -1;
        }
        sent += tmpres;
    }
	return sent;
}

int http_client_close(http_client *pclient)
{
#ifdef _WIN32
	closesocket(pclient->socket);
#else
	close(pclient->socket);
#endif
	pclient->connected = 0;
    return 0;
}

int http_post(http_client *pclient,char *page,char *request,char **response)
{
	char post[1500] = {0};
	char host[100] = { 0 };
	char content_len[100] = {0};
	char *lpbuf = NULL;
	char *ptmp = NULL;
	int len=0;
    lpbuf = NULL;
    const char *header2 = "User-Agent: linux_http_client Http 1.1\r\nCache-Control: no-cache\r\nContent-Type: application/x-www-form-urlencoded\r\nAccept: */*\r\n";
    snprintf(post,sizeof(post)-1,"GET %s HTTP/1.1\r\n",page);
    snprintf(host,sizeof(host)-1,"HOST: %s:%d\r\n",pclient->remote_ip,pclient->remote_port);
    snprintf(content_len,sizeof(content_len)-1,"Content-Length: %d\r\n\r\n",strlen(request));
    len = strlen(post)+strlen(host)+strlen(header2)+strlen(content_len)+strlen(request);
    lpbuf = (char*)malloc(len+1);
	memset(lpbuf, 0, len+1);
    if(lpbuf==NULL)
    {
    	#ifdef _WIN32
    	WSACleanup();
    	#endif
    	return -1;
    }
    strcpy(lpbuf,post);
    strcat(lpbuf,host);
    strcat(lpbuf,header2);
    strcat(lpbuf,content_len);
    strcat(lpbuf,request);
    if(!pclient->connected)
    {
        int ret = http_client_conn(pclient);
		if (ret != -1) 
		{
			
			printf("connect to %s:%d success!\n", pclient->remote_ip, pclient->remote_port);
		}
		else 
		{
#ifdef _WIN32
			printf("connect to %s:%d failed  error=%d\n", pclient->remote_ip, pclient->remote_port, GetLastError());
#else
			printf("connect to %s:%d failed  errno=%d\n", pclient->remote_ip, pclient->remote_port, errno);
#endif
			if (lpbuf)
			{
				free(lpbuf);
			}
			return -1;
		}

    }
	//printf("http_client_send  begin err = %d\n", errno);
    if(http_client_send(pclient,lpbuf,len)<0)
    {
#ifdef _WIN32
		printf("http_client_send  send to %s:%d failed error=%d\n", pclient->remote_ip, pclient->remote_port,GetLastError());
#else
		printf("http_client_send  send to %s:%d failed errno=%d\n", pclient->remote_ip, pclient->remote_port,errno);
#endif
		if (lpbuf)
		{
			free(lpbuf);
		}
    	return -1;
    }
	//printf("http_client_send  end err = %d\n", errno);
    //printf("发送请求:\n%s\n",lpbuf);
    /*释放内存*/
    /*it's time to recv from server*/
	memset(lpbuf,0,len);
	//printf("http_client_recv  begin err = %d\n", errno);
	free(lpbuf);
	lpbuf = NULL;
    if(http_client_recv(pclient,&lpbuf,0) <= 0)
    {
    	if(lpbuf) free(lpbuf);
    	return -2;
    }

	memset(post,0,sizeof(post));
    strncpy(post,lpbuf+9,3);
	char szLog[MAX_LOG_LEN] = {0};
	snprintf(szLog, MAX_LOG_LEN-1, "返回码: %d",atoi(post));
	CLogInfo(szLog);
	memset(szLog, 0, sizeof(szLog));
    snprintf(szLog, MAX_LOG_LEN-1, "接收响应:%s", lpbuf);
	CLogInfo(szLog);
    /*响应代码,|HTTP/1.0 200 OK|
     *从第10个字符开始,第3位
     * */
    ptmp = (char*)strstr(lpbuf,"\r\n\r\n");
    if(ptmp == NULL)
    {
        free(lpbuf);
        return -3;
    }
    ptmp += 4;/*跳过\r\n*/
    len = strlen(ptmp)+1;
    *response=(char*)malloc(len);
    if(*response == NULL)
    {
    	if(lpbuf) free(lpbuf);
    	return -1;
    }
    memset(*response,0,len);
    memcpy(*response,ptmp,len-1);
    /*从头域找到内容长度,如果没有找到则不处理*/
    ptmp = (char*)strstr(lpbuf,"Content-Length:");
    if(ptmp != NULL)
    {
    	char *ptmp2;
        ptmp += 15;
        ptmp2 = (char*)strstr(ptmp,"\r\n");
        if(ptmp2 != NULL)
        {
            memset(post,0,sizeof(post));
            strncpy(post,ptmp,ptmp2-ptmp);
            if(atoi(post)<len)
                (*response)[atoi(post)] = '\0';
        }
    }
    if(lpbuf) free(lpbuf);
    return 0;
}

int http_get(http_client *pclient,char *page,char **response)
{
	char post[300],host[100],header2[1024];
	char *lpbuf,*ptmp;
	int len=0;
    lpbuf = NULL;
	memset(header2,0,1024);
	sprintf(header2,"Connection: keep-alive\r\nAccept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\nUser-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/534.3 (KHTML, like Gecko) Chrome/6.0.472.33 Safari/534.3 SE 2.X MetaSr 1.0\r\nAccept-Encoding: gzip,deflate\r\nAccept-Language: zh-CN,zh;q=0.8\r\nAccept-Charset: GBK,utf-8;q=0.7,*;q=0.3\r\nCookie: prov=86; city=other; weather_city=bj; userid=1306844325031_5716; ilocid=1000; ilocidflag=1; vjuids=60f761c9e.13045fde8be.0.427b2991; iCast2_15963_2105hO=0_2_20160; _iCast2_15963_2105hO=2; Q37_sid=U4Bb45; ExpendBuoywwwpacx2011619=1; vjlast=1306844326.1308468360.11; Q37_visitedfid=442D284\r\nIf-Modified-Since: Sun, 19 Jun 2011 11:27:13 GMT\r\n\r\n");
    sprintf(post,"GET %s HTTP/1.1\r\n",page);
    sprintf(host,"Host: %s:%d\r\n",pclient->remote_ip,pclient->remote_port);
    len = strlen(post)+strlen(host)+strlen(header2);
    lpbuf = (char*)malloc(len);
    if(lpbuf==NULL)
    {
    	#ifdef _WIN32
    	WSACleanup();
    	#endif
    	return -1;
    }
    strcpy(lpbuf,post);
    strcat(lpbuf,host);
    strcat(lpbuf,header2);
    if(!pclient->connected)
    {
        http_client_conn(pclient);
    }
    if(http_client_send(pclient,lpbuf,len)<0)
    {
    	return -1;
    }
    //printf("发送请求:\n%s\n",lpbuf);
    /*释放内存*/
/*it's time to recv from server*/
	memset(lpbuf,0,len);
	free(lpbuf);
	lpbuf = NULL;
    if(http_client_recv(pclient,&lpbuf,0) <= 0)
    {
    	if(lpbuf) free(lpbuf);
    	return -2;
    }
	memset(post,0,sizeof(post));
    strncpy(post,lpbuf+9,3);
	//printf("返回码: %d\n",atoi(post));
    //printf("接收响应:\n%s\n",lpbuf);
    /*响应代码,|HTTP/1.0 200 OK|
     *从第10个字符开始,第3位
     * */
    ptmp = (char*)strstr(lpbuf,"\r\n\r\n");
    if(ptmp == NULL)
    {
        free(lpbuf);
        return -3;
    }
    ptmp += 4;/*跳过\r\n*/
    len = strlen(ptmp)+1;
    *response=(char*)malloc(len);
    if(*response == NULL)
    {
    	if(lpbuf) free(lpbuf);
    	return -1;
    }
    memset(*response,0,len);
    memcpy(*response,ptmp,len-1);
/*从头域找到内容长度,如果没有找到则不处理*/
    ptmp = (char*)strstr(lpbuf,"Content-Length:");
    if(ptmp != NULL)
    {
    	char *ptmp2;
        ptmp += 15;
        ptmp2 = (char*)strstr(ptmp,"\r\n");
        if(ptmp2 != NULL)
        {
            memset(post,0,sizeof(post));
            strncpy(post,ptmp,ptmp2-ptmp);
            if(atoi(post)<len)
                (*response)[atoi(post)] = '\0';
        }
    }
    if(lpbuf) free(lpbuf);
    return 0;
}

#if 0
//demo
 int main()
 {
 	http_client client;
 	char *response = NULL;
 	char url[256];
 	printf("Input the URL:");
 	scanf("%s",url);//这里的URL应该就是输入IP地址，
 	printf("开始组包\n");
 	http_client_create(&client,"192.168.1.246",2011);
	
 	if(http_post(&client,"/"," ",&response))//
     {
 		printf("失败!\n");
 		exit(2);
     }
	
	//if(http_get(&client,"/",&response))
     //{
 		//printf("失败!\n");
 		//exit(2);
     //}

 	printf("响应:\n%d:%s\n",strlen(response),response);
 	FILE *fp;
 	fp = fopen("index.html","w");
 	fwrite(response,strlen(response),1,fp);
 	fclose(fp);
 	free(response);
 #ifdef _WIN32
     WSACleanup();
 #endif
     return 0;
 }
 #endif
