#include "Rand.h"
#include "Timer.h"
//#include "Journal.h"
#include <stdlib.h>
#include "FloatUtil.h"

START_CB

COMPILER_ASSERT( (RAND_MAX+1) == (RAND_MAX_PLUS_ONE) );

static uint32 stb__rand_seed = 0;

static uint32 stb_rand32()
{
	// shuffle non-random bits to the middle, and xor to decorrelate with seed
	uint32 result = 0x31415926 ^
		((stb__rand_seed >> 16) + (stb__rand_seed << 16));
	stb__rand_seed = stb__rand_seed * 2147001325 + 715136305;
	return result;
}

static int stb_rand()
{
	// shuffle non-random bits to the middle, and xor to decorrelate with seed
	uint32 result = stb_rand32();
	
	int ret = (result & RAND_MAX);

	//Journal::IO(ret);

	return ret;
}

static void stb_srand(int value)
{
	//Journal::IO(value);

	value ++;
	//srand(value + value * 67 + value * 1031);
	value ^= 0x31415926;
	stb__rand_seed = value + value * 67 + value * 1031;
	stb_rand();
	stb_rand();
	stb_rand();
	stb_rand();
}

/*

Marsaglia's KISS32 and KISS99



*/

static uint32 x = 123456789,y = 362436000,z = 521288629,c = 7654321; // seeds
static uint32 w = 14921776; 

static void kiss_srand(int value)
{
	stb_srand(value);
	x = stb_rand32();
	stb_rand();
	y = stb_rand32();
	stb_rand();
	z = stb_rand32();
	stb_rand();
	c = stb_rand32();
	stb_rand();
	w = stb_rand32();
}

uint32 KISS99()
{
	//static const uint64 a = 698769069ULL;  
	x = 69069*x+12345;  
	
	y ^= (y<<13);
	y ^= (y>>17); 
	y ^= (y<<5);  
	
	//uint64 t = a*z+c; 
	uint64 t = c;
	t += (698769069ULL*z);
	c = (uint32)(t>>32); 
	z = (uint32)t;
	
	return (x+y+z);
}

/*
uint32 KISS32()
{
	x += 545925293;  

	y ^= (y<<13); 
	y ^= (y>>17); 
	y ^= (y<<5);  
	
	uint32 t = z+w+c; 
	z = w; 
	c = (t>>31); 
	w = t & 0x7FFFFFFF;  
	
	return (x+y+w); 
}
*/

uint32 myrand32()
{
	return KISS99();	
}

int my_c_rand()
{
	uint32 kiss = myrand32();
	
	return ((kiss>>16) + kiss)&RAND_MAX;
}

void mysrand(int value)
{
	kiss_srand(value);
	
	myrand32();
	myrand32();
	myrand32();
	myrand32();
	myrand32();
}

// reversible mixer from BurtleBurtle :
#define mix3(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

void mysrand_time()
{
	//kiss_srand( 2166136261 );
	
	// default seeds
	x = 123456789;
	y = 362436000;
	z = 521288629;
	c = 7654321;
	w = 14921776; 

	// use three different timers to get different bits
	
	uint64 tsc = Timer::rdtsc() ^ 545925293;
	uint64 qpc = Timer::RawQPC() ^ 698769069ULL;
	uint64 mil = Timer::GetMillis32() * 69069 + 12345;
	
	mix3(tsc,qpc,mil);
	
	x ^= tsc;
	y ^= qpc;
	z ^= mil;
	c ^= (tsc>>32);
	w ^= (qpc>>32);
	
	myrand32();
	myrand32();
	myrand32();
	myrand32();
	myrand32();
}


//! Return a (normal) Gaussian-distributed random number
//!		Average 0.0 , Standard-Dev 1.0	
//! Range is +/-3*Sqrt(2).
float	frandgaussian()
{
	// larger count for better approximation
	//	smaller count for more speed
	enum { c_count = 6 };
	
	float sum = 0.f;

	for(int i = 0;i<c_count;i++)
	{
		sum += frandunit();
	}

	// subtract off the mean
	static const float c_mean = c_count * 0.5f;

	// divide by the s-dev
	static const float c_normalizer = 1.f / sqrtf( c_count / 12.f );
	
	return (sum - c_mean) * c_normalizer;
}

//-----------------------------------------------------------

WeightedDecayRand::WeightedDecayRand(int num,float redrawProbability,float restoreSpeed) :
	m_num(num),
	m_redrawPenalty(redrawProbability),
	m_restoreSpeed(restoreSpeed)
{
	m_weights = new float [num];
	for(int i=0;i<num;i++)
	{
		m_weights[i] = 1.f;
	}
	m_weightSum = (float) num;
}

WeightedDecayRand::~WeightedDecayRand()
{
	delete [] m_weights;
}

int WeightedDecayRand::Draw()
{
	float p = frandunit() * m_weightSum;
	for(int i=0;;i++)
	{
		if ( i == m_num-1 || p < m_weights[i] )
		{
			// accepted !
			m_weightSum -= m_weights[i];
			m_weights[i] -= m_redrawPenalty;
			m_weights[i] = MAX(m_weights[i],0.f);
			m_weightSum += m_weights[i];
			return i;
		}
		else
		{
			p -= m_weights[i];
		}
	}
}

void WeightedDecayRand::Tick(float dt)
{
	m_weightSum = 0;
	for(int i=0;i<m_num;i++)
	{
		if ( m_weights[i] < 1.f )
		{
			m_weights[i] += dt * m_restoreSpeed;
			m_weights[i] = MIN(m_weights[i],1.f);
		}
		m_weightSum += m_weights[i];
	}
}

int WeightedDecayRand::DrawFixedDecay()
{
	int ret = Draw();
	
	Tick( 1.f );
	
	return ret;
};

int WeightedDecayRand::DrawTimeDecay(double curTime)
{
	if ( curTime != m_lastTime )
	{
		Tick( (float)(curTime - m_lastTime) );
		m_lastTime = curTime;
	}

	int ret = Draw();
		
	return ret;
};


//-----------------------------------------------------------


END_CB
