#pragma once

#include "cblib/Vec3.h"
#include "cblib/Vec3U.h"
#include "cblib/IKChain.h"
#include "cblib/vector.h"

START_CB

class IKChain
{
public :
	 IKChain();
	~IKChain();

	//-------------------------------------------------------------

	void Build_SetRoot(const Vec3 & root)			{ m_root = root; }
	void Build_AddSegment(const Vec3 & pos);
	void Build_Finish();

	//-------------------------------------------------------------

	const Vec3 &		GetRootPos() const		{ return m_root; }
	const Vec3 &		GetEndPos() const		{ return m_chain.back().pos; }
	int					GetNumLinks() const		{ return m_chain.size32(); }
	const Vec3 &		GetLinkPos(const int i) const { return m_chain[i].pos; }
	const Vec3 &		GetPrevPos(const int i) const
	{
		if ( i == 0 ) return m_root;
		else return m_chain[i-1].pos;
	}

	//-------------------------------------------------------------

	// numIterations of 1 is often plenty
	bool Apply(const Vec3 & newEndPos, const int numIterations);

	//-------------------------------------------------------------

	void GetVectors(vector<Vec3> * pVectors) const;

	void Relax(const vector<Vec3> & restVectors,const int maxIterations,const float minTemperature = 0.01f);

	//-------------------------------------------------------------

private:
	Vec3				m_root;
	struct Segment
	{
		float	length;
		Vec3	pos;
	};
	vector<Segment>	m_chain;
	float				m_totalLength;
	
	bool GetPairConstrainedPos(Vec3 * pInto,const Vec3 & prevPos,const Segment & cur,const Segment & next) const;
	float MeasureEnergy(const vector<Vec3> & restVectors,const int start,const int count) const;
};

END_CB
