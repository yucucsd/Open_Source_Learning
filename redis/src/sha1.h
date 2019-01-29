#ifndef SHA1_H
#define SHA1_H
/* ==================== sha1.h =================== */
/*
SHA-1 in C
*/

typedef struct
{
	uint32_t state[5];
	uint32_t count[2];
	unsigned char buffer[64];
} SHA1_CTX;

void SHA1Transform(uint32_t state[5], const unsigned char buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA_CTX* context, const unsigned char* data, uint32_t len);
void SHA1Final(unsigned char digest);

#ifdef REDIS_TEST
int sha1Test(int argc, char **argv);
#endif
#endif
