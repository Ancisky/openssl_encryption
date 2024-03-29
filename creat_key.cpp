
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<openssl/evp.h>
#include<openssl/err.h>
#include<openssl/conf.h>
#include<openssl/pem.h>
#include<openssl/rand.h>

/*RSA密钥长度*/
#define RSASIZE 2048
/*缓冲池*/
#define BUFSIZE 1024

struct md {
	unsigned char *md_v;
	unsigned int digest_len;
};

md file_digest(FILE *inf) {
	const EVP_MD *type;
	type = EVP_md5();
	EVP_MD_CTX *mdctx;
	int read_len;
	unsigned char in_buf[BUFSIZE];
	md md_stc;

	md_stc.md_v = (unsigned char*)malloc(16 * sizeof(unsigned char));


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

	/*
	for (int round = 0; round < md_stc.digest_len; round++) {
		printf("%02x", md_stc.md_v[round]);
	}
	printf("\n");
	*/

	EVP_MD_CTX_destroy(mdctx);
	return md_stc;
}

int create_key(const char *privatekeypath, const char *publickeypath) {
	unsigned char pwd[20];
	md md_md5;
	RSA *res = RSA_generate_key(RSASIZE, 65537, NULL, NULL);
	FILE *priout, *pubout;

	pubout = fopen(publickeypath, "wb");
	if (!pubout) {
		fprintf(stderr, "ERROR: write pubkey file error: %s\n", strerror(errno));
		return errno;
	}
	PEM_write_RSAPublicKey(pubout, res);

	fclose(pubout);
	pubout = fopen(publickeypath, "rb");
	if (!pubout) {
		fprintf(stderr, "ERROR: write pubkey file error: %s\n", strerror(errno));
		return errno;
	}
	md_md5 = file_digest(pubout);
	fclose(pubout);

	priout = fopen(privatekeypath, "wb");
	if (!priout) {
		fprintf(stderr, "ERROR: write prikey file error: %s\n", strerror(errno));
		return errno;
	}
	//fwrite(md_md5.md_v, sizeof(unsigned char), md_md5.digest_len, priout);
	printf("Input password of private key \"%s\" ：\n", privatekeypath);
	gets_s((char*)pwd, 20);
	PEM_write_RSAPrivateKey(priout, res, EVP_aes_256_cbc(), pwd, strlen((char*)pwd), NULL, NULL);
	fclose(priout);

	return 0;
}

int main(int argc, char *argv[])
{
	int keynum = 0;

	if (!strcmp(argv[1], "-h")) {
		printf("--help\n\n");
		printf("Command : [prikey1] [prikey2] ... [pubkey1] [pubkey2] ... \n");
		return 0;
	}

	if (argc == 3) {
		create_key(argv[1], argv[2]);
	}
	if (argc > 3) {
		keynum = (argc - 1) / 2;
		for (int round = 0; round < keynum; round++) {
			create_key(argv[1 + round], argv[1 + round + keynum]);
		}
	}
}

