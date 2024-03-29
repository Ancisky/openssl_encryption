// envelope_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

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
/*对称加密算法密钥长度*/
#define DESSIZE 8
#define AES128SIZE 16
#define AES192SIZE 24
#define AES256SIZE 32
/*初始化向量*/
#define DESIVSIZE 8
#define AESIVSIZE 16
/*缓冲池*/
#define BUFSIZE 1024
#define RSABUF 256
#define SYMBUF 1024
#define ENKEY 1024


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
/*
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
	fwrite(md_md5.md_v, sizeof(unsigned char), md_md5.digest_len, priout);
	printf("Input password of private key \"%s\" ：\n", privatekeypath);
	gets_s((char*)pwd, 20);
	PEM_write_RSAPrivateKey(priout, res, EVP_aes_256_cbc(), pwd, strlen((char*)pwd), NULL, NULL);
	fclose(priout);

	return 0;
}
*/
EVP_PKEY** import_key(char **keylist, int keynum, int flag, unsigned char **_md) {
	EVP_PKEY **key;
	RSA *rsa;
	rsa = RSA_new();
	FILE *inf;
	char pwd[20];
	md *md_md5;
	md_md5 = (md*)malloc(keynum * sizeof(md));
	key = (EVP_PKEY**)malloc(keynum * sizeof(EVP_PKEY*));
	for (int round = 0; round < keynum; round++) {
		key[round] = EVP_PKEY_new();
	}
	for (int round = 0; round < keynum; round++) {
		if (flag == 1) {
			inf = fopen(keylist[round], "rb");
			if (!keylist[round]) {
				fprintf(stderr, "ERROR: read prikey file error: %s\n", strerror(errno));
				exit(errno);
			}

			printf("Input password of privatekey %s :\n", keylist[round]);
			gets_s(pwd, 20);
			rsa = PEM_read_RSAPrivateKey(inf, NULL, NULL, pwd);

			FILE *temp_pub;
			md temp_md;
			int temp_read = 0;
			char temp_buf[BUFSIZE];
			temp_pub = tmpfile();
			PEM_write_RSAPublicKey(temp_pub, rsa);
			fseek(temp_pub, 0L, 0);
			temp_md = file_digest(temp_pub);
			memcpy(_md[round], temp_md.md_v, temp_md.digest_len);
			fclose(temp_pub);

			EVP_PKEY_set1_RSA(key[round], rsa);
			fclose(inf);
		}
		if (flag == 0) {
			inf = fopen(keylist[round], "rb");
			if (!keylist[round]) {
				fprintf(stderr, "ERROR: read pubkey file error: %s\n", strerror(errno));
				exit(errno);
			}
			rsa = PEM_read_RSAPublicKey(inf, NULL, NULL, NULL);
			EVP_PKEY_set1_RSA(key[round], rsa);
			//printf("%s\n", key[round]);
			fseek(inf, 0L, 0);
			md_md5[round] = file_digest(inf);
			memcpy(_md[round], md_md5[round].md_v, 16);
			fclose(inf);
		}
	}
	free(md_md5);
	return key;
}

