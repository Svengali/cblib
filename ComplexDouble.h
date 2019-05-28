#pragma once

#include "cblib/Base.h"
#include "cblib/Util.h"
#include "cblib/FloatUtil.h"

/**

YO ! FUCKING CB : THIS IS DUMB

using std::complex<double> instead

**/

START_CB

//! complex number class
class ComplexDouble
{
public:
	double re,im;

	//-------------------------------------------------------------------

	ComplexDouble()
	{
		//! do-nada constructor; 
		dinvalidatedbg(re);
		dinvalidatedbg(im);
	}

	explicit ComplexDouble(double r) : re(r), im(0.0)
	{
	}

	explicit ComplexDouble(double r,double i) : re(r) , im(i)
	{
	}
	
	static const ComplexDouble zero;
	static const ComplexDouble one;
	static const ComplexDouble I;

	void operator = (const double v);
	void operator *= (const double scale);
	void operator /= (const double scale);
	void operator *= (const ComplexDouble & v);
	void operator /= (const ComplexDouble & v);
	void operator += (const double val);
	void operator -= (const double val);
	void operator += (const ComplexDouble & v);
	void operator -= (const ComplexDouble & v);
	bool operator == (const ComplexDouble & v) const; //!< exact equality test
	bool operator != (const ComplexDouble & v) const; //!< exact equality test
	bool operator == (const double v) const; //!< exact equality test
	bool operator != (const double v) const; //!< exact equality test

	static const ComplexDouble MakeUnitPolar(const double radians)
	{
		double c = cos(radians);
		double s = sin(radians);
		return ComplexDouble(c,s);
	}

	static const ComplexDouble MakePolar(const double mag,const double radians)
	{
		double c = cos(radians);
		double s = sin(radians);	
		return ComplexDouble(mag*c,mag*s);
	}
	
	const ComplexDouble GetConjugate() const
	{
		return ComplexDouble(re,-im);
	}
	
	double GetMagnitudeSquared() const
	{
		return re*re + im*im;
	}
	double GetMagnitude() const
	{
		return sqrt( GetMagnitudeSquared() );
	}
	double GetAngle() const
	{
		return atan2( im, re );
	}
	
	const ComplexDouble GetInverse() const
	{
		double m2 = GetMagnitudeSquared();
		ASSERT( m2 != 0.0 );
		double inv = 1.0/m2;
		return ComplexDouble(re*inv,-im*inv);
	}
	
	const ComplexDouble GetNormalized() const
	{
		double mag = GetMagnitude();
		ASSERT( mag != 0.0 );
		double inv = 1.0/mag;
		return ComplexDouble(re*inv,im*inv);
	}
	
	bool IsReal(const double tolerance = EPSILON) const
	{
		return fiszero(im,tolerance);
	}
	bool Equals(const ComplexDouble & rhs,const double tolerance = EPSILON) const
	{
		return	fiszero( re - rhs.re , tolerance ) &&
				fiszero( im - rhs.im , tolerance );
	}
	bool Equals(const double rhs,const double tolerance = EPSILON) const
	{
		return	fiszero( re - rhs , tolerance ) &&
				fiszero( im , tolerance );
	}
};

//-------------------------------------------------------------------------

inline void ComplexDouble::operator = (const double v)
{
	re = v;
	im = 0.0;
}

inline void ComplexDouble::operator *= (const double scale)
{
	re *= scale;
	im *= scale;
}

inline void ComplexDouble::operator *= (const ComplexDouble & rhs)
{
	double r = re * rhs.re - im * rhs.im;
	double i = re * rhs.im + im * rhs.re;
	re = r;
	im = i;
}

inline void ComplexDouble::operator /= (const double scale)
{
	ASSERT( scale != 0.f );
	const double inv = 1.f / scale;
	re *= inv;
	im *= inv;
}

inline void ComplexDouble::operator /= (const ComplexDouble & rhs)
{
	(*this) *= rhs.GetInverse();
}

