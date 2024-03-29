// sign_verify_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<openssl/evp.h>
#include<openssl/err.h>
#include<openssl/conf.h>
#include<openssl/pem.h>
#include<openssl/rsa.h>

/*缓冲池*/
#define BUFSIZE 1024

EVP_PKEY* import_prikey(char *keylist) {
	EVP_PKEY *key;
	key = EVP_PKEY_new();
	RSA *rsa;
	rsa = RSA_new();
	FILE *inf;
	char pwd[20];
	inf = fopen(keylist
, "rb");
	if (!inf) {
		fprintf(stderr, "ERROR: read prikey file error: %s\n", strerror(errno));
		exit(errno);
	}
	printf("Input password of privatekey %s :\n", keylist);
	gets_s(pwd, 20);
	rsa = PEM_read_RSAPrivateKey(inf, NULL, NULL, pwd);
	EVP_PKEY_set1_RSA(key, rsa);
	fclose(inf);
	return key;
}//从私钥文件导出私钥

EVP_PKEY* import_pubkey(char *keylist) {
	EVP_PKEY *key;
	key = EVP_PKEY_new();
	RSA *rsa;
	rsa = RSA_new();
	FILE *inf;
	inf = fopen(keylist, "rb");
	if (!inf) {
		fprintf(stderr, "ERROR: read pubkey file error: %s\n", strerror(errno));
		exit(errno);
	}
	rsa = PEM_read_RSAPublicKey(inf, NULL, NULL, NULL);
	EVP_PKEY_set1_RSA(key, rsa);
	fclose(inf);
	return key;
}//从公钥文件导入公钥

int sign(FILE *inf, FILE *otf, EVP_PKEY *key) {
	unsigned char in_buf[BUFSIZE];
	int read_len;
	EVP_MD_CTX *mdctx = NULL;
	int ret = 0;
	size_t slen;

	unsigned char *sig;

	/* Create the Message Digest Context */
	if (!(mdctx = EVP_MD_CTX_create())) {
		fprintf(stderr, "ERROR: EVP_MD_CTX_create failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_destroy(mdctx);
		return 1;
	}

	/* Initialise the DigestSign operation - SHA-256 has been selected as the message digest function in this example */
	if (1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, key)) {
		fprintf(stderr, "ERROR: EVP_DigestSignInit failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_destroy(mdctx);
		return 1;
	}//初始化，选择sha256消息摘要算法

	while (1) {

		read_len = fread(in_buf, sizeof(unsigned char), BUFSIZE, inf);

		if (1 != EVP_DigestSignUpdate(mdctx, in_buf, read_len)) {
			fprintf(stderr, "ERROR: EVP_DigestSignUpdate failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
			EVP_MD_CTX_destroy(mdctx);
			return 1;
		}

		if (read_len < BUFSIZE)
			break;
	}//与消息摘要相似，循环读取文件计算摘要

	/* Finalise the DigestSign operation */
	/* First call EVP_DigestSignFinal with a NULL sig parameter to obtain the length of the
	 * signature. Length is returned in slen */
	if (1 != EVP_DigestSignFinal(mdctx, NULL, &slen)) {
		fprintf(stderr, "ERROR: EVP_DigestSignFinal 1 failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_destroy(mdctx);
		return 1;
	}//最后计算出签名信息
	/* Allocate memory for the signature based on size in slen */
	if (!(sig = (unsigned char*)malloc(sizeof(unsigned char) * slen))) goto err;
	/* Obtain the signature */
	if (1 != EVP_DigestSignFinal(mdctx, sig, &slen)) {
		fprintf(stderr, "ERROR: EVP_DigestSignFinal 2 failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_destroy(mdctx);
		return 1;
	}

	/* Success */
	ret = 1;
	printf("\nSigning Seccess.\n");

	fwrite(sig, sizeof(unsigned char), slen, otf);//签名写入文件

err:
	if (ret != 1)
	{
		/* Do some error handling */
		printf("OPENSSL_malloc Error!\n");
	}

	/* Clean up */
	if (sig && !ret) OPENSSL_free(sig);
	if (mdctx) EVP_MD_CTX_destroy(mdctx);
	return 0;
}

int verify(FILE *sign, FILE *inf, EVP_PKEY *key) {

	unsigned char in_buf[BUFSIZE];
	int read_len;
	EVP_MD_CTX *mdctx = NULL;
	int ret = 0;
	size_t slen;

	unsigned char sig[BUFSIZE];
	slen = fread(sig, sizeof(unsigned char), BUFSIZE, sign);//读存放取签名信息的文件

	if (!(mdctx = EVP_MD_CTX_new())) {
		fprintf(stderr, "ERROR: EVP_MD_CTX_new failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_destroy(mdctx);
		return 1;
	}

	/* Initialize `key` with a public key */
	if (1 != EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, key)) {
		fprintf(stderr, "ERROR: EVP_DigestVerifyInit failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_destroy(mdctx);
		return 1;
	}

	while (1) {
		read_len = fread(in_buf, sizeof(unsigned char), BUFSIZE, inf);
		/* Initialize `key` with a public key */
		if (1 != EVP_DigestVerifyUpdate(mdctx, in_buf, read_len)) {
			fprintf(stderr, "ERROR: EVP_DigestVerifyUpdate failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
			EVP_MD_CTX_destroy(mdctx);
			return 1;
		}
		if (read_len < BUFSIZE) {
			break;
		}
	}//对原文件计算摘要值

	if (1 == EVP_DigestVerifyFinal(mdctx, sig, slen))
	{
		printf("Verify success.\n");
	}
	else {
		printf("Verify failure.\n");
	}//私钥加密的签名进行公钥解密，和刚计算的消息摘要比较，正确返回认证成功
	EVP_MD_CTX_destroy(mdctx);
	return 0;
}

int main(int argc, char *argv[])
{
	EVP_PKEY *prikey, *pubkey;
	prikey = EVP_PKEY_new();
	pubkey = EVP_PKEY_new();
	FILE *inf, *otf, *prifile, *pubfile;
	if (!strcmp(argv[1], "-h")) {
		printf("--help:\n\n");
		printf("[-s] [infile] [prikeyfile] [outfile]\n");
		printf("infile：要生成签名的文件  prikeyfile：生成签名使用的私钥文件  outfile：签名信息保存的文件\n");
		printf("[-v] [infile] [pubkeyfile] [signfile]\n");
		printf("infile：要验证签名的文件  prikeyfile：验证签名使用的公钥文件  outfile：签名信息保存的文件\n");
		return 0;
	 }

	if (!strcmp(argv[1], "-s")) {
		if (argc < 4) {
			printf("Command Error : Wrong Command.\n");
			return 1;
		}
		else {
			inf = fopen(argv[2], "rb");
			if (!inf) {
				fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
				return errno;
			}
			prikey = import_prikey(argv[3]);
			otf = fopen(argv[4], "wb");
			if (!otf) {
				fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
				return errno;
			}
			sign(inf, otf, prikey);
			fclose(otf);
			fclose(inf);
		}
	}

	if (!strcmp(argv[1], "-v")) {
		inf = fopen(argv[2], "rb");
		if (!inf) {
			fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
			return errno;
		}
		pubkey = import_pubkey(argv[3]);
		otf = fopen(argv[4], "rb");
		if (!otf) {
			fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
			return errno;
		}
		verify(otf, inf, pubkey);
	}
}

