#include "cblib/Base.h"
#include "cblib/SemiRandomGenerator.h"
#include "cblib/Timer.h"
#include "cblib/FloatUtil.h"
#include "cblib/Rand.h"

START_CB

/**

SemiRandomGenerator

generates numbers in the range [0,count-1]

Has state and can generate with nice non-repetition properties.
Weight-based.  A full weight is "1.0"

"selectionPenalty" is used to decrease the chance of repeating a value.
	if selectionPenalty is >= 1.0 , then immediate repetition is forbidden
		for selectionPenalty > 1.0 , you forbid repetition for some time after

"restoreSpeed" determines how fast old weights become available again
	restoreSpeed >= 1.0 means all is forgotten within one second
	if the Generator is NOT timeBased, then each time you GetValue, restoreSpeed is
		added to each weight, so you generally want 
		restoreSpeed < selectionPenalty

"minDT" is used only for time-based Generators
	it makes sure the weights are ticked at least a little after each GetValue()

**/

SemiRandomGenerator::SemiRandomGenerator() : m_weights(NULL), m_count(0)
{
}

SemiRandomGenerator::~SemiRandomGenerator()
{
	Clear();
}

void SemiRandomGenerator::Clear()
{
	if ( m_weights )
	{
		delete [] m_weights;
		m_weights = NULL;
	}
}

void SemiRandomGenerator::Init(int count)
{
	Clear();
	
	ASSERT( count >= 0 );
	
	m_count = count;
	if ( count > 1 )
		m_weights = new float[count];
	else
		m_weights = NULL;
	m_lastTime = Timer::GetSeconds();

	ResetWeights();
}

int SemiRandomGenerator::GetValue(const SemiRandomGeneratorParams & params)
{
	if ( m_count < 2 )
		return 0;
		
	ASSERT( m_weights != NULL );

	if ( params.m_timeBased )
	{
		// run the clock up to the present
		double t = Timer::GetSeconds();
		float dt = (float)( t - m_lastTime );
		m_lastTime = t;
		dt = Clamp( dt, params.m_minDT, params.m_maxDT );

		TickWeights( dt,params );
	}
	else
	{
		TickWeights(1.f,params);
	}

	// pick a value using weighted probabilities :
	float sum = 0.f;
	for(int i=0;i<m_count;i++)
	{
		sum += MAX(0.f,m_weights[i]);
	}
	if ( sum == 0.f )
	{
		// possible when all values have been recently used, making the weights go negative :
		// boost weights up :
		float maxW = - FLT_MAX;
		int iMaxW = 0;
		for(int i=0;i<m_count;i++)
		{
			float w = m_weights[i];
			if ( w > maxW )
			{
				maxW = w;
				iMaxW = i;
			}
		}
		ASSERT( maxW <= 0.f );
		float boost = 1.f - maxW;
		sum = 0.f;
		for(int i=0;i<m_count;i++)
		{
			m_weights[i] += boost;
			sum += MAX(0.f,m_weights[i]);
		}
		ASSERT( sum > 0.f );
	}

	float p = frandunit() * sum;

	float accum = 0.f;
	for(int i=0;i<m_count;i++)
	{
		accum += MAX(0.f,m_weights[i]);
		if ( p <= accum || i == (m_count-1) )
		{
			// that's me
			m_weights[i] -= params.m_selectionPenalty;
			// let it go negative
			return i;
		}
	}
	
	// should not get here
	FAIL("should not get here");
	return -1;
}

void SemiRandomGenerator::ResetWeights()
{
	if ( m_weights == NULL )
		return;
	// all weights start at 1.0 :
	for(int i=0;i<m_count;i++)
	{
		m_weights[i] = 1.f;
	}
}

void SemiRandomGenerator::TickWeights(float dt,const SemiRandomGeneratorParams & params)
{
	if ( m_weights == NULL )
		return;
	// restore weights
	
	for(int i=0;i<m_count;i++)
	{
		// @@ DampedDrive or LerpedDrive ?
		//m_weights[i] = FloatUtil::LerpedDrive(m_weights[i],1.f, m_restoreSpeed, dt);
		// let weights go over 1.0 :
		m_weights[i] += dt * params.m_restoreSpeed;
	}
}

END_CB
