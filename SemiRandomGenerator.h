#pragma once

#include "Base.h"

START_CB

/**

SemiRandomGenerator

generates numbers in the range [0,count-1]

see notes in .cpp

**/

struct SemiRandomGeneratorParams
{
	float	m_restoreSpeed;
	float	m_selectionPenalty;
	float	m_minDT;
	float	m_maxDT;
	bool	m_timeBased;
	
	SemiRandomGeneratorParams() :
		m_restoreSpeed(0.1f),
		m_selectionPenalty(0.8f),
		m_minDT(1.f),
		m_maxDT(10.f),
		m_timeBased(true)
	{
	}
	
	void Set( float selectionPenalty, float restoreSpeed, float minDT, float maxDT, bool timeBased )
	{
		m_restoreSpeed = restoreSpeed;
		m_selectionPenalty = selectionPenalty;
		m_minDT = minDT;
		m_maxDT = maxDT;
		m_timeBased = timeBased;
	}
};

class SemiRandomGenerator
{
public:
	SemiRandomGenerator();
	~SemiRandomGenerator();

	void Init( int count );

	int GetValue(const SemiRandomGeneratorParams & params);

private:
	void Clear();
	
	void TickWeights(float dt,const SemiRandomGeneratorParams & params);
	void ResetWeights();

	int		m_count;
	float * m_weights;
	double	m_lastTime;

	FORBID_ASSIGNMENT(SemiRandomGenerator);
	FORBID_COPY_CTOR(SemiRandomGenerator);
};

END_CB
