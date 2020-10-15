#include "LinearSpline.h"
#include "BezierSpline.h"
#include "Vec2U.h"
#include "Vec3U.h"
#include "vector.h"

START_CB

LinearSpline::LinearSpline()
{
	m_type = eInvalid;
}

LinearSpline::PlaceOnSpline::PlaceOnSpline()
{
	//Some invalid data. Not okay to juse 'use' these. You have to fill them out. 
	segment = -1;
	localTime = -1.f;
}


bool LinearSpline::IsValid() const
{
#if 0 // def DO_ASSERTS
	for (int i = 0; i < m_knots.size(); i++)
	{
		ASSERT(m_knots[i].IsValid());
	}
#endif
	ASSERT(m_bbox.IsValid());
	ASSERT(m_type == eGeneric ||
		m_type == eUniformTime ||
		m_type == eUniformLength);

	return true;
}


/*
	SetFromPoints takes the points and uses them as knots
	scales durations based on distances of knots
*/
void LinearSpline::SetFromPoints(const Vec3 * points,const int numPoints)
{
	m_knots.clear();
	m_knots.resize(numPoints);

	m_knots[0].accumLength = 0.f;

	for(int i=0;i<numPoints;i++)
	{
		m_knots[i].point = points[i];
		
		if ( i > 0 )
		{
			m_knots[i].accumLength = m_knots[i-1].accumLength +
					Distance(m_knots[i].point,m_knots[i-1].point);
		}
	}

	float totLen = GetTotalLength();
	float scale = 1.f / totLen;
	
	for(int i=0;i<numPoints;i++)
	{
		m_knots[i].accumTime = m_knots[i].accumLength * scale;
	}

	m_type = eGeneric;
	UpdateDerived();
}

void LinearSpline::SetUniformTimeSampled(const BezierSpline & from,const int numKnots)
{
	ASSERT( numKnots >= 2 );

	// make curves from knots :
	m_knots.clear();
	m_knots.resize(numKnots);

	float timePerSeg = 1.f / (numKnots-1);
	
	for(int i=0;i<numKnots;i++)
	{
		float time = i * timePerSeg;
		
		BezierSpline::PlaceOnSpline place;
		from.GetPlaceByTime(time,&place);

		const Vec3 P = from.GetValue(place);

		m_knots[i].point = P;
		m_knots[i].accumTime = time;
	}

	m_type = eUniformTime;
	UpdateDerived();
}

void LinearSpline::UpdateDerived()
{
	ASSERT( m_type != eInvalid );
	
	// force last time to be exactly 1.0
	ASSERT( fisone(m_knots.back().accumTime) );
	m_knots.back().accumTime = 1.f;

	// update bbox and accumLength :
	m_knots[0].accumLength = 0.f;
	m_bbox.SetToPoint(m_knots[0].point);
	for(int i=1;i<m_knots.size32();i++)
	{
		m_knots[i].accumLength = m_knots[i-1].accumLength +
					Distance(m_knots[i].point,m_knots[i-1].point);
		m_bbox.ExtendToPoint(m_knots[i].point);
	}
}

float LinearSpline::GetTimeFromPlace(const PlaceOnSpline & place) const
{
	float t0,t1;
	GetSegmentTimeRange(place.segment,&t0,&t1);
	return flerp(t0,t1,place.localTime);
}

float LinearSpline::GetLengthFromPlace(const PlaceOnSpline & place) const
{
	float t0,t1;
	GetSegmentLengthRange(place.segment,&t0,&t1);
	return flerp(t0,t1,place.localTime);
}

void LinearSpline::GetSegmentTimeRange(	int segment,float *pStart,float *pEnd) const
{
	ASSERT( segment >= 0 && segment < GetNumSegments() );
	*pStart = m_knots[segment].accumTime;
	*pEnd = m_knots[segment+1].accumTime;
}

