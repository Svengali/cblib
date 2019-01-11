#pragma once

#include "cblib/Base.h"
#include "cblib/TypeTraits.h"
#include "cblib/Log.h"		// just for the PrintMembers functor
#include "cblib/File.h"		// just for the IOMembers functor
#include "cblib/PrefBlock.h" // just for the IOPrefs functor


/***************

There's no base-class at all for Reflection.  It's just a convention which allows you
to be used in templates.  The cool thing about this Reflection is your class just
implements one Reflect() member, and then you can get template dispatch to all kinds of
different functors that can have different kinds of specializations to all the types you
may hold.

For a class to be Reflection-compatible, you just need a member like this :

	template <class T>
	void Reflection(T & functor)
	{
		REFLECT(m_a);
		REFLECT(m_b);
	}

To implement a functor which works in the Reflection call, you do something
like this :

struct PrintMembers
{
	template <class T>
	void operator() (const char * name,T & value)
	{
		lprintf("%s:\n",name);
		gLog::PushTab();
		value.Reflection(*this);
		gLog::PopTab();
	}
};

The default implementation should always just call through to Reflection() on the
unknown type.  You should then specialize for all the basic types and any other
known types you want to handle.

The gIsPOD<> helper can make it easier to specialize for all the basic types.

The only annoying thing is that you can't do virtual templates in C++.  So, you
need to somehow get the initial Reflection to be called on the most-derived type
of your class.  There are two options for that :

1. A concrete-class dispatcher; see Prefs for example.

2. Lots of virtual functions.  You just have to make a virtual function for each
type of Reflection you want to do.  See TestClass in this cpp.

The nice thing about the dispatcher is you don't have to do anything 
in all your classes, it's done automatically for you via templates and such.

*****************/

#include "ReflectionMacros.h"


START_CB

template< typename CLASS >
class Reflect : public CLASS
{
public:
};


class HasReflection
{
public:
	template< typename T >
	void Reflection( T & );
};

/**

required interface to be in Reflection :

	template <class T>
	void Reflection(T & functor)
	{
		REFLECT(m_a);
		REFLECT(m_b);
	}

For static arrays you can just REFLECT_ARRAY , like

	int stuff[4];
	
you can just REFLECT_ARRAY(stuff); and it will make labels "stuff0" , "stuff1", etc.

**/

//============================================================

struct PrintMembers
{
	template <class T>
	void operator() (const char * name,T & value)
	{
		lprintf("%s:\n",name);
		LogPushTab();
		value.Reflection(*this);
		LogPopTab();
	}

	template <> void operator()(const char * name,char & value)
	{
		lprintf("%s:%c\n",name,value);
	}
	template <> void operator()(const char * name,short & value)
	{
		lprintf("%s:%d\n",name,value);
	}
	template <> void operator()(const char * name,int & value)
	{
		lprintf("%s:%d\n",name,value);
	}
	template <> void operator()(const char * name,float & value)
	{
		lprintf("%s:%f\n",name,value);
	}
	template <> void operator()(const char * name,double & value)
	{
		lprintf("%s:%f\n",name,value);
	}
};

//============================================================

struct SizeMembers
{
	int m_total;

	SizeMembers() : m_total(0)
	{		
	}

	template <class T>
	void ZZ(const BoolAsType_False,const BoolAsType_False,T & value)
	{
		m_total += sizeof(value);
	}
	
	template <class T>
	void ZZ(const BoolAsType_False,const BoolAsType_True,T & value)
	{
		value.Reflection( *this );
	}
	template <class T>
	void ZZ(const BoolAsType_False,const BoolAsType_True,T * value)
	{
		value->Reflection( *this );
	}

	template <class T>
	void ZZ(const BoolAsType_True,const BoolAsType_False,T & value)
	{
		typename T::iterator it = value.begin();
		
		while( it != value.end() )
		{
			(*this)( "", *it );
			
			++it;
		}
	}

	template <class T>
	void operator() (const char * name,T & value)
	{
		UNUSED_PARAMETER( name );
		ZZ( TypeTraits<T>().isContainer,TypeTraits<T>().hasReflection,value);
	}

};

//============================================================
// Binary IO of members

struct IOMembers
{
	FileRCPtr m_file;

	IOMembers(FileRCPtr & _gf) : m_file(_gf)
	{		
	}

	template <class T>
	void ZZ(const BoolAsType_False,T & value)
	{
		m_file->IO(&value,sizeof(value));
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

};


//============================================================
// simple functor that makes any Reflection class work
//	in the Prefs IO.  Simply derive from Prefs and implement Reflection()

struct IOPrefs
{
	PrefBlock * m_pBlock;

	IOPrefs(PrefBlock * b) : m_pBlock(b)
	{		
	}

	template <class T>
	void operator() (const char * name,T & value)
	{
		m_pBlock->IO(name,&value);
	}
};

// really I'd like to use some sort of partial specialization so I could
//	select only classes that were Reflection-compatible here, but I can't do that
//	with VC6
template <class T>
void PrefIO(PrefBlock & block,T & me)
{
	IOPrefs iop(&block);
	me.Reflection(iop);
}

//============================================================

END_CB