int envelope_seal(EVP_PKEY **pub_key, FILE *inf, FILE *otf, int keynum, const EVP_CIPHER *type, int flag, unsigned char **_md){

	unsigned char **encrypted_key;
	int *encrypted_key_len;
	unsigned char in_buf[SYMBUF], *out_buf;
	unsigned char key[32], *iv;
	int read_len, out_len, iv_len, location1, location2;
	location1 = 0;
	location2 = 0;

	encrypted_key = (unsigned char**)malloc(keynum * sizeof(unsigned char*));
	for (int round = 0; round < keynum; round++)
		encrypted_key[round] = (unsigned char*)malloc(ENKEY * sizeof(unsigned char));

	encrypted_key_len = (int*)malloc(keynum * sizeof(int));

	if (type == EVP_des_cbc()) {
		iv_len = DESIVSIZE;
		iv = (unsigned char*)malloc(DESIVSIZE * sizeof(unsigned char));
		out_buf = (unsigned char*)malloc((DESIVSIZE + SYMBUF) * sizeof(unsigned char));
	}
	else {
		iv_len = AESIVSIZE;
		iv = (unsigned char*)malloc(AESIVSIZE * sizeof(unsigned char));
		out_buf = (unsigned char*)malloc((AESIVSIZE + SYMBUF) * sizeof(unsigned char));
	}

	EVP_CIPHER_CTX *ctx;

	/* Create and initialise the context */
	if (!(ctx = EVP_CIPHER_CTX_new())) {
		fprintf(stderr, "ERROR: EVP_CIPHER_CTX_new failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
	}

	if (!EVP_SealInit(ctx, type, encrypted_key, encrypted_key_len, iv, pub_key, keynum)) {
		fprintf(stderr, "ERROR: EVP_SealInit failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
	}

	fwrite(&iv_len, sizeof(int), 1, otf);
	fwrite(iv, sizeof(unsigned char), iv_len, otf);
	fwrite(&flag, sizeof(int), 1, otf);
	fwrite(&keynum, sizeof(int), 1, otf);

	for (int round = 0; round < keynum; round++) {
		fwrite(_md[round], sizeof(unsigned char), 16, otf);
	}

	for (int round2, round=0; round < keynum; round++) {
		location1 = location2;
		fwrite(&location1, sizeof(int), 1, otf);
		fwrite((encrypted_key_len + round), sizeof(int), 1, otf);
		location2 += encrypted_key_len[round];
	}
	for (int round = 0; round < keynum; round++) {
		fwrite(encrypted_key[round], sizeof(unsigned char), encrypted_key_len[round], otf);
	}
	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_SealUpdate can be called multiple times if necessary
	 */
	EVP_CIPHER_CTX_set_padding(ctx, 1);
	while (1) {
		read_len = fread(in_buf, sizeof(unsigned char), SYMBUF, inf);
		if (1 != EVP_SealUpdate(ctx, out_buf, &out_len, in_buf, read_len)) {
			fprintf(stderr, "ERROR: EVP_SealUpdate failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
			EVP_CIPHER_CTX_cleanup(ctx);
			fclose(inf);
			fclose(otf);
		}
		fwrite(out_buf, sizeof(unsigned char), out_len, otf);
		if (ferror(otf)) {
			fprintf(stderr, "ERROR: fwrite error: %s\n", strerror(errno));
			EVP_CIPHER_CTX_cleanup(ctx);
			fclose(inf);
			fclose(otf);
		}
		if (read_len < SYMBUF) {
			/*文件读取结束*/
			break;
		}
	}
	/* Finalise the encryption. Further ciphertext bytes may be written at
	 * this stage.
	 */
	if (1 != EVP_SealFinal(ctx, out_buf, &out_len)) {
		fprintf(stderr, "ERROR: EVP_SealFinal failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(inf);
		fclose(otf);
	}
	fwrite(out_buf, sizeof(unsigned char), out_len, otf);
	if (ferror(otf)) {
		fprintf(stderr, "ERROR: fwrite error: %s\n", strerror(errno));
		EVP_CIPHER_CTX_cleanup(ctx);
		fclose(inf);
		fclose(otf);
	}
	EVP_CIPHER_CTX_free(ctx);

	return 0;
}

int envelope_open(EVP_PKEY *priv_key, FILE *inf, FILE *otf, unsigned char *_md)
{
	unsigned char in_buf[SYMBUF], *out_buf;
	unsigned char iv[16], md_md5[16];
	int iv_len;
	const EVP_CIPHER *type;
	int flag = 0, keynum = 0;
	unsigned char *encrypted_key;
	int encrypted_key_len, key_len;
	int location = 0, cipherloc = 0, distence = 0;
	int read_len, out_len;
	int seqnum = 0;

	fread(&iv_len, sizeof(int), 1, inf);

	//iv = (char*)malloc(iv_len * sizeof(char));

	fread(iv, sizeof(unsigned char), iv_len, inf);
	fread(&flag, sizeof(int), 1, inf);
	fread(&keynum, sizeof(int), 1, inf);

	if (flag == 1)
		out_buf = (unsigned char*)malloc((SYMBUF + DESIVSIZE) * sizeof(unsigned char));
	else
		out_buf = (unsigned char*)malloc((SYMBUF + AESIVSIZE) * sizeof(unsigned char));

	if (flag == 1) {
		type = EVP_des_cbc();
		key_len = DESSIZE;
	}
	else if (flag == 2) {
		type = EVP_aes_128_cbc();
		key_len = AES128SIZE;
	}
	else if (flag == 3) {
		type = EVP_aes_192_cbc();
		key_len = AES192SIZE;
	}
	else {
		type = EVP_aes_256_cbc();
		key_len = AES256SIZE;
	}
		
	for (seqnum = 0; seqnum < keynum; seqnum++) {
		fread(md_md5, sizeof(unsigned char), 16, inf);
		if (!memcmp(_md, md_md5, 16)) {
			break;
		}
	}
	if (seqnum == keynum) {
		printf("Error : No Such Envelope Could Decrypted .");
		free(out_buf);
		return -1;
	}

	fseek(inf, 1L * (12 + iv_len + keynum * 16), 0);

	for (int round = 0; round < keynum; round++) {
		if (seqnum == round) {
			fread(&location, sizeof(int), 1, inf);
			fread(&encrypted_key_len, sizeof(int), 1, inf);
			cipherloc += encrypted_key_len;
			continue;
		}
		fseek(inf, 1L * sizeof(int), 1);
		fread(&distence, sizeof(int), 1, inf);
		cipherloc += distence;
	}

	fseek(inf, 1L * location, 1);
	encrypted_key = (unsigned char*)malloc(ENKEY * sizeof(unsigned char));
	fread(encrypted_key, sizeof(unsigned char), encrypted_key_len, inf);

	
	fseek(inf, 1L * (12 + iv_len + keynum * 16 + keynum * 8 + cipherloc), 0);


	EVP_CIPHER_CTX *ctx;

	int len;

	int plaintext_len;


	/* Create and initialise the context */
	if (!(ctx = EVP_CIPHER_CTX_new())) {
		fprintf(stderr, "ERROR: EVP_CIPHER_CTX_new failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
	}

	/* Initialise the decryption operation. The asymmetric private key is
	 * provided and priv_key, whilst the encrypted session key is held in
	 * encrypted_key */
	if (1 != EVP_OpenInit(ctx, type, encrypted_key, encrypted_key_len, iv, priv_key)) {
		fprintf(stderr, "ERROR: EVP_OpenInit failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
	}


	//EVP_CIPHER_CTX_set_padding(ctx, 1);
	while (1) {
		read_len = fread(in_buf, sizeof(unsigned char), SYMBUF, inf);
		if (1 != EVP_OpenUpdate(ctx, out_buf, &out_len, in_buf, read_len)) {
			fprintf(stderr, "ERROR: EVP_OpenUpdate failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
			EVP_CIPHER_CTX_cleanup(ctx);
		}
		fwrite(out_buf, sizeof(unsigned char), out_len, otf);
		if (read_len < SYMBUF) {
			break;
		}
	}
	if (1 != EVP_OpenFinal(ctx, out_buf, &out_len)) {
		fprintf(stderr, "ERROR: EVP_SealFinal failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
		EVP_CIPHER_CTX_cleanup(ctx);
	}
	fwrite(out_buf, sizeof(unsigned char), out_len, otf);
	EVP_CIPHER_CTX_free(ctx);

	return 0;
}

int main(int argc, char *argv[]) {

	const EVP_CIPHER *type;
	int keynum = 0;
	int flag = 0;
	char **keypath;
	EVP_PKEY **keylist;
	FILE *inf, *otf;
	RSA *rsa_test;
	rsa_test = RSA_new();
	unsigned char **_md;
	RSA *temp_rsa;
	FILE *temp_pub;
	FILE *temp_pri;
	char temp_buf[BUFSIZE];
	int temp_read = 0;
	char pwd[20];

	if (!strcmp(argv[1], "-h")) {
		printf("--help\n\n");
		printf("[-e] [sym_method] [inputfile] [outputfile] [pubkey1] [pubkey2]...\n");
		printf("[-d] [inputfile] [outputfile] [prikey]\n");
		return 0;
	}

	if (!strcmp(argv[1], "-e")) {
		if (!strcmp(argv[2], "des")) {
			type = EVP_des_cbc();
			flag = 1;
		}
		else if (!strcmp(argv[2], "aes128")) {
			type = EVP_aes_128_cbc();
			flag = 2;
		}
		else if (!strcmp(argv[2], "aes192")) {
			type = EVP_aes_192_cbc();
			flag = 3;
		}
		else if (!strcmp(argv[2], "aes256")) {
			type = EVP_aes_256_cbc();
			flag = 4;
		}
		else {
			printf("Cipher Algorithm Error: Wrong Command.");
			return 1;
		}
		inf = fopen(argv[3], "rb");
		if (!inf) {
			fprintf(stderr, "ERROR: read input file error: %s\n", strerror(errno));
			return errno;
		}
		otf = fopen(argv[4], "wb");
		if (!otf) {
			fprintf(stderr, "ERROR: write output file error: %s\n", strerror(errno));
			return errno;
		}
		keynum = argc - 5;
		_md = (unsigned char**)malloc(keynum * sizeof(unsigned char*));
		keypath = (char**)malloc(keynum * sizeof(char*));
		for (int round = 0; round < keynum; round++) {
			_md[round] = (unsigned char*)malloc(16 * sizeof(unsigned char));
			keypath[round] = argv[5 + round];
		}
		keylist = import_key(keypath, keynum, 0, _md);
		envelope_seal(keylist, inf, otf, keynum, type, flag, _md);
		fclose(inf);
		fclose(otf);
		printf("OK!\n");
	}

	if (!strcmp(argv[1], "-d")) {
		_md = (unsigned char**)malloc(1 * sizeof(unsigned char*));
		_md[0] = (unsigned char*)malloc(16 * sizeof(unsigned char));
		inf = fopen(argv[2], "rb");
		if (!inf) {
			fprintf(stderr, "ERROR: read input file error: %s\n", strerror(errno));
			return errno;
		}
		otf = fopen(argv[3], "wb");
		if (!otf) {
			fprintf(stderr, "ERROR: write output file error: %s\n", strerror(errno));
			return errno;
		}
		keypath = (char**)malloc(keynum * sizeof(char*));
		keypath[0] = argv[4];
		keylist = import_key(keypath, 1, 1, _md);
		envelope_open(keylist[0], inf, otf, _md[0]);
		fclose(inf);
		fclose(otf);
		printf("OK!\n");
	}

	if (!strcmp(argv[1], "-t")) {
		temp_pri = fopen(argv[2], "rb");
		if (!temp_pri) {
			fprintf(stderr, "ERROR: read input file error: %s\n", strerror(errno));
			return errno;
		}
		temp_pub = tmpfile();
		if (!temp_pub) {
			fprintf(stderr, "ERROR: read input file error: %s\n", strerror(errno));
			return errno;
		}
		printf("Input password of privatekey %s :\n", argv[2]);
		gets_s(pwd, 20);
		temp_rsa = PEM_read_RSAPrivateKey(temp_pri, NULL, NULL, pwd);
		PEM_write_RSAPublicKey(temp_pub, temp_rsa);
		fseek(temp_pub, 0L, 0);
		while (1) {
			temp_read = fread(temp_buf, sizeof(char), BUFSIZE - 1, temp_pub);
			temp_buf[temp_read] = '\0';
			puts(temp_buf);
			if (temp_read < BUFSIZE - 1)
				break;
		}
		fclose(temp_pri);
		fclose(temp_pub);
	}

	return 0;
}

