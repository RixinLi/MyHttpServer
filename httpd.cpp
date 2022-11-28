#include <stdio.h>
#include <WinSock2.h> // 网络通信需要包含的头文件
#pragma comment(lib,"WS2_32.lib") // 需要加载的Windows库文件

#define PRINT(str) printf("[%s - %d]"#str"=%s", __func__, __LINE__, str)

// 报错
void error_die(const char* str) {
	perror(str);
	exit(1);
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


// 处理用户请求的线程函数
DWORD WINAPI accept_request(LPVOID arg) {

	SOCKET sock = (SOCKET)arg;
	char buff[1024]; // 1k

	//	读取一行数据
	int numchars = get_line(sock, buff, sizeof(buff));
	PRINT(buff);
	


	return 0;
}


int main(void) {

	unsigned short port = 8080;
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