inline void ComplexDouble::operator += (const ComplexDouble & v)
{
	re += v.re;
	im += v.im;
}

inline void ComplexDouble::operator -= (const ComplexDouble & v)
{
	re -= v.re;
	im -= v.im;
}

inline void ComplexDouble::operator += (const double v)
{
	re += v;
}

inline void ComplexDouble::operator -= (const double v)
{
	re -= v;
}

inline bool ComplexDouble::operator == (const ComplexDouble & v) const
{
	return (re == v.re &&
			im == v.im );
}

inline bool ComplexDouble::operator == (const double v) const
{
	return (re == v &&
			im == 0.0 );
}

inline bool ComplexDouble::operator != (const ComplexDouble & v) const
{
	return !operator==(v);
}

inline bool ComplexDouble::operator != (const double v) const
{
	return (re != v ||
			im != 0.0 );
}

inline const ComplexDouble operator - (const ComplexDouble & a )
{
	return ComplexDouble( - a.re, - a.im );
}

inline const ComplexDouble operator + (const ComplexDouble & a )
{
	return a;
}

inline const ComplexDouble operator * (const ComplexDouble & a,const ComplexDouble & b)
{
	return ComplexDouble( a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re );
}

inline const ComplexDouble operator / (const ComplexDouble & a,const ComplexDouble & b)
{
	const ComplexDouble binv = b.GetInverse();
	return a * binv;
}

inline const ComplexDouble operator + (const ComplexDouble & a,const ComplexDouble & b)
{
	return ComplexDouble( a.re + b.re, a.im + b.im );
}

inline const ComplexDouble operator - (const ComplexDouble & a,const ComplexDouble & b)
{
	return ComplexDouble( a.re - b.re, a.im - b.im );
}

inline const ComplexDouble operator * (const ComplexDouble & a,const double b)
{
	return ComplexDouble( a.re * b, a.im * b );
}

inline const ComplexDouble operator / (const ComplexDouble & a,const double b)
{
	return ComplexDouble( a.re / b , a.im / b );
}

inline const ComplexDouble operator + (const ComplexDouble & a,const double b)
{
	return ComplexDouble( a.re + b, a.im );
}

inline const ComplexDouble operator - (const ComplexDouble & a,const double b)
{
	return ComplexDouble( a.re - b, a.im );
}

inline const ComplexDouble operator * (const double b,const ComplexDouble & a)
{
	return ComplexDouble( a.re * b, a.im * b );
}

inline const ComplexDouble operator / (const double b,const ComplexDouble & a)
{
	return ComplexDouble(b,0) / a;
}

inline const ComplexDouble operator + (const double b,const ComplexDouble & a)
{
	return ComplexDouble( a.re + b, a.im );
}

inline const ComplexDouble operator - (const double b,const ComplexDouble & a)
{
	return ComplexDouble( b - a.re, - a.im );
}

//-------------------------------------------------------------

const ComplexDouble ComplexSqrt(const ComplexDouble & v);
const ComplexDouble ComplexSqrt(const double v);

const ComplexDouble ComplexPow(const ComplexDouble & v, const double p);
const ComplexDouble ComplexPow(const double v, const double p);

// ax^2 + bx + c = 0
//	returns number of solutions
int SolveQuadratic(const double a,const double b,const double c,
					ComplexDouble * root1,ComplexDouble * root2);

// ax^3 + bx^2 + cx + d = 0
void SolveCubic(double a,double b,double c,double d,
					ComplexDouble * px0,
					ComplexDouble * px1,
					ComplexDouble * px2);
					
// ax^4 + cx^2 + dx + e = 0
void SolveDepressedQuartic(double A,double C,double D,double E,
					ComplexDouble * px0,
					ComplexDouble * px1,
					ComplexDouble * px2,
					ComplexDouble * px3);
					
END_CB
