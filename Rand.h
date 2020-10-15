#pragma once

#include "Base.h"

START_CB

//-----------------------------------------------------------

#define RAND_MAX_SHIFT	(15)
#define RAND_MAX_PLUS_ONE	(1<<RAND_MAX_SHIFT)

// in cpp :
//COMPILER_ASSERT( RAND_MAX == ((1<<RAND_MAX_SHIFT)-1) );

// you should Journal calls to rand, I don't do it internally

// rand() and srand() act just like stdlib
int my_c_rand(); // [0,RAND_MAX]

// better :
uint32 myrand32();

void mysrand(int value);
void mysrand_time();

// returns a value in [0,size-1]
inline uint32 irandmod(uint32 size)
{
	return (uint32)((myrand32() * (uint64)size) >> 32);
}

// irandranged is in [lo,hi] inclusive
inline int irandranged(int lo,int hi)
{
	return lo + irandmod(hi-lo+1);
}

// drandunit is in [0,1)
inline double drandunit()
{
	return myrand32() * (1.0/4294967296.0);
}

// frandunit is in [0,1)
/*
inline float frandunit()
{
	return myrand32() * (1.f/4294967296.f);
}
*/
inline float frandunit()
{
	return (float) drandunit();
}

inline double drandranged(double lo,double hi)
{
	return lo + (hi - lo)*drandunit();
}

inline float frandranged(float lo,float hi)
{
	return lo + (hi - lo)*frandunit();
}

inline bool randbool()
{
	//return !!irandmod(2);
	uint32 v32 = myrand32();
	// which bit to check is random :
	int bit = (v32>>28);
	// check that bit :
	return (v32>>bit)&1;
}

//! Return a (normal) Gaussian-distributed random number
//!		Average 0.0 , Standard-Dev 1.0	
//! Range is +/-3*Sqrt(2).
float	frandgaussian();

//-----------------------------------------------------------

class WeightedDecayRand
{
public:

	// redrawPenalty in [0,1] - 0 is a true rand, 1 forbids repeats
	// restoreSpeed is how much chance of redraw is restored per second or per draw
	explicit WeightedDecayRand(int num,float redrawPenalty,float restoreSpeed);
	~WeightedDecayRand();

	int Draw();
	void Tick(float dt);
	
	int DrawFixedDecay();
	int DrawTimeDecay(double curTime);

private:
	int		m_num;
	float * m_weights;	
	float	m_redrawPenalty;
	float	m_restoreSpeed;
	float	m_weightSum;
	double	m_lastTime;

	FORBID_CLASS_STANDARDS(WeightedDecayRand);
};

//-----------------------------------------------------------

END_CB
