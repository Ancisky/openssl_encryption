// file_digest_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<openssl/evp.h>
#include<openssl/err.h>
#include<openssl/conf.h>

/*摘要长度*/
#define MD5SIZE 16
#define SHA256SIZE 32
#define SHA512SIZE 64
/*缓冲池*/
#define BUFSIZE 1024

struct md {
	unsigned char *md_v;
	unsigned int digest_len;
};

md file_digest(FILE *inf, const EVP_MD *type){
	EVP_MD_CTX *mdctx;
	int read_len;
	unsigned char in_buf[BUFSIZE];
	md md_stc;

	if (type == EVP_sha256()) 
		md_stc.md_v = (unsigned char*)malloc(SHA256SIZE * sizeof(unsigned char));
	else if (type == EVP_sha512()) 
		md_stc.md_v= (unsigned char*)malloc(SHA512SIZE * sizeof(unsigned char));
	else 
		md_stc.md_v= (unsigned char*)malloc(MD5SIZE * sizeof(unsigned char));


	if ((mdctx = EVP_MD_CTX_create()) == NULL) {
		fprintf(stderr, "ERROR: EVP_MD_CTX_create failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_destroy(mdctx);
		fclose(inf);
	}
		

	if (1 != EVP_DigestInit_ex(mdctx, type, NULL)) {
		fprintf(stderr, "ERROR: EVP_DigestInit_ex failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_destroy(mdctx);
		fclose(inf);
	}
		
	while (1) {

		read_len = fread(in_buf, sizeof(unsigned char), BUFSIZE, inf);

		if (1 != EVP_DigestUpdate(mdctx, in_buf, read_len)) {
			fprintf(stderr, "ERROR: EVP_DigestUpdate failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
			EVP_MD_CTX_destroy(mdctx);
			fclose(inf);
		}
		
		if (read_len < BUFSIZE)
			break;
	}

	if (1 != EVP_DigestFinal_ex(mdctx, md_stc.md_v, &(md_stc.digest_len))) {
		fprintf(stderr, "ERROR: EVP_DigestFinal_ex failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_destroy(mdctx);
		fclose(inf);
	}
	
	for (int round = 0; round < md_stc.digest_len; round++) {
		printf("%02x", md_stc.md_v[round]);
	}
	printf("\n");

	EVP_MD_CTX_destroy(mdctx);
	return md_stc;
}

int main(int argc,char *argv[])
{
	FILE *inf, *otf;
	md md_stc;
	const EVP_MD *type = NULL;
	unsigned char flag = 0;
	unsigned char *digest_check;

	if (!strcmp(argv[1], "-h")) {
		printf("help:\n\n\n");
		printf("Command: [Method]/-h [Input File Path] [Output File Path]\n\n");
		printf("sha256                              使用sha256对文件进行消息摘要\n");
		printf("sha512                              使用sha512对文件进行消息摘要\n");
		printf("md5                                 使用md5对文件进行消息摘要\n");
		return 0;
	}

	if (!strcmp(argv[1], "-d")) {
		if (!strcmp(argv[2], "sha256"))
			type = EVP_sha256();
		else if (!strcmp(argv[2], "md5"))
			type = EVP_md5();
		else if (!strcmp(argv[2], "sha512")) {
			type = EVP_sha512();
		}
		else {
			printf("MD Algorithm Error: Wrong Command\n");
			return 0;
		}

		inf = fopen(argv[3], "rb");
		if (!inf) {
			fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
			return errno;
		}

		md_stc = file_digest(inf, type);

		if (argc == 5) {
			otf = fopen(argv[4], "wb");
			if (!otf) {
				fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
				return errno;
			}
			if (type == EVP_sha256()) {
				flag = (unsigned char)1;
			}
			else if (type == EVP_sha512()) {
				flag = (unsigned char)2;
			}
			else if (type == EVP_md5()) {
				flag = (unsigned char)3;
			}
			fwrite(&flag, sizeof(unsigned char), 1, otf);
			fwrite(md_stc.md_v, sizeof(unsigned char), md_stc.digest_len, otf);
			fclose(otf);
		}
		fclose(inf);
	}

	if (!strcmp(argv[1], "-c")) {
		inf = fopen(argv[2], "rb");
		if (!inf) {
			fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
			return errno;
		}
		otf = fopen(argv[3], "rb");
		if (!otf) {
			fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
			return errno;
		}
		fread(&flag, sizeof(unsigned char), 1, otf);
		if (flag == 1) {
			type = EVP_sha256();
		}
		else if (flag == 2) {
			type == EVP_sha512();
		}
		else if (flag == 3) {
			type == EVP_md5();
		}
		else {
			printf("Error: Wrong Digest Method.\n");
			return 0;
		}
		md_stc = file_digest(inf, type);
		digest_check = (unsigned char*)malloc(md_stc.digest_len * sizeof(unsigned char));
		fread(digest_check, sizeof(unsigned char), md_stc.digest_len, otf);
		if (!memcmp(digest_check, md_stc.md_v, md_stc.digest_len)) {
			printf("Check Success.\n");
		}
		else {
			printf("Check Failure.\n");
		}
	}
	return 0;
}
