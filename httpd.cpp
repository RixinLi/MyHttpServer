#include <stdio.h>
#include <WinSock2.h> // ����ͨ����Ҫ������ͷ�ļ�
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#pragma comment(lib,"WS2_32.lib") // ��Ҫ���ص�Windows���ļ�

#define PRINT(str) printf("[%s - %d]"#str"=%s\n", __func__, __LINE__, str)

// ����
void error_die(const char* str) {
	perror(str);
	exit(1);
}

// ����·����׺�����ļ� 0��rb, 1 ��r 
int file_type(const char* path, int len) {
	int i = len - 1;
	char type[255];
	int j = 0;

	for (; i >= 0 && path[i]!='.' && path[i]!='/'; i--) {
		type[j++] = path[i];
	}
	type[j] = 0;

	if (path[i] == '/') return 1;

	// ͼƬ�����ܺ�
	if (!strcmp(type, "gpj") || !strcmp(type, "gnp")) return 0;

	return 1;
}

// ʵ������ĳ�ʼ��
// return �׽��֣��������˵��׽��֣�
//	�˿� port
// ������port ��ʾ�˿�
// ���*port��ֵ��0����ô���Զ�������ö˿�
// Ϊ����tinyhttpd�¾�
int startup(unsigned short* port) {
	// 1. ����ͨ�Եĳ�ʼ���� Windows��Ҫ��linux����Ҫ
	WSADATA data;
	int ret = WSAStartup(
		MAKEWORD(1,1), // 1.1 �汾
		&data
	);
	
	// �����ʼ��ʧ��
	if (ret) { // if ret != 0
		error_die("Network initialization failed!\n");
		exit(1);
	}

	int server_socket = socket(
		PF_INET, // �׽��ֵ�����
		SOCK_STREAM,//������
		IPPROTO_TCP // TCPЭ��
	);

	if (server_socket == -1) {
		error_die("socket failed!\n");
	}

	// ���ö˿ڿɸ���(�˿ڸ���)
	int opt = 1;
	ret = setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt, sizeof(opt));
	if (ret==-1) {
		error_die("setsockopt\n");
	}

	// ���÷������������ַ
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(*port); // htons: host to nework short
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // ����ȫ���� htonl: host to network long



	// ���׽���
	if ( bind(server_socket, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) 
	{
		error_die("bind\n");
	}


	// ��̬����˿�
	int nameLen = sizeof(server_addr);
	if (*port == 0) {
		if (getsockname(server_socket, (struct sockaddr*)&server_addr, &nameLen) < 0) {
			error_die("getsockname\n");
		}
		*port = server_addr.sin_port; // �����µĶ�̬�˿�
	}

	
	// ������������
	if (listen(server_socket, 5) < 0) {
		error_die("listen\n");
	}
	
	return server_socket;
}


// ��ָ���Ŀͻ����׽��֣���ȡһ�����ݣ����浽buff��
// ����ʵ�ʶ�ȡ�����ֽ�
int get_line(int sock, char* buff, int size) {

	char c = '\0'; // '\0'
	int i = 0;

	// \r\n
	while ( i<size-1 && c != '\n') {

		int n = recv(sock, &c, 1, 0);
		
		if (n > 0) {
			if (c == '\r') {
				n = recv(sock, &c, 1, MSG_PEEK); // MSG_PEEK ��һ��
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
	// ��ָ�����׽��֣����ͷ���ʵ��

}

void Notfound(int client) {
	// ��ָ�����׽��֣����Ͳ�����
	

}

void headers(int client, int method, int type) {
	// ������Ӧ����ͷ��Ϣ

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

	// ����
	send(client, "\r\n", 2, 0);

}

// �ļ���ʵ������
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
	// ׼����ָ�����׽��֣������ļ�
	char buff[1024];
	// ���������ݰ�ʣ�������У�����
	while (get_line(client, buff, sizeof(buff)) > 0 && strcmp(buff, "\n"))PRINT(buff);

	FILE* resource = NULL;
	// ��ʽ������Դ�������

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

	// �����������Դ��Ϣ
	cat(client,resource);

	printf("messages sent success!\n");
	fclose(resource);

}


// �����û�������̺߳���
DWORD WINAPI accept_request(LPVOID arg) {

	SOCKET sock = (SOCKET)arg;
	char buff[1024]; // 1k

	//	��ȡһ������
	int numchars = get_line(sock, buff, sizeof(buff));
	PRINT(buff);

	char method[255];
	int j = 0, i=0;

	// ��ȡ����
	while (!isspace(buff[j]) && i < sizeof(method)-1 && j < sizeof(buff)) {
		method[i++] = buff[j++];
	}
	method[i] = 0;
	PRINT(method);

	// �������ķ������������Ƿ�֧��
	if (stricmp(method, "GET") && stricmp(method, "POST")) {
		// �����������һ����������ҳ��
		// To do 
		unimplement(sock);
		return 0;
	}


	// ������Դ��·��
	// www.baidu.com/abc/test.html
	char url[255]; // �����Դ��������·��
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

	// �ļ�Ȩ��
	struct stat st;
	if (stat(path, &st) == -1) { // ûȨ������ʧ��
		while (get_line(sock, buff, sizeof(buff))>0 && strcmp(buff, "\n"));
		Notfound(sock);
	}
	else {
		if ((st.st_mode & S_IFMT) == S_IFDIR) { // ������ļ����ټ�һ��index.html
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

		// ����ʽ�ȴ��û�ͨ��������������
		int client_sock = accept(server_socket, (sockaddr *)&client_addr, &client_addr_len);

		//���ͻ����׽���
		if (client_sock == -1) {
			error_die("accept\n");
		}

		// �������߳�


		
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request,(LPVOID)client_sock,0,&threadId);


		// "/"��վ��������ԴĿ¼�µ� index.html


	}

	closesocket(server_socket);
	return 0;

}