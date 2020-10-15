#include "IKChain.h"
#include "Quat.h"
#include "QuatUtil.h"
#include "Vec2.h"

START_CB

IKChain::IKChain() : m_root(0,0,0), m_totalLength(0.f)
{
}
IKChain::~IKChain()
{
}

void IKChain::Build_AddSegment(const Vec3 & pos)
{
	m_chain.push_back();
	m_chain.back().pos = pos;
	m_chain.back().length = Distance(pos, GetPrevPos(m_chain.size32()-1) );
	m_totalLength += m_chain.back().length;
}

void IKChain::Build_Finish()
{
}

// return a vec by moving "pos" along "pos-base" to make a result that is "dist"
//	away from "base"
static const Vec3 ScaleDistanceTo(const Vec3 & base,const Vec3 & pos,const float dist)
{
	Vec3 delta = pos - base;
	float curLength = delta.Length();
	if ( curLength == 0.f )
	{
		// damn crap
		return pos;
	}
	delta *= dist / curLength;
	return pos + delta;
}

bool IKChain::GetPairConstrainedPos(Vec3 * pInto,const Vec3 & prevPos,const Segment & cur,const Segment & next) const
{
	// move cur.pos to be within cur.length of prevPos and next.length of next.pos
	// this is a sphere-sphere intersection problem
	Vec3 deltaNormal = next.pos - prevPos;
	const float deltaLength = deltaNormal.NormalizeSafe();
	if ( deltaLength == 0.f )
	{
		// degenerate! end are on top of each other !
		const float lenAvg = (cur.length + next.length)*0.5f;
		const Vec3 base = MakeAverage(prevPos,next.pos);
		*pInto = ScaleDistanceTo(base,cur.pos,lenAvg);
		return false;
	}
	else if ( deltaLength >= (cur.length + next.length) )
	{
		// too separated for any intersection
		float lerper = cur.length / (cur.length + next.length);
		*pInto = MakeLerp(prevPos,next.pos,lerper);
		return false;
	}
	else if ( deltaLength <= MAX(cur.length,next.length) - MIN(cur.length,next.length) )
	{
		// too close for any intersection
		// normal points from the larger sphere to the shorter
		const Vec3 base = MakeAverage(prevPos,next.pos);
		float lenAvg = (cur.length + next.length)*0.5f;

		if ( next.length > cur.length )
			lenAvg *= -1.f;

		*pInto = base + lenAvg * deltaNormal;
		return false;
	}
	else
	{
		// else we have a solution, good lord
		float invDeltaLength = 1.f/deltaLength;
		float x = (fsquare(cur.length) - fsquare(next.length) + fsquare(deltaLength))*0.5f*invDeltaLength;
		float t = x * invDeltaLength;
		// t need not be in [0,1] or anything
		Vec3 retOnAxis = MakeLerp(prevPos,next.pos,t);
		float ySqr = fsquare(cur.length) - x*x;
		ASSERT( ySqr >= 0.f );
		float y = sqrtf(ySqr);
		// go off axis in the same way cur.pos was
		Vec3 curDelta = cur.pos - prevPos;
		// remove normal component
		float dot = curDelta * deltaNormal;
		curDelta -= dot * deltaNormal;
		ASSERT( fiszero(curDelta * deltaNormal) );
		if ( curDelta.NormalizeSafe() == 0.f )
		{
			// crapper, must choose a random way to go off axis
			// @@ this could be better; use persistence
			do
			{
				SetRandomNormal(&curDelta);
				// remove normal component
				float dot = curDelta * deltaNormal;
				curDelta -= dot * deltaNormal;
				ASSERT( fiszero(curDelta * deltaNormal) );
			} while ( curDelta.NormalizeSafe() == 0.f );
		}
		*pInto = retOnAxis + curDelta * y;
		ASSERT( fequal( Distance(*pInto,prevPos),cur.length,0.005f) );
		ASSERT( fequal( Distance(*pInto,next.pos),next.length,0.005f) );
		return true;
	}
}

//=======================================================================================

