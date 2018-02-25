#include "cblib/Rand.h"
//#include "cblib/Journal.h"
#include <stdlib.h>

START_CB

COMPILER_ASSERT( (RAND_MAX+1) == (RAND_MAX_PLUS_ONE) );

static unsigned long stb__rand_seed = 0;

int myrand()
{
	// shuffle non-random bits to the middle, and xor to decorrelate with seed
	unsigned long result = 0x31415926 ^
		((stb__rand_seed >> 16) + (stb__rand_seed << 16));
	stb__rand_seed = stb__rand_seed * 2147001325 + 715136305;

	int ret = (result & RAND_MAX);

	//Journal::IO(ret);

	return ret;
}

void mysrand(int value)
{
	//Journal::IO(value);

	value ++;
	//srand(value + value * 67 + value * 1031);
	value ^= 0x31415926;
	stb__rand_seed = value + value * 67 + value * 1031;
	myrand();
	myrand();
	myrand();
	myrand();
}

END_CB
