#pragma once

#include "Util.h"

START_CB

/*********

TypeTraits helper to tell you things about types :

You can do compile-time switches on TypeTraits like this :

	template <class T>
	void ZZ(const BoolAsType_False,T & value)
	{
		m_total += sizeof(value);
	}
	template <class T>
	void ZZ(const BoolAsType_True,T & value)
	{
		value.Reflection( *this );
	}

	template <class T>
	void operator() (const char * name,T & value)
	{
		ZZ( TypeTraits<T>().hasReflection,value);
	}

***********/

//============================================================

template <typename C_T> struct TypeTraits
{
	BoolAsType_False	hasReflection;
	BoolAsType_True 	isPrimitive;
	BoolAsType_False	ioBytes;
};

#define TT_DECLARE_ISPRIMITIVE(type)		\
template <> struct TypeTraits<type>\
{									\
	BoolAsType_False	hasReflection;\
	BoolAsType_True	isPrimitive;\
	BoolAsType_True	ioBytes;	\
};
#define TT_DECLARE_ISPODCLASS(type)		\
template <> struct TypeTraits<type>\
{									\
	BoolAsType_False	hasReflection;\
	BoolAsType_False	isPrimitive;\
	BoolAsType_True	ioBytes;	\
};

#define TT_DECLARE_ISPRIMITIVE_US(type) \
	TT_DECLARE_ISPRIMITIVE(type) \
	TT_DECLARE_ISPRIMITIVE(unsigned type) \

TT_DECLARE_ISPRIMITIVE_US(int)
TT_DECLARE_ISPRIMITIVE_US(short)
TT_DECLARE_ISPRIMITIVE_US(long)
TT_DECLARE_ISPRIMITIVE_US(char)
TT_DECLARE_ISPRIMITIVE(float)
TT_DECLARE_ISPRIMITIVE(double)
// @@ any enum
// @@ any raw pointer

// @@@@ with VC7.1 you can partial specialize like this :
//	(though doing ioBytes on a pointer is questionable)
/*
template <typename T *> struct TypeTraits
{
	BoolAsType_False	hasReflection;
	BoolAsType_True	isPrimitive;
	BoolAsType_True	ioBytes;
};
*/

//============================================================
// forward decls for basic Galaxy types :

/*
class Token;
class gCRC;
class gVec2;
class gVec3;
class gVec4;
class Vec2i;
class Vec3i;
class Mat2;
class Mat3;
class Mat4;
class gQuat;
class gColor;
class gFrame3;
class Frame3Scaled;
class gAxialBox;
class gCone;
class gSphere;
class gRect;
class OrientedBox;

// @@ some of these should have Reflection and still be IO-Bytes
TT_DECLARE_ISPODCLASS(Token)
TT_DECLARE_ISPODCLASS(gCRC)
TT_DECLARE_ISPODCLASS(gVec2)
TT_DECLARE_ISPODCLASS(gVec3)
TT_DECLARE_ISPODCLASS(gVec4)
TT_DECLARE_ISPODCLASS(Vec2i)
TT_DECLARE_ISPODCLASS(Vec3i)
TT_DECLARE_ISPODCLASS(Mat2)
TT_DECLARE_ISPODCLASS(Mat3)
TT_DECLARE_ISPODCLASS(Mat4)
TT_DECLARE_ISPODCLASS(gQuat)
TT_DECLARE_ISPODCLASS(gColor)
TT_DECLARE_ISPODCLASS(gFrame3)
TT_DECLARE_ISPODCLASS(Frame3Scaled)
TT_DECLARE_ISPODCLASS(gAxialBox)
TT_DECLARE_ISPODCLASS(gCone)
TT_DECLARE_ISPODCLASS(gSphere)
TT_DECLARE_ISPODCLASS(gRect)
TT_DECLARE_ISPODCLASS(OrientedBox)
*/

//============================================================

END_CB
