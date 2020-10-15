#include "BezierSpline.h"
#include "Vec2U.h"
#include "Log.h"
#include <malloc.h> // for alloca

#pragma warning(disable :4701) // the darn STL

START_CB

/****

Some TODOs :

1. I frequently resample a spline to linear time knots; in that case, GetPlaceByTime() is just
	a multiply; maybe I should store it as a flag ?

---------------

Note : the construction from points is just Catmull-Rom
that is, I use the centered-derivative piece connection method

eg. this is not a "B-spline" ; though it could be with just a 
different SetFromPoints 

****/

//===============================================================================================

BezierSpline::BezierSpline() : m_type(eInvalid)
{
}

BezierSpline::~BezierSpline()
{
}

bool BezierSpline::IsValid() const
{
	#if 0 // DO_ASSERTS
	//if ( g_eAssertLevel == ASSERT_Safe )
	{ 
		for (int i = 0; i < m_pieces.size(); i++)
		{
			//ASSERT(m_pieces[i].curve.IsValid());
			ASSERT(fisvalid(m_pieces[i].accumTime) && m_pieces[i].accumTime >= 0);
			ASSERT(m_pieces[i].sphere.IsValid());
			ASSERT(fisvalid(m_pieces[i].accumLength) && m_pieces[i].accumLength >= 0);
		}
	}
	#endif
	ASSERT(m_bbox.IsValid());
	ASSERT(m_type == eGeneric ||
		   m_type == eUniformTime ||
		   m_type == eUniformLength);

	return true;
}

static int Index(const int i, const int size, const BezierSpline::EEndHandling eEnds)
{
	if ( i < 0 )
	{
		if ( eEnds == BezierSpline::eEnd_Clamp )
			return 0;
		else // Wrap
			return size + i;
	}
	else if ( i >= size )
	{
		if ( eEnds == BezierSpline::eEnd_Clamp )
			return size-1;
		else // Wrap
			return i - size;
	}
	else
	{
		return i;
	}
}

void BezierSpline::SetFromPoints(const Vec3 * pKnots,const int numKnots,const EEndHandling eEnds)
{
	int numCurves = numKnots-1;
	ASSERT( numCurves > 0 );

	// make curves from knots :
	m_pieces.clear();
	m_pieces.resize(numCurves);
	
	for(int i=0;i<numCurves;i++)
	{
		// P0 = pKnots[i];
		// P1 = pKnots[i+1];
		// D0 = (pKnots[i+1]-pKnots[i-1])/6
		// D1 = (pKnots[i+2]-pKnots[i])/6

		Vec3 D0 = ( pKnots[i+1] - pKnots[Index(i-1,numKnots,eEnds)] )/2.f;
		Vec3 D1 = ( pKnots[Index(i+2,numKnots,eEnds)] - pKnots[i] )/2.f;

		m_pieces[i].curve.SetFromEnds(pKnots[i],D0, pKnots[i+1],D1);
		m_pieces[i].accumTime = (i+1) / float(numCurves);
	}

	m_type = eUniformTime;

	UpdateDerived();
	// do NOT ParameterizeByLength, need even time params to match derivatives
}