void LinearSpline::GetSegmentLengthRange(	int segment,float *pStart,float *pEnd) const
{
	ASSERT( segment >= 0 && segment < GetNumSegments() );
	*pStart = m_knots[segment].accumLength;
	*pEnd = m_knots[segment+1].accumLength;
}

void LinearSpline::GetPlaceByTime(const float t,PlaceOnSpline * pInto) const
{
	ASSERT( fisinrange(t,0.f,1.f) );
	
	if ( m_knots.empty() )
	{
		pInto->segment = 0;
		pInto->localTime = 0;
		return;
	}

	ASSERT( m_knots.back().accumTime == 1.f );

	if ( m_type == eUniformTime )
	{
		float v = t * GetNumSegments();
		pInto->segment = ftoi(v);
		pInto->localTime = v - pInto->segment;
		
		#ifdef DO_ASSERTS
		float start,end;
		GetSegmentTimeRange(pInto->segment,&start,&end);
		ASSERT( fisinrange(t,start,end) );
		#endif
	}
	else
	{
		// @@@@ TODO : could binary search on t for more speed
		int seg = 0;
		while ( t > m_knots[seg].accumTime )
		{
			seg++;
		}

		// I'm in "seg"
		float start,end;
		GetSegmentTimeRange(seg,&start,&end);
		ASSERT( fisinrange(t,start,end) );
		pInto->segment = seg;
		pInto->localTime = fmakelerpernoclamp(start,end,t);
	}
}

void LinearSpline::GetPlaceByLength(const float _len,PlaceOnSpline * pInto) const
{
	const float len = fclamp(_len,0.f,GetTotalLength());
	
	if ( m_knots.empty() )
	{
		pInto->segment = 0;
		pInto->localTime = 0;
		return;
	}

	if ( m_type == eUniformLength )
	{
		float v = len * GetNumSegments() / GetTotalLength();
		pInto->segment = ftoi(v);
		
		float start,end;
		GetSegmentLengthRange(pInto->segment,&start,&end);
		ASSERT( fisinrange(len,start,end) );

		pInto->localTime = fmakelerpernoclamp(start,end,len);
	}
	else
	{
		// @@@@ TODO : could binary search on len for more speed
		int seg = 0;
		while ( seg < GetNumSegments() && len > m_knots[seg+1].accumLength )
		{
			seg++;
		}

		// I'm in "seg"
		float start,end;
		GetSegmentLengthRange(seg,&start,&end);
		ASSERT( fisinrange(len,start,end) );
		pInto->segment = seg;
		pInto->localTime = fmakelerpernoclamp(start,end,len);
	}
}

const Vec3 LinearSpline::GetValue(const PlaceOnSpline & place) const
{
	ASSERT( place.segment >= 0 && place.segment < GetNumSegments() );
	return MakeLerp( m_knots[place.segment].point, m_knots[place.segment+1].point, place.localTime );
}

const Vec3 LinearSpline::GetTangent(const PlaceOnSpline & place) const
{
	ASSERT( place.segment >= 0 && place.segment < GetNumSegments() );
	return m_knots[place.segment+1].point - m_knots[place.segment].point;
}

const Vec3 LinearSpline::GetTangentNormal(const PlaceOnSpline & place) const
{
	return cb::MakeNormalizedFast( GetTangent(place) );
}

void LinearSpline::ComputeClosestPoint(const Vec3 & query,float * pDistSqr, PlaceOnSpline * pPlace) const
{
	ASSERT( pPlace != NULL );
	// seed the search :
	float curBestDSqr = FLT_MAX;

	for(int i=0;i<(m_knots.size32()-1);i++)
	{
		float dSqr,t;
		ComputeClosestPointToEdge(query, m_knots[i].point,m_knots[i+1].point, &dSqr,&t);

		if ( dSqr < curBestDSqr )
		{
			curBestDSqr = dSqr;
			pPlace->segment = i;
			pPlace->localTime = t;
		}
	}

	ASSERT( curBestDSqr != FLT_MAX );

	*pDistSqr = curBestDSqr;
}

END_CB
