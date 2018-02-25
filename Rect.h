#pragma once

#include "cblib/Util.h"
#include "cblib/Vec2.h"

START_CB

class RectI; //! integer rectangle
class RectF; //! float rectangle

/*!

RectI and RectF rectangles

made of X and Y {lo,hi} ranges

RectI is byte-wise identical to the windows "RECT"

*/

//////////////////////////////
/*! @@ \todo : put the template function bodies in the cpp file;
	I'm pretty sure the template instantiation commands in there are
	all that's needed

*/

/*! RectTemplate
	generic rectangle of any value_type
	RectI and RectF are just instances of this
*/

template <class T_value_type> class RectTemplate
{
public:
	typedef T_value_type				value_type;
	typedef RectTemplate<value_type>	this_type;

	////////////////////////////////////////////////////////////////
	// construct and destruct :

	//! NO default constructor, because it's impossible to make
	//!	one that makes sense for all template parameters

	/*
	inline  RectTemplate()
	{
		finvalidatedbg(m_xLo);
		finvalidatedbg(m_xHi);
		finvalidatedbg(m_yLo);
		finvalidatedbg(m_yHi);
	}
	*/

	enum EConstructorZero		{ eZero };

	explicit inline RectTemplate(const EConstructorZero) :
		m_xLo(0),m_xHi(0),m_yLo(0),m_yHi(0)
	{
	}

	//! this one can be confusing, it's used like this :
	//!		RectF(xLo,xHi,yLo,yHi)
	//!		RectF(0.f,1.f,0.f,1.f)
	explicit inline RectTemplate(const value_type xLo,const value_type xHi,const value_type yLo,const value_type yHi) :
		m_xLo(xLo),m_xHi(xHi),m_yLo(yLo),m_yHi(yHi)
	{
		ASSERT( IsValid() );
	}

	explicit inline RectTemplate(const value_type x,const value_type y) :
		m_xLo(x),m_xHi(x),m_yLo(y),m_yHi(y)
	{
		ASSERT( IsValid() ); 
	}

	bool IsValid() const
	{
		/*
		ASSERT( fisvalid(m_xLo) );
		ASSERT( fisvalid(m_xHi) );
		ASSERT( fisvalid(m_yLo) );
		ASSERT( fisvalid(m_yHi) );
		*/
		ASSERT( m_xHi >= m_xLo );
		ASSERT( m_yHi >= m_yLo );
		return true;
	}

	// default copy constructor and operator = and such are all fine

	////////////////////////////////////////////////////////////////
	// Gets :

	value_type Left()	const { return m_xLo; }
	value_type Right()	const { return m_xHi; }
	value_type Top()	const { return m_yLo; }
	value_type Bottom()	const { return m_yHi; }

	value_type LoX()	const { return m_xLo; }
	value_type HiX()	const { return m_xHi; }
	value_type LoY()	const { return m_yLo; }
	value_type HiY()	const { return m_yHi; }

	value_type Width() const	{ ASSERT( IsValid() ); return (m_xHi - m_xLo); }
	value_type Height() const	{ ASSERT( IsValid() ); return (m_yHi - m_yLo); }

	value_type Area() const		{ return Width() * Height(); }

	value_type XCenter() const	{ ASSERT( IsValid() ); return (m_xHi + m_xLo)/((value_type)2); }
	value_type YCenter() const	{ ASSERT( IsValid() ); return (m_yHi + m_yLo)/((value_type)2); }

	value_type GetPerimeter() const { ASSERT(IsValid()); return (Width() + Height()) * ((value_type)2); }

	////////////////////////////////////////////////////////////////
	// Sets :

	// set the range on each axis :
	void SetX(const value_type lo,const value_type hi)
	{
		ASSERT( hi >= lo );
		m_xLo = lo;
		m_xHi = hi;
		// may not be valid at this point
	}

	void SetY(const value_type lo,const value_type hi)
	{
		ASSERT( hi >= lo );
		m_yLo = lo;
		m_yHi = hi;
		// may not be valid at this point
	}

	//! set to a point :
	void SetToPoint(const value_type x,const value_type y)
	{
		// this may not be valid yet
		m_xLo = x;
		m_xHi = x;
		m_yLo = y;
		m_yHi = y;
		ASSERT( IsValid() );
	}

	//! set all four corners
	//void Set(const value_type xLo,const value_type xHi,const value_type yLo,const value_type yHi)
	void Set(const value_type xLo,const value_type xHi,const value_type yLo,const value_type yHi)
	{
		// this may not be valid yet
		SetX(xLo,xHi);
		SetY(yLo,yHi);
		
		ASSERT( IsValid() );
	}

	////////////////////////////////////////////////////////////////
	// Geometry :

	void ExtendToPoint(const value_type x,const value_type y)
	{
		ASSERT(IsValid());
		m_xLo = MIN(m_xLo,x);
		m_xHi = MAX(m_xHi,x);
		m_yLo = MIN(m_yLo,y);
		m_yHi = MAX(m_yHi,y);
		ASSERT(IsValid());
		ASSERT( Contains(x,y) );
	}

	void SetEnclosing(const this_type & r1)
	{
		SetEnclosing(*this,r1);
	}

	void SetEnclosing(const this_type & r1,const this_type & r2)
	{
		ASSERT( r1.IsValid() && r2.IsValid() );
		m_xLo = MIN(r1.m_xLo,r2.m_xLo);
		m_xHi = MAX(r1.m_xHi,r2.m_xHi);
		m_yLo = MIN(r1.m_yLo,r2.m_yLo);
		m_yHi = MAX(r1.m_yHi,r2.m_yHi);
		ASSERT(IsValid());
		ASSERT( Contains(r1) && Contains(r2) );
	}

	/*!  SetIntersection
		makes a rect from the intersection of two others
		returns bool to indicate whether the rects actually intersected 

		if the return is false, then the intersection was the empty set,
		and the resultant rect is INVALID !  You must clear it or not use
		it again.  I intentionally do NOT set it to (0,0) or anything valid
		like that so that I can be sure that you don't just keep using it.
	*/

	bool SetIntersection(const this_type & r1)
	{
		return SetIntersection(*this,r1);
	}

	bool SetIntersection(const this_type & r1,const this_type & r2)
	{
		ASSERT( r1.IsValid() && r2.IsValid() );

		// set the Lo to the larger of the two Lo's
		//	and so on

		m_xLo = MAX(r1.m_xLo,r2.m_xLo);
		m_xHi = MIN(r1.m_xHi,r2.m_xHi);
		m_yLo = MAX(r1.m_yLo,r2.m_yLo);
		m_yHi = MIN(r1.m_yHi,r2.m_yHi);

		// check to see if they actually intersected !
		if ( m_xLo > m_xHi || m_yLo > m_yHi )
		{
			return false;
		}

		ASSERT(IsValid());

		return true;
	}

	bool Contains(const value_type x,const value_type y) const
	{
		ASSERT( IsValid() );
		return (m_xLo <= x && m_xHi >= x &&
				m_yLo <= y && m_yHi >= y );
	}

	bool Contains(const this_type & r) const
	{
		ASSERT( IsValid() && r.IsValid() );
		return (m_xLo <= r.m_xLo && m_xHi >= r.m_xHi &&
				m_yLo <= r.m_yLo && m_yHi >= r.m_yHi );
	}

	bool Intersects(const this_type & r) const
	{
		return Intersects(*this,r);
	}

	static bool Intersects(const this_type & r1,const this_type & r2)
	{
		ASSERT( r1.IsValid() && r2.IsValid() );

		// intersecting iff the larger of the min's
		//	is smaller than the smaller of the max's
		//	in both dimensions

		return (
			MAX( r1.m_xLo, r2.m_xLo ) <= MIN( r1.m_xHi, r2.m_xHi ) &&
			MAX( r1.m_yLo, r2.m_yLo ) <= MIN( r1.m_yHi, r2.m_yHi )
			);
	}

	void Expand(const value_type r)
	{
		ASSERT( r >= 0 );
		m_xLo -= r;
		m_xHi += r;
		m_yLo -= r;
		m_yHi += r;
	}

	// eccentricity >= 1.f
	float GetEccentricity() const
	{
		float w = (float) Width();
		float h = (float) Height();
		return MAX(w,h) / MIN(w,h);
	}

	value_type GetMaxSideLength() const
	{
		return MAX( Width(), Height() );
	}

	////////////////////////////////////////////////////////////////

protected:

	enum EConstructorNoInit		{ eNoInit };

	explicit inline RectTemplate(const EConstructorNoInit)
	{
	}

	value_type	m_xLo,m_yLo,m_xHi,m_yHi;
};