//! SetFromKnots is nearly SetFromPoints, but the time parameter is provided
void BezierSpline::SetFromKnots(const Knot * pKnots,const int numKnots,const EEndHandling eEnds)
{
	int numCurves = numKnots-1;
	ASSERT( numCurves > 0 );

	// make curves from knots :
	m_pieces.clear();
	m_pieces.resize(numCurves);
	
	ASSERT( pKnots[0].time == 0.f );
	ASSERT( pKnots[numKnots-1].time == 1.f );

	for(int i=0;i<numCurves;i++)
	{
		// P0 = pKnots[i];
		// P1 = pKnots[i+1];
		// D0 = (pKnots[i+1]-pKnots[i-1])/6
		// D1 = (pKnots[i+2]-pKnots[i])/6

		Vec3 D0 = ( pKnots[i+1].point - pKnots[Index(i-1,numKnots,eEnds)].point )/2.f;
		Vec3 D1 = ( pKnots[Index(i+2,numKnots,eEnds)].point - pKnots[i].point )/2.f;

		float timeScale = (pKnots[i+1].time - pKnots[i].time) / float(numCurves);
		// if time was even, timeScale would be 1.0
		// if timeScale is > 1.0 it means we move slower on this knot than we would think,
		//	so then we have to scale up D to match the neighbor
		D0 *= timeScale;
		D1 *= timeScale;

		m_pieces[i].curve.SetFromEnds(pKnots[i].point,D0, pKnots[i+1].point,D1);
		m_pieces[i].accumTime = pKnots[i+1].time;
	}
	
	m_type = eGeneric;

	UpdateDerived();
}

//! Sample another spline at equi-distant spots to make a new spline with
//	equal arc-length spaced knots 
void BezierSpline::SetUniformTimeSampled(const BezierSpline & from,const int numKnots)
{
	int numCurves = numKnots-1;
	ASSERT( numCurves > 0 );

	float timePerSeg = 1.f / float(numCurves);
	
	// make curves from knots :
	m_pieces.clear();
	m_pieces.resize(numCurves);
	
	Vec3 P[2];
	Vec3 D[2];
	int index=1;
	P[0] = from.GetValue(0.f);
	D[0] = from.GetDerivativeGlobal(0.f);

	for(int i=0;i<numCurves;i++)
	{
		float t = (i+1) * timePerSeg;
		if ( i == (numCurves-1) )
			t = 1.f;
		PlaceOnSpline place;
		from.GetPlaceByTime(t,&place);

		P[index] = from.GetValue(place);
		D[index] = from.GetDerivativeGlobal(place);

		const Vec3 & P0 = P[index^1];
		const Vec3 & P1 = P[index];

		const Vec3 D0 = D[index^1] * timePerSeg;
		const Vec3 D1 = D[index] * timePerSeg;

		m_pieces[i].curve.SetFromEnds(P0,D0, P1,D1);
		m_pieces[i].accumTime = t;

		index ^=1;
	}

	m_type = eUniformTime;

	UpdateDerived();
}


//! Sample another spline at equi-distant spots to make a new spline with
//	equal arc-length spaced knots 
void BezierSpline::SetUniformDistanceSampled(const BezierSpline & from,const int numKnots)
{
	int numCurves = numKnots-1;
	ASSERT( numCurves > 0 );

	float totLen = from.GetTotalLength();
	float lenPerSeg = totLen / numCurves;
	float timePerSeg = 1.f / float(numCurves);
	
	// make curves from knots :
	m_pieces.clear();
	m_pieces.resize(numCurves);
	
	Vec3 P[2];
	Vec3 D[2];
	int index=1;
	P[0] = from.GetValue(0.f);
	D[0] = from.GetDerivativeGlobal(0.f);

	for(int i=0;i<numCurves;i++)
	{
		if ( i == numCurves-1 )
		{
			P[index] = from.GetValue(1.f);
			D[index] = from.GetDerivativeGlobal(1.f);
		}
		else
		{
			float accumLen = (i+1) * lenPerSeg;
		
			PlaceOnSpline place;
			from.GetPlaceByLengthAccurate(accumLen,&place);

			P[index] = from.GetValue(place);
			D[index] = from.GetDerivativeGlobal(place);
		}
		
		const Vec3 & P0 = P[index^1];
		const Vec3 & P1 = P[index];

		const Vec3 D0 = D[index^1] * timePerSeg;
		const Vec3 D1 = D[index] * timePerSeg;

		m_pieces[i].curve.SetFromEnds(P0,D0, P1,D1);
		m_pieces[i].accumTime = (i+1) * timePerSeg;

		index ^=1;
	}

	m_pieces[numCurves-1].accumTime = 1.f;
	
	m_type = eUniformLength;

	UpdateDerived();
}


