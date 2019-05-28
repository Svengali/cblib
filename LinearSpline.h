#pragma once

#include "cblib/Vec3.h"
#include "cblib/AxialBox.h"
#include "cblib/vector.h"

START_CB

class BezierSpline;

/*

a "Linear" spline is just a bunch of connected line segments

"time" runs over the segments, spending equal time on each segment
for smooth overall parameterization, use length

*/

class LinearSpline
{
public:

	LinearSpline();
	~LinearSpline() { }

	//-----------------------------------------------------------------------------

	//! Sample another spline at equial time spots
	void SetUniformTimeSampled(const BezierSpline & from,const int numKnots);

	//	SetFromPoints takes the points and uses them as knots
	//	scales durations based on distances of knots
	void SetFromPoints(const Vec3 * points,const int numPoints);

	//-----------------------------------------------------------------------------

	const AxialBox & GetBBox() const { return m_bbox; }

	float GetTotalLength() const { return m_knots.back().accumLength; }

	//-----------------------------------------------------------------------------

	struct PlaceOnSpline
	{
		PlaceOnSpline();
		int		segment;
		float	localTime;
	};

	int GetNumKnots() const		{ return m_knots.size32(); }
	int GetNumSegments() const	{ return m_knots.size32()-1; }
	const Vec3 & GetKnot(const int i) const { return m_knots[i].point; }

	void GetPlaceByTime(const float t,PlaceOnSpline * pInto) const;
	void GetPlaceByLength(const float length,PlaceOnSpline * pInto) const;

	float GetTimeFromPlace(const PlaceOnSpline & place) const;
	float GetLengthFromPlace(const PlaceOnSpline & place) const;

	void GetSegmentTimeRange(	int seg,float *pStart,float *pEnd) const;
	void GetSegmentLengthRange(	int seg,float *pStart,float *pEnd) const;

	float GetTimeFromLength(const float length) const
	{
		PlaceOnSpline place;
		GetPlaceByLength(length,&place);
		return GetTimeFromPlace(place);
	}

	//-----------------------------------------------------------------------------

	void ComputeClosestPoint(const Vec3 & query,float * pDistSqr, PlaceOnSpline * pTime) const;

	const Vec3 GetValue(const PlaceOnSpline & place) const;
	const Vec3 GetTangent(const PlaceOnSpline & place) const;
	const Vec3 GetTangentNormal(const PlaceOnSpline & place) const;

	const Vec3 GetValue(const float time) const
	{
		PlaceOnSpline place;
		GetPlaceByTime(time,&place);
		return GetValue(place);
	}
	const Vec3 GetTangent(const float time) const
	{
		PlaceOnSpline place;
		GetPlaceByTime(time,&place);
		return GetTangent(place);
	}

	bool IsValid() const;

	//-----------------------------------------------------------------------------

private:
	void UpdateDerived();

	struct Knot
	{
		Vec3	point;
		float	accumTime;	// time of the end of this piece
		float	accumLength; // length + length of predecessors (derived)
	};

	enum ESplineType
	{
		eInvalid,
		eGeneric,
		eUniformTime,
		eUniformLength
	};

	ESplineType		m_type;
	vector<Knot>	m_knots;
	AxialBox		m_bbox; //(derived)
};

END_CB
