#include <stdio.h>
#include <WinSock2.h> // ����ͨ����Ҫ������ͷ�ļ�
#pragma comment(lib,"WS2_32.lib") // ��Ҫ���ص�Windows���ļ�


// ����
void error_die(const char* str) {
	perror(str);
	exit(1);
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

int main(void) {

	unsigned short port = 120;
	int server_socket = startup(&port);
	printf("Myhttpserver is listening %d port.\n", port);
	system("pause");
	return 0;

}