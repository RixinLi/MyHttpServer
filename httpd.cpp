#include <stdio.h>
#include <WinSock2.h> // 网络通信需要包含的头文件
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#pragma comment(lib,"WS2_32.lib") // 需要加载的Windows库文件

#define PRINT(str) printf("[%s - %d]"#str"=%s\n", __func__, __LINE__, str)

// 报错
void error_die(const char* str) {
	perror(str);
	exit(1);
}

// 根据路径后缀名打开文件 0用rb, 1 用r 
int file_type(const char* path, int len) {
	int i = len - 1;
	char type[255];
	int j = 0;

	for (; i >= 0 && path[i]!='.' && path[i]!='/'; i--) {
		type[j++] = path[i];
	}
	type[j] = 0;

	if (path[i] == '/') return 1;

	// 图片类型总和
	if (!strcmp(type, "gpj") || !strcmp(type, "gnp")) return 0;

	return 1;
}

// 实现网络的初始化
// return 套接字（服务器端的套接字）
//	端口 port
// 参数：port 表示端口
// 如果*port的值是0，那么就自动分配可用端口
// 为了向tinyhttpd致敬
int startup(unsigned short* port) {
	// 1. 网络通性的初始化： Windows需要，linux不需要
	WSADATA data;
	int ret = WSAStartup(
		MAKEWORD(1,1), // 1.1 版本
		&data
	);
	
	// 如果初始化失败
	if (ret) { // if ret != 0
		error_die("Network initialization failed!\n");
		exit(1);
	}

	int server_socket = socket(
		PF_INET, // 套接字的类型
		SOCK_STREAM,//数据流
		IPPROTO_TCP // TCP协议
	);

	if (server_socket == -1) {
		error_die("socket failed!\n");
	}

	// 设置端口可复用(端口复用)
	int opt = 1;
	ret = setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt, sizeof(opt));
	if (ret==-1) {
		error_die("setsockopt\n");
	}

	// 配置服务器的网络地址
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(*port); // htons: host to nework short
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 允许全访问 htonl: host to network long



	// 绑定套接字
	if ( bind(server_socket, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) 
	{
		error_die("bind\n");
	}


	// 动态分配端口
	int nameLen = sizeof(server_addr);
	if (*port == 0) {
		if (getsockname(server_socket, (struct sockaddr*)&server_addr, &nameLen) < 0) {
			error_die("getsockname\n");
		}
		*port = server_addr.sin_port; // 更新新的动态端口
	}

	
	// 创建监听队列
	if (listen(server_socket, 5) < 0) {
		error_die("listen\n");
	}
	
	return server_socket;
}


// 从指定的客户端套接字，读取一行数据，保存到buff中
// 返回实际读取到的字节
int get_line(int sock, char* buff, int size) {

	char c = '\0'; // '\0'
	int i = 0;

	// \r\n
	while ( i<size-1 && c != '\n') {

		int n = recv(sock, &c, 1, 0);
		
		if (n > 0) {
			if (c == '\r') {
				n = recv(sock, &c, 1, MSG_PEEK); // MSG_PEEK 瞄一眼
				if (n > 0 && c == '\n') {
					recv(sock, &c, 1, 0);
				}
				else {
					c = '\n';
				}
			}
			buff[i++] = c;
		}
		else {
			c = '\n';
		}
	}

	buff[i] = '\0';
	return i;
}

void unimplement(int client) {
	// 向指定的套接字，发送服务不实现

}

void Notfound(int client) {
	// 向指定的套接字，发送不存在
	

}

void headers(int client, int method, int type) {
	// 发送响应包的头信息

	char buff[1024];

	switch (method)
	{
	case 1:
		strcpy(buff, "HTTP/1.0 200 OK\r\n");
		break;
	default:
		buff[0] = 0;
		break;
	}
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Server: RixinHttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	switch (type)
	{
	case 1:
		strcpy(buff, "Content-type:text/html\n");
		break;
	case 2:
		strcpy(buff, "Content-type:image/jpg\n");
		break;
	default:
		break;
	}
	send(client, buff, strlen(buff), 0);

	// 结束
	send(client, "\r\n", 2, 0);

}

// 文件的实际内容
void cat(int client, FILE* resource) {
	// To do

	char buff[4096];

	int count = 0;

	while (1) {

		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0) {
			break;
		}
		send(client, buff, ret, 0);
		count += ret;
	}

	printf("total =%d\n", count);

}

void server_file(int client, const char* filename) {
	// 准备向指定的套接字，发送文件
	char buff[1024];
	// 把请求数据包剩余数据行，读完
	while (get_line(client, buff, sizeof(buff)) > 0 && strcmp(buff, "\n"))PRINT(buff);

	FILE* resource = NULL;
	// 正式发送资源给浏览器

	int type;

	if (file_type(filename, strlen(filename))) {
		resource = fopen(filename, "r");
		type = 1;
	}
	else {
		resource = fopen(filename, "rb");
		type = 2;
	}

	if (resource == NULL) {
		Notfound(client);
	}


	headers(client,1,type);

	// 发送请求的资源信息
	cat(client,resource);

	printf("messages sent success!\n");
	fclose(resource);

}


// 处理用户请求的线程函数
DWORD WINAPI accept_request(LPVOID arg) {

	SOCKET sock = (SOCKET)arg;
	char buff[1024]; // 1k

	//	读取一行数据
	int numchars = get_line(sock, buff, sizeof(buff));
	PRINT(buff);

	char method[255];
	int j = 0, i=0;

	// 获取方法
	while (!isspace(buff[j]) && i < sizeof(method)-1 && j < sizeof(buff)) {
		method[i++] = buff[j++];
	}
	method[i] = 0;
	PRINT(method);

	// 检查请求的方法，本服务是否支持
	if (stricmp(method, "GET") && stricmp(method, "POST")) {
		// 向浏览器返回一个错误提升页面
		// To do 
		unimplement(sock);
		return 0;
	}


	// 解析资源的路径
	// www.baidu.com/abc/test.html
	char url[255]; // 存放资源请求完整路径
	i = 0;
	while (isspace(buff[j]) && j < sizeof(buff)) j++;
	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff)) {
		url[i++] = buff[j++];
	}
	url[i] = 0;
	PRINT(url);

	// www.rock.com
	// 127.0.0.1/test.html
	// url  /test.html
	// htdocs/test.html

	char path[512] = "";
	sprintf(path, "htdocs%s",url);
	if (path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");
	}
	PRINT(path);

	// 文件权限
	struct stat st;
	if (stat(path, &st) == -1) { // 没权限请求失败
		while (get_line(sock, buff, sizeof(buff))>0 && strcmp(buff, "\n"));
		Notfound(sock);
	}
	else {
		if ((st.st_mode & S_IFMT) == S_IFDIR) { // 如果是文件夹再加一个index.html
			strcat(path, "/index.html");
		}
		server_file(sock,path);
	}

	closesocket(sock);
	return 0;
}


int main(void) {

	unsigned short port = 80;
	int server_socket = startup(&port);
	printf("Myhttpserver is listening %d port.\n", port);

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);


	while (1) {

		// 阻塞式等待用户通过浏览器发起访问
		int client_sock = accept(server_socket, (sockaddr *)&client_addr, &client_addr_len);

		//检查客户端套接字
		if (client_sock == -1) {
			error_die("accept\n");
		}

		// 创建多线程


		
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request,(LPVOID)client_sock,0,&threadId);


		// "/"网站服务器资源目录下的 index.html


	}

	closesocket(server_socket);
	return 0;

}