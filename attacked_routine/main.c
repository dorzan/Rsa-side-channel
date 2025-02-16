#define _GNU_SOURCE
#include <sched.h>
#include "stdio.h"
#include <sys/select.h>
#include "time.h"
#include <gmp.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

typedef unsigned long int qword;
#define STDIN 0
#pragma pack(push, 64)
volatile int Memory[1024];
#pragma pack(pop)

static qword Average = 0;
static int Samples = 0;
int getAdrFlag=1;

#define NUMBER_OF_DIFFERENT_KEYS 1
#define KEY_LEN 4096
//#define KEY_LEN 2048
#define TIME_TO_CALC_EXP 16000
#define TIME_BETWEEN_ENCRYPTIONS 0
#define SAME_KEYS_IN_A_ROW 120
#if KEY_LEN == 4096
#define TIME_TO_CALC_EXP 16000
#else
#define TIME_TO_CALC_EXP 5200
#endif


#define WINDOW_SIZE 4
#define W 64
//#define DEMO

__always_inline uint64_t rdtscp64() {
	uint32_t low, high;
	asm volatile ("rdtscp": "=a" (low), "=d" (high) :: "ecx");
	return (((uint64_t)high) << 32) | low;
}


mpz_t* SquareMultiplyExp(mpz_t Base, mpz_t Modulu, mpz_t Exponent);
void* returnAdress();
void routine(int a);
int slotwait(uint64_t slotend);
void pinToCore(int coreId);
mpz_t* sliding_window_exponentiation(mpz_t base, mpz_t exp, mpz_t modulus);
int binary_to_decimal(mpz_t input, int start, int end);
mpz_t* fixed_window(mpz_t base, mpz_t exp, mpz_t modulus) ;
void getSandU(int m, int* s, int* u);