void BezierSpline::SetSubdivisionSampled_RecursiveAdd( const Bezier & curve, const float t0, const float t1, const float maxErrSqr)
{
	if ( curve.ComputeLinearErrorBoundSqr() <= maxErrSqr )
	{
		 // ok, add it
		ASSERT( m_pieces.size() > 0 || t0 == 0.f );
		ASSERT( m_pieces.size() == 0 || m_pieces.back().accumTime == t0 );
		m_pieces.push_back();
		m_pieces.back().curve = curve;
		m_pieces.back().accumTime = t1;
	}
	else
	{
		// must subdivide
		Bezier b0,b1;
		curve.Subdivide(&b0,&b1);
		float tmid = (t0 + t1)*0.5f;

		SetSubdivisionSampled_RecursiveAdd(b0, t0,tmid, maxErrSqr );
		SetSubdivisionSampled_RecursiveAdd(b1, tmid,t1, maxErrSqr );
	}
}

void BezierSpline::SetSubdivisionSampled(const BezierSpline & from,const float maxErrSqr)
{
	m_pieces.clear();
	
	for(int i=0;i<from.GetNumSegments();i++)
	{
		float start,end;
		from.GetSegmentTimeRange(i,&start,&end);
		SetSubdivisionSampled_RecursiveAdd( from.m_pieces[i].curve, start,end, maxErrSqr);
	}
	
	m_type = eGeneric;

	UpdateDerived();
}

struct PieceToSplit
{
	float midTime; // refer to a piece by its midTime, since indexing is changing
	float errSqr;
	
	bool operator < (const PieceToSplit & rhs) const
	{
		return errSqr < rhs.errSqr;
	}
};

static double NumericalErrSqr(const BezierSpline &s1,const BezierSpline & s2,float start,float end,int numSteps)
{
	double errSqr = 0;
	
	// for two bezier curves we could integrate the exact error
	// but on the two splines the sampling might not be the same so we have to deal with various overlapping intervals
	//	definitely possible, but I'm lazy so just do some step samples :
	
	for(int i=0;i<numSteps;i++)
	{
		// no need to sample right at the ends, so offset in :
		float t = flerp(start,end,(float)(i+1)/(numSteps+1));
	
		Vec3 v1 = s1.GetValue(t);
		Vec3 v2 = s2.GetValue(t);
		
		errSqr += DistanceSqr(v1,v2);
	}
	
	// return err per step :
	errSqr /= numSteps;
	
	return errSqr;
}

static void InitPieceToSplit(PieceToSplit * pts,const BezierSpline & me,const BezierSpline & other,int piece)
{
	float start,end;
	me.GetSegmentTimeRange(piece,&start,&end);
	
	pts->midTime = faverage(start,end);
	
	// take a # of steps proportional to the time range
	const int c_totalNumSteps = 256;
	int numSteps = ftoi( c_totalNumSteps * (end - start) );
	numSteps = Clamp(numSteps,16,256);
	
	double errSqr = NumericalErrSqr(me,other,start,end,numSteps);
	
	// scale by time range so it's like the area of error
	//	@@ could scale by length
	errSqr *= (end - start);
	
	pts->errSqr = (float) errSqr;
}

