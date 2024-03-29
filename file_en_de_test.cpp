#include <stdio.h>
#include <string.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include <openssl/x509.h>
#include <conio.h>
#include <direct.h>
#include<openssl/rand.h>

/* 密钥长 */
#define AES_128_KEY_SIZE 16
#define AES_192_KEY_SIZE 24
#define AES_256_KEY_SIZE 32
#define DES_KEY_SIZE 8
/* 块长 */
#define AES_BLOCK_SIZE 16
#define DES_BLOCK_SIZE 8
/*缓冲池*/
#define BUFSIZE 1024
/*随机数*/
#define RANDSIZE 1024

void encrypt(FILE *ifp, FILE *ofp,const EVP_CIPHER *type) {
	unsigned char *key, *iv;
	if (type == EVP_aes_128_cbc())
		key = (unsigned char*)malloc(AES_128_KEY_SIZE * sizeof(unsigned char));
	else if(type==EVP_aes_192_cbc())
		key = (unsigned char*)malloc(AES_192_KEY_SIZE * sizeof(unsigned char));
	else if(type==EVP_aes_256_cbc())
		key = (unsigned char*)malloc(AES_256_KEY_SIZE * sizeof(unsigned char));
	else if(type==EVP_des_cbc())
		key = (unsigned char*)malloc(DES_KEY_SIZE * sizeof(unsigned char));
	else exit(0);

	unsigned char in_buf[BUFSIZE], *out_buf;

	if (type == EVP_des_cbc()) {
		iv = (unsigned char*)malloc(DES_BLOCK_SIZE * sizeof(unsigned char));
		out_buf = (unsigned char*)malloc(BUFSIZE+DES_BLOCK_SIZE * sizeof(unsigned char));
	}
	else {
		iv = (unsigned char*)malloc(AES_BLOCK_SIZE * sizeof(unsigned char));
		out_buf = (unsigned char*)malloc(BUFSIZE + AES_BLOCK_SIZE * sizeof(unsigned char));
	}

	int num_bytes_read, out_len;
	EVP_CIPHER_CTX *ctx;
	ctx = EVP_CIPHER_CTX_new();

	if (!EVP_EncryptInit_ex(ctx,type,NULL,key,iv)) {
		fprintf(stderr, "ERROR: EVP_CipherInit_ex failed. OpenSSL error: %s\n",ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(ifp);
		fclose(ofp);
	}

	/*设置随机数*/
	unsigned char salt[RANDSIZE];
	time_t *timer;
	timer = (time_t*)malloc(sizeof(time_t));
	time(timer);

	RAND_seed(time, sizeof(time_t));
	RAND_bytes(salt, RANDSIZE);

	//设置key和iv
	const int ROUND = 2;
	char pwd[21];
	pwd[20] = '\0';
	printf("please input your password:\n");
	for (int round = 0; round < 20; round++) {
		pwd[round] = _getwch();
		if (pwd[round] == '\r') {
			pwd[round] = '\0';
			break;
		}
		printf("*");
	}

	EVP_BytesToKey(type, EVP_md5(), (const unsigned char*)salt, (const unsigned char*)pwd, (int)strlen(pwd), ROUND, key, iv);

	EVP_CIPHER_CTX_set_padding(ctx, 1);

	if (!EVP_EncryptInit_ex(ctx, type, NULL, key, iv)) {
		fprintf(stderr, "ERROR: EVP_CipherInit_ex failed. OpenSSL error: %s\n",ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(ifp);
		fclose(ofp);
	}

	fwrite(salt, sizeof(unsigned char), RANDSIZE, ofp);

	while (1) {
		num_bytes_read = fread(in_buf, sizeof(unsigned char), BUFSIZE, ifp);
		if (ferror(ifp)) {
			fprintf(stderr, "ERROR: fread error: %s\n", strerror(errno));
			EVP_CIPHER_CTX_cleanup(ctx);
			fclose(ifp);
			fclose(ofp);
		}
		if (!EVP_CipherUpdate(ctx, out_buf, &out_len, in_buf, num_bytes_read)) {
			fprintf(stderr, "ERROR: EVP_CipherUpdate failed. OpenSSL error: %s\n",ERR_error_string(ERR_get_error(), NULL));
			EVP_CIPHER_CTX_cleanup(ctx);
			fclose(ifp);
			fclose(ofp);
		}
		fwrite(out_buf, sizeof(unsigned char), out_len, ofp);
		if (ferror(ofp)) {
			fprintf(stderr, "ERROR: fwrite error: %s\n", strerror(errno));
			EVP_CIPHER_CTX_cleanup(ctx);
			fclose(ifp);
			fclose(ofp);
		}
		if(num_bytes_read<BUFSIZE){
			/*文件读取结束*/
			break;
		}
	}
	
	if (!EVP_EncryptFinal_ex(ctx, out_buf, &out_len)) {
		fprintf(stderr, "ERROR: EVP_CipherFinal_ex failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(ifp);
		fclose(ofp);
	}
	fwrite(out_buf, sizeof(unsigned char), out_len, ofp);
	if (ferror(ofp)) {
		fprintf(stderr, "ERROR: fwrite error: %s\n", strerror(errno));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(ifp);
		fclose(ofp);
	}
	EVP_CIPHER_CTX_cleanup(ctx);
}

void decrypt(FILE *ifp, FILE *ofp, const EVP_CIPHER *type) {
	unsigned char *key, *iv;
	if (type == EVP_aes_128_cbc())
		key = (unsigned char*)malloc(AES_128_KEY_SIZE * sizeof(unsigned char));
	else if (type == EVP_aes_192_cbc())
		key = (unsigned char*)malloc(AES_192_KEY_SIZE * sizeof(unsigned char));
	else if (type == EVP_aes_256_cbc())
		key = (unsigned char*)malloc(AES_256_KEY_SIZE * sizeof(unsigned char));
	else if (type == EVP_des_cbc())
		key = (unsigned char*)malloc(DES_KEY_SIZE * sizeof(unsigned char));
	else exit(0);

	unsigned char in_buf[BUFSIZE], *out_buf;

	if (type == EVP_des_cbc()) {
		iv = (unsigned char*)malloc(DES_BLOCK_SIZE * sizeof(unsigned char));
		out_buf = (unsigned char*)malloc(BUFSIZE + DES_BLOCK_SIZE * sizeof(unsigned char));
	}
	else {
		iv = (unsigned char*)malloc(AES_BLOCK_SIZE * sizeof(unsigned char));
		out_buf = (unsigned char*)malloc(BUFSIZE + AES_BLOCK_SIZE * sizeof(unsigned char));
	}
	int num_bytes_read, out_len;
	EVP_CIPHER_CTX *ctx;
	ctx = EVP_CIPHER_CTX_new();

	if (!EVP_DecryptInit_ex(ctx, type, NULL, key, iv)) {
		fprintf(stderr, "ERROR: EVP_CipherInit_ex failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(ifp);
		fclose(ofp);
	}

	unsigned char salt[RANDSIZE];
	fread(salt, sizeof(unsigned char), RANDSIZE, ifp);

	//设置key和iv
	const int ROUND = 2;
	char pwd[21];
	pwd[20] = '\0';
	printf("please input your password:\n");
	for (int round = 0; round < 20; round++) {
		pwd[round] = _getwch();
		if (pwd[round] == '\r') {
			pwd[round] = '\0';
			break;
		}
		printf("*");
	}

	EVP_BytesToKey(type, EVP_md5(), (const unsigned char*)salt, (const unsigned char*)pwd, (int)strlen(pwd), ROUND, key, iv);

	EVP_CIPHER_CTX_set_padding(ctx, 1);

	if (!EVP_DecryptInit_ex(ctx, type, NULL, key, iv)) {
		fprintf(stderr, "ERROR: EVP_CipherInit_ex failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(ifp);
		fclose(ofp);
	}

	while (1) {
		num_bytes_read = fread(in_buf, sizeof(unsigned char), BUFSIZE, ifp);
		if (ferror(ifp)) {
			fprintf(stderr, "ERROR: fread error: %s\n", strerror(errno));
			EVP_CIPHER_CTX_cleanup(ctx);
			fclose(ifp);
			fclose(ofp);
		}
		if (!EVP_CipherUpdate(ctx, out_buf, &out_len, in_buf, num_bytes_read)) {
			fprintf(stderr, "ERROR: EVP_CipherUpdate failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
			EVP_CIPHER_CTX_cleanup(ctx);
			fclose(ifp);
			fclose(ofp);
		}
		fwrite(out_buf, sizeof(unsigned char), out_len, ofp);
		if (ferror(ofp)) {
			fprintf(stderr, "ERROR: fwrite error: %s\n", strerror(errno));
			EVP_CIPHER_CTX_cleanup(ctx);
			fclose(ifp);
			fclose(ofp);
		}
		if (num_bytes_read < BUFSIZE) {
			/*文件读取结束*/
			break;
		}
	}
	/*对不足块大小的文件填充后进行加密*/
	if (!EVP_DecryptFinal_ex(ctx, out_buf, &out_len)) {
		fprintf(stderr, "ERROR: EVP_CipherFinal_ex failed. OpenSSL error: %s\n",ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(ifp);
		fclose(ofp);
	}
	fwrite(out_buf, sizeof(unsigned char), out_len, ofp);
	if (ferror(ofp)) {
		fprintf(stderr, "ERROR: fwrite error: %s\n", strerror(errno));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(ifp);
		fclose(ofp);
	}
	EVP_CIPHER_CTX_cleanup(ctx);
}


int main(int argc, char *argv[]) {
	FILE *f_input, *f_enc, *f_dec;
	const EVP_CIPHER *type;

	if (!strcmp(argv[1], "-h")) {
		printf("help:\n\n");
		printf("Command: [-e]/[-d] [Method] [Input File Path] [Output File Path]\n\n");
		printf("-e                         加密文件\n");
		printf("-d                         加密文件\n");
		printf("aes128                     使用aes128算法加密\n");
		printf("aes192                     使用aes192算法加密\n");
		printf("aes256                     使用aes256算法加密\n");
		printf("des                        使用des算法加密\n");
		return 0;
	}

	/*选择算法*/
	if (!strcmp(argv[2], "des"))
		type = EVP_des_cbc();
	else if (!strcmp(argv[2], "128aes"))
		type = EVP_aes_128_cbc();
	else if (!strcmp(argv[2], "192aes"))
		type = EVP_aes_192_cbc();
	else if (!strcmp(argv[2], "256aes"))
		type = EVP_aes_256_cbc();
	else {
		printf("Cipher Algorithm Error: Wrong Command.");
		return 0;
	}

	if (!strcmp(argv[1], "-e")) {
		/*按二进制读文件*/
		f_input = fopen(argv[3], "rb");
		if (!f_input) {
			fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
			return errno;
		}
		/* 按二进制形式写文件（如果没有会创建） */
		f_enc = fopen(argv[4], "wb");
		if (!f_enc) {
			fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
			return errno;
		}
		/* 加密文件 */
		encrypt(f_input, f_enc, type);

		fclose(f_input);
		fclose(f_enc);
	}

	if (!strcmp(argv[1], "-d")) {
		/* 按二进制形式读文件 */
		f_input = fopen(argv[3], "rb");
		if (!f_input) {
			fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
			return errno;
		}
		/* 按二进制形式写文件（如果没有会创建） */
		f_dec = fopen(argv[4], "wb");
		if (!f_dec) {
			fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
			return errno;
		}
		/* 解密文件 */
		decrypt(f_input, f_dec, type);

		fclose(f_input);
		fclose(f_dec);
	}

	return 0;
}

