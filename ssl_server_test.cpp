/*****************************************************************
*SSL/TLS服务端程序WIN32版(以demos/server.cpp为基础)
*需要用到动态连接库libeay32.dll,ssleay.dll,
*同时在setting中加入ws2_32.lib libeay32.lib ssleay32.lib,
*以上库文件在编译openssl后可在out32dll目录下找到,
*所需证书文件请参照文章自行生成.
*****************************************************************/

#include <stdio.h>  
#include <stdlib.h>  
#include <memory.h>  
#include <errno.h>  
#include <sys/types.h>  
#include <openssl/rsa.h>
#include <openssl/crypto.h>  
#include "openssl/x509.h"  
#include "openssl/pem.h"  
#include "openssl/ssl.h"  
#include "openssl/err.h"  

#include <winsock2.h>  
#pragma comment(lib, "Ws2_32.lib")



/*所有需要的参数信息都在此处以#define的形式提供*/
#define CERTF   "03.pem" /*服务端的证书(需经CA签名)*/  
#define KEYF   "server.pem"  /*服务端的私钥(建议加密存储)*/  
#define CACERT "cacert.pem" /*CA 的证书*/  
#define PORT   11111   /*准备绑定的端口*/  

#define CHK_NULL(x) if ((x)==NULL) exit (1)  
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }  
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }  

int main()
{
	int err;
	int listen_sd;
	int sd;
	struct sockaddr_in sa_serv;
	struct sockaddr_in sa_cli;
	int client_len;
	SSL_CTX* ctx;
	SSL*     ssl;
	X509*    client_cert;
	char*    str;
	str = (char*)malloc(100 * sizeof(char));
	char     buf[4096];
	const SSL_METHOD *meth;
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup()fail:%d/n", GetLastError());
		return -1;
	}

	SSL_load_error_strings();            /*为打印调试信息作准备*/
	OpenSSL_add_ssl_algorithms();        /*初始化*/
	meth = SSLv23_server_method();  /*采用什么协议(SSLv2/SSLv3/TLSv1)在此指定*/

	ctx = SSL_CTX_new(meth);
	CHK_NULL(ctx);

	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);   

	if (SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(3);
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(4);
	}

	if (!SSL_CTX_check_private_key(ctx)) {
		printf("Private key does not match the certificate public key\n");
		exit(5);
	}

	SSL_CTX_set_cipher_list(ctx, "RC4-MD5");

	/*开始正常的TCP socket过程.................................*/
	printf("Begin TCP socket...\n");

	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	CHK_ERR(listen_sd, "socket");

	memset(&sa_serv, '\0', sizeof(sa_serv));
	sa_serv.sin_family = AF_INET;
	sa_serv.sin_addr.s_addr = INADDR_ANY;
	sa_serv.sin_port = htons(PORT);

	err = bind(listen_sd, (struct sockaddr*) &sa_serv,

		sizeof(sa_serv));

	CHK_ERR(err, "bind");

	/*接受TCP链接*/
	err = listen(listen_sd, 5);
	CHK_ERR(err, "listen");

	client_len = sizeof(sa_cli);
	sd = accept(listen_sd, (struct sockaddr*) &sa_cli, &client_len);
	CHK_ERR(sd, "accept");
	closesocket(listen_sd);

	printf("Connection from %lx, port %x\n",
		sa_cli.sin_addr.s_addr, sa_cli.sin_port);

	/*TCP连接已建立,进行服务端的SSL过程. */
	printf("Begin server side SSL\n");

	ssl = SSL_new(ctx);
	CHK_NULL(ssl);
	SSL_set_fd(ssl, sd);
	err = SSL_accept(ssl);
	CHK_SSL(err);
	printf("SSL_accept finished\n");
	
	/* 数据交换开始,用SSL_write,SSL_read代替write,read */
	while (1) {
		err = SSL_read(ssl, buf, sizeof(buf) - 1);
		CHK_SSL(err);
		buf[err] = '\0';
		printf("Client >> ");
		printf("Got %d chars: %s\n", err, buf);
		if (!strncmp(buf, "end", 3)) {
			break;
		}
		printf("Server >> ");
		for (err = 0; err < 4095; err++) {
			buf[err] = getchar();
			buf[err + 1] = '\0';
			if (buf[err] == '\n') {
				buf[err] == '\0';
				break;
			}
		}
		err = SSL_write(ssl, buf, err);
		CHK_SSL(err);
	}

	/* 收尾工作*/
	shutdown(sd, 2);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	return 0;
}