void BezierSpline::MakeSimplification(const BezierSpline & from,const float maxErrSqr)
{
	// set from ends to start us with 1 segment
	SetUniformTimeSampled(from,2);
	
	// now keep splitting the worst one of my pieces until we're under error :
	
	vector<PieceToSplit> heap;
	heap.push_back();
	InitPieceToSplit( &heap.back(), *this, from, 0 );
	
	while ( ! heap.empty() )
	{
		std::pop_heap(heap.begin(),heap.end());
		PieceToSplit cur = heap.back();
		heap.pop_back();
		
		if ( cur.errSqr < maxErrSqr )
			break; // done
			
		// find the piece we want to split :
		PlaceOnSpline place;
		GetPlaceByTime(cur.midTime,&place);
	
		// split place.segment
		int piece = place.segment;
		Piece toSplit = m_pieces[piece];
		
		float start,end;
		GetSegmentTimeRange(piece,&start,&end);
		
		m_pieces.insert( m_pieces.begin()+piece, toSplit );
		
		float tmid = faverage(start,end);
		
		Vec3 V0 = toSplit.curve.GetValue0();
		Vec3 D0 = toSplit.curve.GetDerivative0() * (1.f / (end - start));
		Vec3 V1 = toSplit.curve.GetValue1();
		Vec3 D1 = toSplit.curve.GetDerivative1() * (1.f / (end - start));
		
		DURING_ASSERT( const Vec3 T0 = from.GetValue(start) );
		DURING_ASSERT( const Vec3 T1 = from.GetValue(end) );
		ASSERT( T0 == V0 );
		ASSERT( T1 == V1 );
		
		// just add the new point at the middle
		// @@ in theory we could find the best spot to add a new point instead of just always doing the middle
		//  even just trying 1/3,1/2,2/3 might help a lot
		
		Vec3 VM = from.GetValue(tmid);
		Vec3 DM = from.GetDerivativeGlobal(tmid);
		
		// all derivatives are global, make them local :
		
		D0 *= (end - start)*0.5f;
		D1 *= (end - start)*0.5f;
		DM *= (end - start)*0.5f;
		
		m_pieces[piece].curve.SetFromEnds(V0,D0,VM,DM);
		m_pieces[piece].accumTime = tmid;
		
		m_pieces[piece+1].curve.SetFromEnds(VM,DM,V1,D1);
		m_pieces[piece+1].accumTime = end;
		
		heap.push_back();
		InitPieceToSplit( &heap.back(), *this, from, piece );
		std::push_heap(heap.begin(),heap.end());
		
		heap.push_back();
		InitPieceToSplit( &heap.back(), *this, from, piece+1 );
		std::push_heap(heap.begin(),heap.end());
	}
	
	UpdateDerived();
}

//! UpdateDerived uses only "curve"
void BezierSpline::UpdateDerived()
{
	if ( m_pieces.empty() )
		return;

	m_bbox.SetToPoint( m_pieces[0].curve.GetValue0() );

	float accumLength = 0.f;
	for(int i=0;i<m_pieces.size32();i++)
	{
		float length = m_pieces[i].curve.ComputeLength();
		accumLength += length;
		m_pieces[i].accumLength = accumLength;

		m_pieces[i].curve.GetBoundinSphere( &(m_pieces[i].sphere) );

		AxialBox ab;
		m_pieces[i].curve.GetBoundingBox(&ab);
		m_bbox.ExtendToBox(ab);
	}
}

//! ParameterizeByArcLength requires UpdateDerived
void BezierSpline::ParameterizeByArcLength()
{
	if ( m_pieces.empty() )
		return;

	float scale = 1.f / GetTotalLength();
	for(int i=0;i<m_pieces.size32();i++)
	{
		m_pieces[i].accumTime = m_pieces[i].accumLength * scale;
	}
	// make the ending time exactly 1.f
	ASSERT( fisone(m_pieces.back().accumTime) );
	m_pieces.back().accumTime = 1.f;
}

//===============================================================================================

float BezierSpline::GetTotalLength() const
{
	if ( m_pieces.empty() )
		return 0.f;
	return m_pieces.back().accumLength;
}

void BezierSpline::GetSegmentTimeRange(int seg,float *pStart,float *pEnd) const
{

	ASSERT( seg >= 0 && seg < m_pieces.size32() );
	if ( seg == 0 )
		*pStart = 0.f;
	else
		*pStart = m_pieces[seg-1].accumTime;
	*pEnd = m_pieces[seg].accumTime;
}