/////////////////////////////////////////////////////////////////////////////
// explicit instantiation of the template to RectI and RectF :

class RectI : public RectTemplate<int>
{
public:
	typedef RectTemplate<value_type> parent_type;

	inline RectI()
	#ifdef _DEBUG
	 : parent_type(0xABADF00D,0xABADF00D)
	#else
	 : parent_type(eNoInit)
	#endif
	{
	}

	explicit inline RectI(const EConstructorZero e) : parent_type(e)
	{
	}

	explicit inline  RectI(const int xLo,const int xHi,const int yLo,const int yHi) :
		parent_type(xLo,xHi,yLo,yHi)
	{
		ASSERT( IsValid() );
	}

	explicit inline  RectI(const int x,const int y) : parent_type(x,y)
	{
		ASSERT( IsValid() );
	}

	bool operator == (const RectI & other) const;
	
	static const RectI zero;
	static const RectI unit;
};

/////////////////////////////////////////////////////////////////////////////

class RectF : public RectTemplate<float>
{
public:
	typedef RectTemplate<value_type> parent_type;
	
	inline RectF() : parent_type(eNoInit)
	{
		finvalidatedbg(m_xLo);
		finvalidatedbg(m_xHi);
		finvalidatedbg(m_yLo);
		finvalidatedbg(m_yHi);
	}

