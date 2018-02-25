#include "cblib/Base.h"

START_CB

//-----------------------------------------------------------

#define RAND_MAX_SHIFT	(15)
#define RAND_MAX_PLUS_ONE	(1<<RAND_MAX_SHIFT)

//COMPILER_ASSERT( RAND_MAX == ((1<<RAND_MAX_SHIFT)-1) );

// you should Journal calls to rand, I don't do it internally

int myrand();
void mysrand(int value);

// returns a value in [0,size-1]
inline int irand(int size)
{
	return (myrand() * size) >> RAND_MAX_SHIFT;
}

// irandranged is in [lo,hi] inclusive
inline int irandranged(int lo,int hi)
{
	return lo + irand(hi-lo+1);
}

// frandunit is in [0,1)
inline float frandunit()
{
	return myrand() * (1.f/RAND_MAX_PLUS_ONE);
}
// drandunit is in [0,1)
inline double drandunit()
{
	return myrand() * (1.0/RAND_MAX_PLUS_ONE);
}
inline bool brand()
{
	return !!irand(2);
}

inline float frand(float lo,float hi)
{
	return lo + (hi - lo)*frandunit();
}

//-----------------------------------------------------------

END_CB
