#include <tommath.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

int cb(unsigned char* dst, int len, void*)
{
	static int f = -1;
	if (f < 0)
	{
		f = open("/dev/urandom", 0);
		if (f < 0)
		{
			perror("open /dev/urandom");
			exit(1);
		}
	}
	int rc = read(f, dst, len);
	if (rc < 0)
	{
		perror("read /dev/urandom");
		exit(1);
	}
	if (rc != len)
	{
		printf("Read %d from /dev/urandom instead %d\n", rc, len);
		exit(1);
	}
	return rc;
}

main()
{
	mp_int prime;
	assert(mp_init(&prime) == 0);
	int rc = mp_prime_random_ex(&prime, 256, 1024, LTM_PRIME_SAFE, cb, NULL);
	assert(rc == 0);

	int size;
	assert(mp_radix_size(&prime, 16, &size) == 0);

	char* rad = new char[size + 1];
	assert(mp_toradix(&prime, rad, 16) == 0);
	rad[size] = 0;

	printf("%s\n", rad);
}