bool IKChain::Apply(const Vec3 & newEndPos, const int numIterations)
{
	Vec3 newNormal = newEndPos - m_root;
	float newLength = newNormal.NormalizeSafe();

	bool doInitialRotAndScale = true;

	if ( newLength >= m_totalLength )
	{
		// no solution, stretch beyond my capacity
		// just space them at length along normal
		for(int i=0;i<m_chain.size32();i++)
		{
			m_chain[i].pos = GetPrevPos(i) + m_chain[i].length * newNormal;
		}
		return false;
	}
	else if ( newLength == 0.f )
	{
		// special case for putting the end on
		//	top of the root
		doInitialRotAndScale = false;
	}

	const Vec3 & oldEndPos = GetEndPos();
	Vec3 oldNormal = oldEndPos - m_root;
	float oldLength = oldNormal.NormalizeSafe();

	if ( oldLength == 0.f )
	{
		// special case for taking the end from on
		//	top of the root
		doInitialRotAndScale = false;
	}

	if ( doInitialRotAndScale )
	{
		// rotate and stretch oldEndPos to get to newEndPos

		Quat q;
		SetRotationArc(&q,oldNormal,newNormal);

		// @@ heuristic for perpScale ?
		float paraScale = newLength / oldLength;
		float perpScale = oldLength / newLength;

		for(int i=0;i<m_chain.size32();i++)
		{
			Vec3 delta = m_chain[i].pos - m_root;
			// rotate delta by q :
			Vec3 rotDelta = q.Rotate( delta );
			// now apply perp and parallel scale
			float paraDot = rotDelta * newNormal;
			rotDelta *= perpScale;
			float desiredParaDot = paraDot * paraScale;
			float curParaDot = paraDot * perpScale;
			float addNormal = desiredParaDot - curParaDot;
			rotDelta += addNormal * newNormal;
			m_chain[i].pos = m_root + rotDelta;
		}
	}

	// accuracy is poor ?
	//ASSERT( Vec3::Equals(GetEndPos(),newEndPos) );
	m_chain.back().pos = newEndPos;

	// now apply the length constraints pairwise
	// with temperature easing in
	
	bool allOk = true;

	ASSERT( numIterations > 0 );
	for(int i=0;i<numIterations;i++)
	{
		// temperature goes up from low to 1.f
		// @@ linearly or exponentially?
		float temperature = float(i+1)/(numIterations);
	
		// apply 2-segment length constraints
		// @@ from root, or from end ?
		allOk = true;
		for(int l=0;l<m_chain.size32()-1;l++)
		{
			Vec3 constrainedPos; 
			bool ok = GetPairConstrainedPos(&constrainedPos,GetPrevPos(l),m_chain[l],m_chain[l+1]);
			
			if ( ! ok )
				allOk = false;
			// lerp towards target with temperature
			if ( i == numIterations-1 )
				m_chain[l].pos = constrainedPos;
			else
				m_chain[l].pos.SetLerp( m_chain[l].pos, constrainedPos, temperature );
		}
	}

	return allOk;
}

//=======================================================================================

void IKChain::GetVectors(vector<Vec3> * pVectors) const
{
	pVectors->resize(m_chain.size());
	for(int i=0;i<m_chain.size32();i++)
	{
		pVectors->at(i) = GetLinkPos(i);
	}
}

float IKChain::MeasureEnergy(const vector<Vec3> & restPos,const int start,const int count) const
{
	float energy = 0.f;
	for(int i=start;i<(start+count) && i<m_chain.size32();i++)
	{
		const Vec3 & v = GetLinkPos(i);

		energy += DistanceSqr(v,restPos[i]); // @@ resistant term at each joint
	}
	return energy;
}

void IKChain::Relax(const vector<Vec3> & restVectors,const int maxIterations,const float minTemperature)
{
	static const Vec2 c_directions[4] =
	{
		Vec2::unitX,
		Vec2::unitXneg,
		Vec2::unitY,
		Vec2::unitYneg
	};

	ASSERT( maxIterations > 0 );
	int iteration=0;
	float temperature = 0.5f;
	for(;;)
	{
		bool improvedEnergy = false;
		for(int i=0;i<m_chain.size32()-2;i++)
		{
			// try moving link pos "i" to reduce its energy
			// then apply the pair-constraint to "i+1"
			// see if it helped 
			const Vec3 & prevPos = GetPrevPos(i);
			const Vec3 pos1 = GetLinkPos(i);
			const Vec3 pos2 = GetLinkPos(i+1);
			//const Vec3 & pos3 = GetLinkPos(i+2);

			float avgLength = (m_chain[i].length + m_chain[i+1].length)*0.5f;
			float step = avgLength * temperature;

			float bestEnergy = MeasureEnergy(restVectors,i,4);
			Vec3 bestPos1 = pos1;
			Vec3 bestPos2 = pos2;

			Vec3 delta1 = pos1 - prevPos;
			float delta1Len = m_chain[i].length;
			Vec3 perp1,perp2;
			GetTwoPerpNormals(delta1,&perp1,&perp2);

			for(int d=0;d<4;d++)
			{
				// pivot pos1 so that it is still the right length from its parent
				Vec3 delta = delta1 + (step * c_directions[d].x) * perp1 + (step * c_directions[d].y) * perp2;
				delta *= delta1Len / delta.Length();
				m_chain[i].pos = prevPos + delta;
				
				if ( GetPairConstrainedPos(&m_chain[i+1].pos,pos1,m_chain[i+1],m_chain[i+2]) )
				{
					// might be ok
					// measure energy ; moving 2 positions affects 3 angles
					float energy = MeasureEnergy(restVectors,i,4);
					if ( energy < bestEnergy - EPSILON )
					{
						bestPos1 = GetLinkPos(i);
						bestPos2 = GetLinkPos(i+1);
						improvedEnergy = true;
					}
				}
			}
			
			// move them to the best positions :
			m_chain[i].pos = bestPos1;
			m_chain[i+1].pos = bestPos2;
		}

		if ( ! improvedEnergy )
		{
			temperature *= 0.75f;
		}
		iteration++;
		if ( iteration > maxIterations )
			break;
		if ( temperature < minTemperature )
			break;
	}
}

//=======================================================================================

END_CB
