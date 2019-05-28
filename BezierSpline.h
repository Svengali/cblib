#pragma once

/*

a BezierSpline is just a bunch of Bezier curves,
with piecewise linear time control for how they fit together

*/
#include "cblib/Bezier.h"
#include "cblib/Sphere.h"
#include "cblib/AxialBox.h"
#include "cblib/vector.h"

START_CB

class Vec2;

class BezierSpline
{
public:
	 BezierSpline();
	~BezierSpline();

	bool IsValid() const;

	//----------------------------------------------
	// Setup :

	enum EEndHandling
	{
		eEnd_Clamp,
		eEnd_Wrap,
	};
	
	void SetFromPoints(const Vec3 * pKnots,const int numKnots,const EEndHandling eEnds);

	struct Knot
	{
		Vec3	point;
		float	time;
	};
	void SetFromKnots(const Knot * pKnots,const int numKnots,const EEndHandling eEnds);
	
	//! Sample another spline at equi-distant spots to make a new spline with
	//	equal arc-length spaced knots 
	//void SetUniformSampled(const BezierSpline & from,const int numKnots);

	void SetUniformTimeSampled(const BezierSpline & from,const int numKnots);
	void SetUniformDistanceSampled(const BezierSpline & from,const int numKnots);

	void SetSubdivisionSampled(const BezierSpline & from,const float maxErrSqr);

	void MakeSimplification(const BezierSpline & from,const float maxErrSqr);

	void MakeMinimumUniformTimeSampled(const BezierSpline & from,const float maxErrSqr);

	//----------------------------------------------
	// Queries :
	
	struct PlaceOnSpline
	{
		int		segment;
		float	localTime;
	};

	int GetNumSegments() const { return m_pieces.size32(); }
	const Bezier & GetCurve(int seg) const { return m_pieces[seg].curve; }

	const AxialBox & GetBBox() const { return m_bbox; }

	float GetTotalLength() const;

	//! the Length functions here treat arc-length as if it
	//	varies linearly on each segment (an approximation)
	void GetSegmentTimeRange(int seg,float *pStart,float *pEnd) const;
	void GetSegmentLengthRange(int seg,float *pStart,float *pEnd) const;

	void GetPlaceByTime(const float t,PlaceOnSpline * pInto) const;
	void GetPlaceByLength(const float length,PlaceOnSpline * pInto) const;

	void GetPlaceByLengthAccurate(const float length,PlaceOnSpline * pInto) const;

	float GetTimeFromPlace(const PlaceOnSpline & place) const;
	float GetLengthFromPlace(const PlaceOnSpline & place) const;
	
	//! finds the closest point on the curve to "query"
	//!	 within an error of maxErrSqr
	void ComputeClosestPointAccurate(const Vec3 & query,const float maxErrSqr,float * pDistSqr, PlaceOnSpline * pTime) const;

	void ComputeClosestPointLinear(const Vec3 & query,float * pDistSqr, PlaceOnSpline * pTime) const;
	void ComputeClosestPointLinear(const Sphere & query,const Vec3 & prevResult,float * pDistSqr, PlaceOnSpline * pFound) const;

	//----------------------------------------------
	//! To do serious queries, get the Curve and then talk to it
	//	here are some little helpers :

	const Vec3 GetValue(const float t) const
	{
		PlaceOnSpline place;
		GetPlaceByTime(t,&place);
		return GetCurve(place.segment).GetValue(place.localTime);	
	}
	const Vec3 GetValue(const PlaceOnSpline & place) const
	{
		return GetCurve(place.segment).GetValue(place.localTime);	
	}
	//! this is the derivative with respect to local time;
	//	if the knots are not equally spaced in time, this needs to be adjusted
	//	to be the correct derivative WRST global time
	const Vec3 GetDerivativeLocal(const float t) const
	{
		PlaceOnSpline place;
		GetPlaceByTime(t,&place);
		return GetCurve(place.segment).GetDerivative(place.localTime);	
	}
	const Vec3 GetDerivativeLocal(const PlaceOnSpline & place) const
	{
		return GetCurve(place.segment).GetDerivative(place.localTime);	
	}
	const Vec3 GetDerivativeGlobal(const float t) const
	{
		PlaceOnSpline place;
		GetPlaceByTime(t,&place);
		return GetDerivativeGlobal(place);
	}
	const Vec3 GetDerivativeGlobal(const PlaceOnSpline & place) const;

	//----------------------------------------------

	enum ESplineType
	{
		eInvalid,
		eGeneric,
		eUniformTime,
		eUniformLength
	};
	
	struct Piece
	{
		// parameters :
		Bezier		curve;
		float		accumTime;	// time of the end of this piece

		// derived :
		Sphere		sphere;
		float		accumLength; // length + length of predecessors
	};

	//----------------------------------------------
	static float DifferenceSqr(const BezierSpline & s1,const BezierSpline & s2,const int numSamples);

	bool FindClosestCircleIntersection(
									Vec3 * const pFoundPos, 
									float * pFoundT,
									  const Vec2& centerPosXY, 
									  const float lastT,
									  const float radius, 
									  const float nearestT,
									  const Vec2& nearestPointXY ) const;
									  
	//----------------------------------------------
	
private:

	void SetSubdivisionSampled_RecursiveAdd( const Bezier & curve, const float t0, const float t1, const float maxErrSqr);

	void UpdateDerived();
	void ParameterizeByArcLength();

	vector<Piece>	m_pieces;
	AxialBox		m_bbox;
	ESplineType		m_type;
	// m_pieces.back().time = 1.f;
};

END_CB
