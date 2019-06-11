
http_client client；
char IPAddr[32] = "192.168.1.1";
unsigned int iPort = 1000;
char *s = NULL;
char *response = NULL;

http_client_create(&client, ipAddr, iPort);
if(http_post(&client, "/fres/face_recognize", s, &response))
 {
	printf("http_post error,\n");
	ret = -6;
	http_client_close(&client);		
 }
 else
 {
	 http_client_close(&client);
 }
 
/*
1：在by里面，s为请求内容，其中首先要把图片用opencv里面的imencode转成内存中的jpg格式，
然后再转成base64格式，然后再和其他的一些东西组成一个json字符串，里面是一系列的k value,
像"baselib_name":"xxx","image":"base64格式的图片数据"，“min_sim”:"0.8"
2.每次post之后要close掉，要不然每次都重新创建了一个新的socket，这样用不了几次节点就不够用了。
3.再his的嵌入式设备里面，因为网卡可能用eth0,eth1，还有4G网卡，所以在这个demo的http_client_create
函数的最后面，创建完socket之后需要添加下面的代码把socket和网卡进行绑定。
#if 1	
	//bind the socket to routeName
	strncpy(struIR.ifr_name, boyun_netName, 10);
	if(setsockopt(pclient->socket, SOL_SOCKET, SO_BINDTODEVICE, (unsigned char *)&struIR, sizeof(struIR)) < 0 )
    {
	    printf("socket[%d] Binding Route[%s] failed===%s===\n", pclient->socket, boyun_netName, strerror(errno));
		close(pclient->socket);
		return -4;
	}
	else
	{
	    printf("socket %d Bind Route %s successfully,\n", pclient->socket, boyun_netName);
	}
#endif

*/