	explicit inline RectF(const EConstructorZero e) : parent_type(e)
	{
	}

	explicit inline  RectF(const float xLo,const float xHi,const float yLo,const float yHi) :
		parent_type(xLo,xHi,yLo,yHi)
	{
		ASSERT( IsValid() );
	}

	explicit inline  RectF(const float x,const float y) : parent_type(x,y)
	{
		ASSERT( IsValid() );
	}

	explicit inline  RectF(const Vec2 & v) : parent_type(v.x,v.y)
	{
		ASSERT( IsValid() );
	}

	explicit inline  RectF(const Vec2 & u,const Vec2 & v) : parent_type(MIN(u.x,v.x),MAX(u.x,v.x),MIN(u.y,v.y),MAX(u.y,v.y))
	{
		ASSERT( IsValid() );
	}


	//-------------------------------------------------------------------------
	// Conversions/helpers for Vec2 :

	void SetToTwoPointsV(const Vec2 &u,const Vec2 &v)	{ Set(MIN(u.x,v.x),MAX(u.x,v.x),MIN(u.y,v.y),MAX(u.y,v.y)); }
	void SetLoHiV(const Vec2 &lo,const Vec2 &hi)		{ Set(lo.x,hi.x,lo.y,hi.y); }

	bool ContainsV(const Vec2 &v) const				{ return Contains(v.x,v.y); }
	void SetToPointV(const Vec2 &v)					{ SetToPoint(v.x,v.y); }
	void ExtendToPointV(const Vec2 &v)				{ ExtendToPoint(v.x,v.y); }
	void SetEnclosingV(const Vec2 &u,const Vec2 &v){ SetToPointV(u); ExtendToPointV(v); }

	const Vec2 GetLo() const { return Vec2( LoX(), LoY() ); }
	const Vec2 GetHi() const { return Vec2( HiX(), HiY() ); }
	
	const Vec2 GetCenter() const { return Vec2( (LoX()+HiX())*0.5f, (LoY()+HiY())*0.5f ); }
	
	void Translate(const Vec2 &t)	{ SetLoHiV( GetLo() + t, GetHi() + t ); }

	void GrowToSquare(); //!< become square by growing the smaller dimension

	void SetScaled(const RectF & r,const float f);
	void SetScaled(const RectF & r,const float xs,const float ys);

	//-------------------------------------------------------------------------

	const Vec2 ScaleVectorIntoRect(const Vec2 & from) const;
	const Vec2 ScaleVectorFromRect(const Vec2 & from) const;

	// override the parent version :
	bool IsValid() const;

	static const RectF zero;
	static const RectF unit;
};

/////////////////////////////////////////////////////////////////////////////

END_CB