int main()
{
	//system("sudo sync; sudo sh -c \"echo 3 > /proc/sys/vm/drop_caches\"");
	//system("sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches");
	pinToCore(1);
	#ifdef DEMO
		routine(1);
	#endif
	MP_INT a;
	char Input[100];
	fd_set fds;
	int maxfd = STDIN, encNum = 0;
	int Counter = 0;
	int i;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1;
	struct timespec start, end;
	unsigned long long elapsedTime;
	int FirstTime = 1;
	mpz_t Key, Base, Modulu, *Result, Keys[NUMBER_OF_DIFFERENT_KEYS];
	int State = 0, currKey = 0;
	char rsaKeys[NUMBER_OF_DIFFERENT_KEYS][KEY_LEN / 4 + 3];
	memset(rsaKeys, 0, (NUMBER_OF_DIFFERENT_KEYS * (KEY_LEN / 4) + 3));
	uint64_t start_64 = 0,end_64=0, tmp = 0;

	//	for (i = 0; i < NUMBER_OF_DIFFERENT_KEYS; i++){
	//
	//		char keyFileName[20];
	//		sprintf(keyFileName, "keys/%d_%d.key",KEY_LEN, i);
	//		FILE* fd = fopen(keyFileName, "r");
	//
	//		EVP_PKEY* privkey = NULL;
	//		EVP_PKEY *pKey = NULL;
	//
	//		if (!PEM_read_PrivateKey(fd, &privkey, NULL, NULL))
	//		{
	//			fprintf(stderr, "Error loading RSA Private Key File.\n");
	//			return 2;
	//		}
	//
	//		RSA* pRsa = privkey->pkey.rsa;
	//		memcpy(&rsaKeys[i][2], (char*) (pRsa->d->d), KEY_LEN/8);
	//		rsaKeys[i][KEY_LEN / 8] = 0;
	//		EVP_PKEY_free(privkey);
	//
	//
	//		fclose(fd);
	//
	//	}
	//	if (!EVP_PKEY_assign_RSA (pKey, privkey))
	//	{
	//		fprintf(stderr, "EVP_PKEY_assign_RSA: failed.\n");
	//		return 3;
	//	}


	printf("Starting to poll memory\r\n"
			"Please press enter to move to the next step\r\n");

#if KEY_LEN == 2048
	mpz_init_set_str (Key, "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 0);

	mpz_init_set_str (Base, "0x12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678", 0);

	mpz_init_set_str (Modulu, "0xabcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234", 0);
#else
	
	/*
	mpz_init_set_str (Key, "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
			"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001111", 0);

	 */

		mpz_init_set_str (Key,"0xd31879914956d3d4a9820de7c88317c0e303898c1c0cb720f4b9894f7bd19fd1705104a9b3ff2a63155d7e10a07be6e7403883c467f5ad1"
					  "8e704c00776114178706ec44879ef4bb7b1d74e935d7623d7f631eec3598121d2165638910ee185f07d5e3422a5f829556860ff34ae133660f8a9"
					  "2d5a2052a81d33d6c1cd50762de418fa75151c6262058209be25996882959557d83180b740203291e8021768ce62387a27248f805ed8f6c0991b5"
					  "a3896c8ab320deac1c71a82b85399e6f4f0e850675e0f9ea33cd9c0442006c3a3b2fd94ec0c107bedd22a43864f282a1ccc030929b468be8230f6"
					  "0479f7bc5aebac4cd26011002684fc0b9658fecb80809ca67932423147219a0e7ccaa8b606a8fef140b59cea39b43271cd24ca8e84bf36885c200"
					  "661c36273c723c5d9b6b51c03dcda94c975c146f88c385b3c189598e3efdc8b54c7b56d6741887ea52b50025e74c3d93a23f114e8c0df11497a538"
					  "95127e28278e42daa552278636e77994b124d96fb4ec35d232caadcdbbc3c34da57d6394b58e45ca348a5e62bf0872c6061e054efed6ca36c15f43"
					  "42d8fac7f173c9c40695ae686416ebd09a47160a2f8a18464958c18795c97eecb57ee55ddf98206b67b8d4ab7d3a606f56d31a335701fdb8e91262"
					  "e42bcaf7cd762cd7a779fe4891d4cac9c9c19a007de85524b8fabcd9cf95c18759fafb008b6bcf7d7e3283ae8f", 0);

	
	

	
	
	/*
	mpz_init_set_str (Key, "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0);
*/



	/*
	mpz_init_set_str (Key, "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
			"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
			"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
			"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
			"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
			"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
			"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
			"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
			"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee", 0);
*/
	 




	mpz_init_set_str (Base, "0x12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678"
			"12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678", 0);

	mpz_init_set_str (Modulu, "0xabcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234"
			"abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234", 0);

#endif

	while(1){


		FD_ZERO(&fds);
		FD_SET(STDIN, &fds);

		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		select(maxfd+1, &fds, NULL, NULL, &timeout);
		//select(maxfd+1, &fds, NULL, NULL, NULL);


		// Check if the user is asking us to move to the next stage of the attack
		if (State == 0 && FD_ISSET(STDIN, &fds)){
			scanf("%s", Input);
			printf("Moving to 1's key\r\n");fflush(stdout);

#if KEY_LEN == 2048
			mpz_init_set_str (Key, "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
					"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
					"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
					"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0);
#else
			mpz_init_set_str (Key, "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
					"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
					"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
					"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
					"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
					"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
					"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
					"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
					"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0);
#endif
			State = 1;
			Average = 0;
			Samples = 0;
		}
		else if (State == 1 && FD_ISSET(STDIN, &fds))
		{
			scanf("%s", Input);
			printf("Moving to real key\r\n");fflush(stdout);

#if KEY_LEN == 2048
			mpz_set_str (Key, "0x023456789abcdef1dcba987654321112233445566778899aabbccddeef1eeddccbbaa99887766554433221100111222333444555666777888999aaabbbcccddd"
					"101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f"
					"505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e70f808182838485868788898a8b8c8d8e8"
					"909192939495969798999a9b9c9d9e91a0a1a2a3a4a5a6a7a8a9aaabacadaea1b0b1b2b3b4b5b6b7b8b9babbbcbdbeb1c0c1c2c3c4c5c6c7c8c9cacbcccdcec1", 0);
			/*
			mpz_set_str (Key, "0x55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555"
								"55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555"
								"55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555"
								"55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555", 0);
			 */
#else
			//			for (i = 0; i < NUMBER_OF_DIFFERENT_KEYS; i++){
			//				mpz_init_set_str(Keys[i], "0x76931fac9dab2b36c248b87d6ae33f9a62d7183a5d5789e4b2d6b441e2411dc709e111c7e1e7acb6f8cac0bb2fc4c8bc2ae3baaab9165cc458e199cb89f51b135f7091a5abb0874df3e8cb4543a5eb93b0441e9ca4c2b0fb3d30875cbf29abd5b1acf38984b35ae882809dd4cfe7abc5c61baa52e053b4c3643f204ef259d2e98042a948aac5e884cb3ec7db925643fd34fdd467e2cca406035cb2744cb90a63e51c9737903343947e02086541e4c48a99630aa9aece153843a4b190274ebc955f8592e30a2205a485846248987550aaf2094ec59e7931dc650c7451cc61c0cb2c46a1b3f2c349faff763c7f8d14ddff946351744378d62c59285a8d7915614f5a2ac9e0d68aca6248a9227ab8f1930ee38ac7a9d239c9b026a481e49d53161f9a9513fe5271c32e9c21d156eb9f1bea57f6ae4f1b1de3b7fd9cee2d9cca7b4c242d26c31d000b7f90b7fe48a131c7debfbe58165266de56e1edf26939af07ec69ab1b17d8db62143f2228b51551c3d2c7de3f5072bd4d18c3aeb64cb9e8cba838667b6ed2b2fcab04abae8676e318b402a7d15b30d2d7ddb78650cc6af82bc3d7aa805b02dd9aa523b7374a1323ee6b516d1b81e5f709c2c790edaf1c3fa9b0a1dbc6dabc2b5ed267244c458752002b106d6381fad58a7e193657bde0fe029120f8379316891f828b8d24a049e5b86d855bcfed56765f9da1ac54caeaf9257a", 0);
			//				mpz_init_set_str(Keys[i], rsaKeys[i], 0);
			//				Keys[i][0]._mp_size = 65;
			//				mpz_init_set_str(Keys[i], rsaKeys[i], 16);
			//				memcpy((char*)(Keys[i][0]._mp_d), rsaKeys[i], KEY_LEN / 8);


			//			}


			mpz_set_str (Key, "0x123456789abcdef1dcba987654321112233445566778899aabbccddeef1eeddccbbaa99887766554433221100111222333444555666777888999aaabbbcccddd"
					"101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f"
					"505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e70f808182838485868788898a8b8c8d8e8"
					"909192939495969798999a9b9c9d9e91a0a1a2a3a4a5a6a7a8a9aaabacadaea1b0b1b2b3b4b5b6b7b8b9babbbcbdbeb1c0c1c2c3c4c5c6c7c8c9cacbcccdcec1"
					"d0d1d2d3d4d5d6d7d8d9dadbdcddded1e0e1e2e3e4e5e6e7e8e9eaebecedeee1f0f1f2f3f4f5f6f7f8f91a1b1c1d1e1f123456789abcdef1dcba987654321521"
					"18152229364350576471788592991061131201271341411481551621691761831901972032102172242312392462532602672742812892963033103173243313"
					"01123581321345589144233377610987159725844181676510946177112865746368750251213931964183178115142298320401346269217830935245785702"
					"123456789abcdef1dcba987654321112233445566778899aabbccddeef1eeddccbbaa99887766554433221100111222333444555666777888999aaabbbcccddd", 0);


			/*
			mpz_set_str (Key, "0xf97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11"
								"f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11"
								"f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11"
								"f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11"
								"f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11"
								"f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11"
								"f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11"
								"f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11f97c6e11", 0);
			mpz_init_set (Keys[0], Key);
			 */
#endif
			State = 2;
			Average = 0;
			Samples = 0;
		}
		// If we are in the real key simulation stage, wait a little between one encryption to another to do "Other things".
		else if (State == 2)
		{
			struct timespec start, end;
			qword TimeSpent;

			clock_gettime(CLOCK_REALTIME, &start);

			do {
				clock_gettime(CLOCK_REALTIME, &end);
				TimeSpent = (qword)(end.tv_nsec - start.tv_nsec) + (qword)(end.tv_sec - start.tv_sec) * 1000000000;
			} while (TimeSpent < TIME_BETWEEN_ENCRYPTIONS);
		}

		//gmp_printf("Exp: %Zx\r\n", Key);
		//end_64=rdtscp64();
		while(1){
		//Result = SquareMultiplyExp(Base, Modulu, Key);
		//gmp_printf("%Zd\n\n", Result);
	    //Result = sliding_window_exponentiation(Base, Key, Modulu);	
		//gmp_printf("%Zd\n\n", Result);
		Result = fixed_window(Base, Key, Modulu);	
		//gmp_printf("%Zd\n\n", Result);
		//while(1);
	
		}
		//start_64=rdtscp64();

		//printf(" Spent %ld \t",start_64-tmp);
		//printf("2 : Spent %d \n",end_64-start_64);

		if( (encNum % SAME_KEYS_IN_A_ROW) == 0){
			encNum =0;
			if (currKey == NUMBER_OF_DIFFERENT_KEYS - 1)
				currKey = 0;
			else
				currKey++;
		}

		encNum++;
		//gmp_printf ("%Zd\n", Result);

		mpz_clear(*Result);




		free(Result);

	}

	return 0;
}

mpz_t* SquareMultiplyExp(mpz_t Base, mpz_t Modulu, mpz_t Exponent){
	char command[90];
	pid_t pid;
	void* adr, *adr2,* adr3;
	mpz_t* r = (mpz_t*)malloc(sizeof(mpz_t));
	int  i,k=0;
	mpz_t tmp;
	uint64_t start_64 = 0,end_64, avg = 0;
	mpz_init_set_str(*r, "1", 10);
	mpz_init(tmp);

	//start_64=rdtscp64();
	//while(1)
	for (i = (KEY_LEN - 1); i >= 0; i--)
	{			
		
		//Times_Array[m++] = rdtscp64();
		//struct timespec start, end;
		//qword TimeSpent;
		adr=returnAdress();
		mpz_set(tmp, *r);
		mpz_powm_ui(*r, tmp, 2, Modulu);
		if (mpz_tstbit(Exponent, i) )
		{
			//adr2=returnAdress();
			mpz_mul(tmp, *r, Base);
			mpz_mod(*r, tmp, Modulu);
		}


		if(getAdrFlag)
		{
			getAdrFlag=0;
			pid=getpid();
			sprintf(command,"sudo ./VTP %ld %p", pid, adr);
			system(command);
			//sprintf(command,"sudo ./VTP %ld %p", pid, adr2);
			//system(command);
		}

	}
	mpz_clear(tmp);
	return r;
}


mpz_t* sliding_window_exponentiation(mpz_t base, mpz_t exp, mpz_t modulus) {
	void* adr, *adr2,* adr3;
	mpz_t* output = (mpz_t*)malloc(sizeof(mpz_t));
	const int table_length = 1 << (WINDOW_SIZE - 1);
	mpz_t table[table_length];
	mpz_t square_base;
	mpz_t temp;
	int i;
	int exp_length;
	int l;
	// value in window
	int u;
	pid_t pid;

	char command[90];
	uint64_t start, end;
	

	mpz_init(temp);
	mpz_init(square_base);
	start = rdtscp64();
	// T[0] = x
	mpz_init_set(table[0], base);
	// x^2 mod N
	mpz_mul(square_base, base, base);
	mpz_mod(square_base, square_base, modulus);
	// Compute lookup table.
	for(i = 1; i < table_length; i++) {
		mpz_init(table[i]);
		mpz_mul(table[i], table[i-1], square_base);
		mpz_mod(table[i], table[i], modulus);
	}
	// t = 1
	mpz_init_set_ui(output, 1);
	// |y|
	//exp_length = mpz_size(exp) * mp_bits_per_limb;
	exp_length = KEY_LEN;
	// |y| - 1
	i = exp_length - 1;
	while(i >= 0) {
		adr=returnAdress();
		if(!mpz_tstbit(exp, i)) {
			l = i;
			u = 0;
		} else {
			l = ((i - WINDOW_SIZE + 1) > 0) ? (i - WINDOW_SIZE + 1) : 0;
			while(!mpz_tstbit(exp,l)) {
				l++;
			}
			// Set u = exp bits between i and l
			u = binary_to_decimal(exp, i, l);
		}
		// t' = t
		mpz_set(temp, output);
		//adr=returnAdress();

		// t^p
		int p = 1 << (i - l + 1);
		if(p>0) {
			// t = t'^p (mod N)
			mpz_powm_ui(output, temp, p, modulus);
		}
		if(u != 0) {
			// t = t * T[(u-1)/2] mod N
			mpz_mul(output, output, table[(u-1)/2]);
			mpz_mod(output, output, modulus);
		}
		i = l - 1;

		if(getAdrFlag)
		{
			getAdrFlag=0;
			pid=getpid();
			sprintf(command,"sudo ./VTP %ld %p", pid, adr);
			system(command);
		}
		//end = rdtscp64()-start;
		//printf("%ld\n",end);	
	}

	mpz_clear(square_base);
	mpz_clear(temp);
	for(i=0 ; i<table_length; i++) {
		mpz_clear(table[i]);
	}
	return output;
}


mpz_t* fixed_window(mpz_t base, mpz_t exp, mpz_t modulus) {
	void* adr, *adr2,* adr3;
	mpz_t* output = (mpz_t*)malloc(sizeof(mpz_t));
	const int table_length = 1 << (WINDOW_SIZE - 1);
	mpz_t table[table_length];
	mpz_t square_base;
	mpz_t temp;
	int i, u, s, m;
	int exp_length;
	int l;
	// value in window
	pid_t pid;
	char command[90];
	
	mpz_init(temp);
	mpz_init(square_base);
	// T[0] = x
	mpz_init_set(table[0], base);
	// x^2 mod N
	mpz_mul(square_base, base, base);
	mpz_mod(square_base, square_base, modulus);
	// Compute lookup table.
	for(i = 1; i < table_length; i++) {
		mpz_init(table[i]);
		mpz_mul(table[i], table[i-1], square_base);
		mpz_mod(table[i], table[i], modulus);
	}
	// t = 1
	mpz_init_set_ui(output, 1);
	// |y|
	//exp_length = mpz_size(exp) * mp_bits_per_limb;
	exp_length = KEY_LEN;
	// |y| - 1
	i = exp_length - 1;
	while(i >= 0) {
		adr=returnAdress();
		l = ((i - WINDOW_SIZE + 1) > 0) ? (i - WINDOW_SIZE + 1) : 0;
		m = binary_to_decimal(exp, i, l);
		getSandU(m, &s, &u);	
		int p = 1 << ((i-l +1) - s);
		if(p>0) {
			mpz_set(temp, output);
			mpz_powm_ui(output, temp, p, modulus);
		}
		if(u != 0) {
			mpz_mul(output, output, table[(u-1)/2]);		
			mpz_mod(output, output, modulus);
		}
		p = 1 << s;
		if(p>0) {
			mpz_set(temp, output);
			mpz_powm_ui(output, temp, p, modulus);
		}
		i = l - 1;
		if(getAdrFlag)
		{
			getAdrFlag=0;
			pid=getpid();
			sprintf(command,"sudo ./VTP %ld %p", pid, adr);
			system(command);
		}
	}
	mpz_clear(square_base);
	mpz_clear(temp);
	for(i=0 ; i<table_length; i++) {
		mpz_clear(table[i]);
	}
	return output;
}

void getSandU(int m, int* s, int* u){
	int i = 0,count = 0;
	if (!m)
	{
		*u = 0;
		*s = WINDOW_SIZE -1;
		return;
	}
	while ((i = m % 2) == 0)
	{
		m = m/2;
		count++;
	}
	
	*u = m;
	*s = count;
}





int binary_to_decimal(mpz_t input, int start, int end) {
    int i;
    int result = 0;
    int g = 1;
    for(i = end; i<=start; i++) {
        int bit = mpz_tstbit(input, i);
        if(bit) {
            result += g * bit;
        }
        g <<= 1;
    }
    return result;
}




void* __attribute__ ((noinline)) returnAdress(){
	return __builtin_return_address(0);
}

void routine(int a){
	 int  i=5, k = 0,t = 40;
	//printf("press enter");
	//scanf("%d",&i);
	int volatile * volatile j;
	char command[90];
	pid_t pid;
	uint64_t start, end;
	void* adr, *adr2, *adr3;
	start = rdtscp64();
	while(1){
	//for(i = 0; i < iterations ; i ++){
		//times[i] = rdtscp64();
		//j=0;
		t=1;
		adr=returnAdress();
			t = t*12;
			if (k==1)
			{
			slotwait(rdtscp64() + 16000);
			k=0;
			}
			else
			{
			slotwait(rdtscp64() + 8000);
				k=1;
			}
			
		//	k=0;
		//}
		if(getAdrFlag)
		{
			getAdrFlag=0;
			//printf("\n adress is %p     ",adr);
			pid=getpid();
			sprintf(command,"sudo ./VTP %ld %p", pid, adr);
			system(command);
			//printf("press enter");
			//scanf("%d",&i);
			//adr2=returnAdress();

			//sprintf(command,"sudo  ./VTP %ld %p", pid, adr2);
				//system(command);
			//	sprintf(command,"sudo  ./VTP %ld %p", pid, adr3);
			//system(command);
		}

	}

}



/*__always_inline*/ int slotwait(uint64_t slotend) {
  if (rdtscp64() > slotend)
    return 1;
  while (rdtscp64() < slotend);
  return 0;
}

void pinToCore(int coreId){
    int rs;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    rs = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (rs) {
        perror("pthread_setaffinity_np");
        exit(EXIT_FAILURE);
    }
}