void BezierSpline::GetSegmentLengthRange(int seg,float *pStart,float *pEnd) const
{
	ASSERT( seg >= 0 && seg < m_pieces.size32() );
	if ( seg == 0 )
		*pStart = 0.f;
	else
		*pStart = m_pieces[seg-1].accumLength;
	*pEnd = m_pieces[seg].accumLength;
}

//! the derivative wrst global time is much larger
const Vec3 BezierSpline::GetDerivativeGlobal(const PlaceOnSpline & place) const
{
	float start,end;
	GetSegmentTimeRange(place.segment,&start,&end);
	float range = end-start;
	ASSERT( range <= 1.f );
	Vec3 localD = GetCurve(place.segment).GetDerivative(place.localTime);
	return localD * (1.f/range);
}

void BezierSpline::GetPlaceByTime(const float t,PlaceOnSpline * pInto) const
{
	ASSERT( fisinrange(t,0.f,1.f) );
	
	if ( m_pieces.empty() )
	{
		pInto->segment = 0;
		pInto->localTime = 0;
		return;
	}

	if( t <= 0.f )
	{
		pInto->segment = 0;
		pInto->localTime = 0;
		return;
	}

	if( t >= 1.f )
	{
		pInto->segment = m_pieces.size32()-1;
		pInto->localTime = 1.f;
		return;
	}

	ASSERT( m_pieces.back().accumTime == 1.f );

	if ( m_type == eUniformTime )
	{
		float v = t * m_pieces.size();
		pInto->segment = ftoi(v);
		pInto->localTime = v - pInto->segment;
		
		#ifdef DO_ASSERTS
		float start,end;
		GetSegmentTimeRange(pInto->segment,&start,&end);		
//	#pragma PRAGMA_MESSAGE("aaron: hitting this assert. t is almost exactly start, but slightly less.")
		ASSERT( fisinrange(t,start,end) );
		//do this instead? ASSERT( fisinrange(t,start-EPSILON,end+EPSILON) );
		#endif
	}
	else
	{
		// @@@@ TODO : could binary search on t for more speed
		int seg = 0;
		while ( t > m_pieces[seg].accumTime && seg < m_pieces.size32()-1 )
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

void BezierSpline::GetPlaceByLength(const float _len,PlaceOnSpline * pInto) const
{	
	if ( m_pieces.empty() )
	{
		pInto->segment = 0;
		pInto->localTime = 0;
		return;
	}

	if( _len <= 0.f )
	{
		pInto->segment = 0;
		pInto->localTime = 0;
		return;
	}

	if( _len >= GetTotalLength() )
	{
		pInto->segment = m_pieces.size32()-1;
		pInto->localTime = 1.f;
		return;
	}

	const float len = fclamp(_len,0.f,GetTotalLength());
	
	if ( m_type == eUniformLength )
	{
		float v = len * m_pieces.size() / GetTotalLength();
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
		while ( len > m_pieces[seg].accumLength )
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

void BezierSpline::GetPlaceByLengthAccurate(const float _length,PlaceOnSpline * pInto) const
{
	const float length = fclamp(_length,0.f,GetTotalLength());
	// first get the seg :
	GetPlaceByLength(length,pInto);

	// I'm in "seg"
	float start,end;
	GetSegmentLengthRange(pInto->segment,&start,&end);
	ASSERT( fisinrange(length,start,end) );

	float lenOnSeg = length - start;
	pInto->localTime = m_pieces[pInto->segment].curve.ComputeParameterForLength(lenOnSeg);
}

float BezierSpline::GetTimeFromPlace(const PlaceOnSpline & place) const
{
	if ( m_pieces.empty() )
		return 0.f;

	float start,end;
	GetSegmentTimeRange(place.segment,&start,&end);
	
	return flerp(start,end,place.localTime);
}

float BezierSpline::GetLengthFromPlace(const PlaceOnSpline & place) const
{
	if ( m_pieces.empty() )
		return 0.f;
	
	float start,end;
	GetSegmentLengthRange(place.segment,&start,&end);
	
	return flerp(start,end,place.localTime);
}

//===============================================================================================
// ComputeClosestPoint

struct PieceRef
{
	int		piece;
	float	minDSqr;
};
struct PieceRefCompare
{
	bool operator()(const PieceRef & p1,const PieceRef & p2) const
	{
		return p1.minDSqr < p2.minDSqr;
	}
};

//! finds the closest point on the curve to "query"
//!	 within an error of maxErrSqr
void BezierSpline::ComputeClosestPointAccurate(const Vec3 & query,const float maxErrSqr,float * pDistSqr, PlaceOnSpline * pTime) const
{
	const int numPieces = m_pieces.size32();
	ASSERT( numPieces > 1 );

	// temp work for distances :
	PieceRef * work = (PieceRef *) _alloca( sizeof(PieceRef) * numPieces );

	// seed the search :
	PlaceOnSpline curBestParameter;
	curBestParameter.localTime = 0.f;
	curBestParameter.segment = 0;
	float curBestDSqr = FLT_MAX;

	int i;
	for(i=0;i<numPieces;i++)
	{
		work[i].piece = i;
		work[i].minDSqr = fsquare( m_pieces[i].sphere.DistanceTo(query) );
	}

	// sort work by minDsqr
	std::sort(work,work+numPieces,PieceRefCompare());
	ASSERT( work[0].minDSqr <= work[1].minDSqr );

	// now step through :
	for(i=0;i<numPieces;i++)
	{
		// early out :
		if ( work[i].minDSqr >= curBestDSqr )
			break;
		
		int seg = work[i].piece;
		const Piece & sub = m_pieces[ seg ];
		float time,dsqr;
		sub.curve.ComputeClosestPointAccurate(query,maxErrSqr,&dsqr,&time);
		ASSERT( (work[i].minDSqr/dsqr) <= 1.f+EPSILON );
		if ( dsqr < curBestDSqr )
		{
			curBestDSqr = dsqr;
			curBestParameter.localTime = time;
			curBestParameter.segment = seg;
		}
	}

	ASSERT( curBestDSqr != FLT_MAX );
	*pDistSqr = curBestDSqr;
	*pTime = curBestParameter;

	// _alloca junk goes away automagically
}


void BezierSpline::ComputeClosestPointLinear(const Vec3 & query,float * pDistSqr, PlaceOnSpline * pFound) const
{
	// seed the search :
	float curBestDSqr = FLT_MAX;
	
	// now step through :
	for(int i=0;i<m_pieces.size32();i++)
	{
		const Piece & sub = m_pieces[ i ];
		float time,dsqr;
		sub.curve.ComputeClosestPointLinear(query,&dsqr,&time);
		if ( dsqr < curBestDSqr )
		{
			curBestDSqr = dsqr;
			pFound->localTime = time;
			pFound->segment = i;
		}
	}

	ASSERT( curBestDSqr != FLT_MAX );
	*pDistSqr = curBestDSqr;
}


//===============================================================================================

static void EdgeSphere_ComputeClosestPointLinear(const Vec3 &v0,const Vec3 &v1,const Sphere & query,const Vec3 & prevResult,float * pDsqr,float * pTime)
{
	// line-sphere intersection

	const Vec3 delta = v1 - v0;
	const Vec3 offset = v0 - query.GetCenter();
	const float deltaSqr = delta.LengthSqr();
	const float offsetSqr = offset.LengthSqr();
	const float rSqr = fsquare( query.GetRadius() );

	const float dot = delta * offset;
	float stuff = dot*dot - deltaSqr*(offsetSqr - rSqr);

	if ( stuff <= 0.f )
	{
		// no intersection
		// point closest to center is point closest to sphere surface
		ComputeClosestPointToEdge(query.GetCenter(),v0,v1,pDsqr,pTime);
		return;
	}

	// segment intersects sphere ; 1 or 2 intersections

	float stuffSqrt = sqrtf(stuff);
	float invDeltaSqr = 1.f / deltaSqr;

	float t1 = ( - dot + stuffSqrt ) * invDeltaSqr;
	float t2 = ( - dot - stuffSqrt ) * invDeltaSqr;

	float t1OnEdge = fclampunit(t1);
	float t2OnEdge = fclampunit(t2);

	float t1Diff = fabsf(t1 - t1OnEdge);
	float t2Diff = fabsf(t2 - t2OnEdge);

	// the closer one to the surface is the one whose "t" matches better
	if ( fiszero(t1Diff) && fiszero(t2Diff) )
	{
		// both are on surface, pick using coherence
		const Vec3 p1 = v0 + t1 * delta;
		const Vec3 p2 = v0 + t2 * delta;
		ASSERT( fequal( DistanceSqr(query.GetCenter(),p1) , rSqr ) );
		ASSERT( fequal( DistanceSqr(query.GetCenter(),p2) , rSqr ) );
		// take the one closer to prev :
		if ( DistanceSqr(p1,prevResult) <= DistanceSqr(p2,prevResult) )
		{
			*pTime = t1OnEdge;
		}
		else
		{
			*pTime = t2OnEdge;
		}

		*pDsqr = 0.f;
	}
	else if ( t1Diff <= t2Diff )
	{
		*pTime = t1OnEdge;
		*pDsqr = fsquare(t1Diff) * deltaSqr; //fsquare( t1Diff * dot ) / offsetSqr;
	}
	else
	{
		*pTime = t2OnEdge;
		*pDsqr = fsquare(t2Diff) * deltaSqr; //fsquare( t2Diff * dot ) / offsetSqr;
	}
}

//! find the closest point on the spline to the surface of the sphere (linear approximation)
//	if the Sphere intersects the spline, there may be multiple solutions with a
//	distance of zero; use coherence to pick between them.
void BezierSpline::ComputeClosestPointLinear(const Sphere & query,const Vec3 & prevResult,float * pDistSqr, PlaceOnSpline * pFound) const
{
	// seed the search :
	float curBestDSqr = FLT_MAX;
	
	// now step through :
	for(int i=0;i<m_pieces.size32();i++)
	{
		const Piece & sub = m_pieces[ i ];
		float time,dsqr;
		EdgeSphere_ComputeClosestPointLinear(sub.curve.GetValue0(),sub.curve.GetValue1(),query,prevResult,&dsqr,&time);
		if ( dsqr < curBestDSqr )
		{
			curBestDSqr = dsqr;
			pFound->localTime = time;
			pFound->segment = i;
		}
	}

	ASSERT( curBestDSqr != FLT_MAX );
	*pDistSqr = curBestDSqr;	
}

//===============================================================================================

//! compare slines and return an error in distance squared
/*static*/ float BezierSpline::DifferenceSqr(const BezierSpline & s1,const BezierSpline & s2,const int numSamples)
{
	ASSERT( numSamples > 2 );

	float worstErrSqr = 0.f;

	float dt = 1.f/(numSamples-1);
	for(int i=0;i<numSamples;i++)
	{
		float t = i * dt;
		const Vec3 v1 = s1.GetValue(t);
		const Vec3 v2 = s2.GetValue(t);
		float dSqr = DistanceSqr(v1,v2);
		worstErrSqr = MAX(worstErrSqr,dSqr);
	}

	return worstErrSqr;
}

//! resample starting with "from" , to make a spline with as few controls as possible
//!	currently uses uniform-time sampling; eg. does not put fewer samples were the
//!	spline is smoother
void BezierSpline::MakeMinimumUniformTimeSampled(const BezierSpline & from,const float maxErrSqr)
{
	static const int c_numCompareSamples = 100;

	// do a binary search
	// will add 2 to searcher to make numKnots
	int searcher = from.GetNumSegments() -1;
	int searcherStep = (searcher+1)>>1;

	for( ;searcherStep > 1; searcherStep = (searcherStep+1)>>1)
	{
		int trySearcher = searcher - searcherStep;
		if ( trySearcher < 0 )
			continue;
		
		int tryKnots = trySearcher + 2;

		// @@ which sampler to use ?
		SetUniformTimeSampled(from,tryKnots);
		float errSqr = DifferenceSqr(*this,from,c_numCompareSamples);
		if ( errSqr <= maxErrSqr )
		{
			ASSERT( trySearcher < searcher );
			searcher = trySearcher;
		}
	}

	int bestNumKnots = searcher+2;

	SetUniformTimeSampled(from,bestNumKnots);
}

//===============================================================================================

/*
	Try to find a point on the spline that is within "radius" of centerPos
	nearestPointXY should be the closest point to centerPos
	the result will be close to lastT if there are ambiguities

	the times are [0,1] times on the entire Spline
	returns the "t" value for the returned point
*/
bool BezierSpline::FindClosestCircleIntersection(
									Vec3 * const pFoundPos, 
									float * pFoundT,
									  const Vec2& centerPosXY, 
									  const float lastT,
									  const float radius, 
									  const float nearestT,
									  const Vec2& nearestPointXY ) const
{
	//PROFILE( Spline_FindClosestCircleIntersection );
	ASSERT( pFoundPos != NULL && pFoundT != NULL );

	/*
	if( lastT == nearestT )
	{
		*pNearestEdgePos = Evaluate(lastT);
		return lastT;
	}
	*/

	const float radiusSqr = fsquare(radius);

	// can't get any closer than nearestPointXY
	//	so if that is farther than radiusSqr , just return that,
	//	it's the closest
	const float nearestDistSqr = DistanceSqr(nearestPointXY,centerPosXY);
	if ( nearestDistSqr >= radiusSqr )
	{
		*pFoundPos = GetValue(nearestT);
		*pFoundT = nearestT;
		return false;		
	}

	// distance from centerPosXY to spline is minimum at "nearestT"
	//	search in T space, on the side of the spline where we were before
	float startT = nearestT;
	float endT = lastT < nearestT ? 0.f : 1.f;
	float newT = endT;
	
	// currently , DistanceSqr(startT pos) < radiusSqr
	//	so try to walk towards points with larger distance

	const int maxIterations = 100;
	const float stoppingCriterionSqr = fsquare(0.1f);

	int count = 0;

	Vec2 posAtNewT = MakeProjectionXY(GetValue(newT)); //need XY evaluator.	
	float distFromBaseSqr = DistanceSqr(posAtNewT, centerPosXY);

	while( fabsf(radiusSqr - distFromBaseSqr) > stoppingCriterionSqr && count < maxIterations && !fequal(startT,endT) ) 
	{
		count++;

		newT = (startT + endT) * 0.5f;
		Vec2 posAtNewT = MakeProjectionXY(GetValue(newT)); //xy evaluator
		distFromBaseSqr = DistanceSqr(posAtNewT, centerPosXY);
		// if the mid-point is still too close, move out to it
		// otherwise if it's too far, explore between start and there
		if( distFromBaseSqr < radiusSqr )
		{
			startT = newT;
		}
		else
		{
			endT = newT;
		}
	}
	
	// could choose between startT and endT at this point
	//	(newT matches one of them)
	// but forget it for now

	*pFoundPos = GetValue(newT);
	*pFoundT = newT;

	if(count >= maxIterations)
	{
		lprintf("Warning: FindClosestCircleIntersection() exceeded count of %d.\n", maxIterations);
		return false;
	}
	
	return true;
}

//===============================================================================================

END_CB
