#pragma once

#include "cblib/Base.h"
#include "cblib/TypeTraits.h"
#include "cblib/Log.h"		// just for the PrintMembers functor
#include "cblib/File.h"		// just for the IOMembers functor
#include "cblib/PrefBlock.h" // just for the IOPrefs functor

START_CB

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
		parent_type::Reflection(functor);
		REFLECT(m_a);
		REFLECT(m_b);
	}

(if you are a derived class, call Reflection on your parent first)
(you can also put any other code you want in your Reflection member)

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

//============================================================

#define REFLECT(x)	functor(#x,x)

#define REFLECT_ARRAYN(what,count)	do{ for(int i=0;i<(count);i++) { char str[80]; sprintf(str,"%s%d",#what,i); functor(str,(what)[i]); } }while(0)

#define REFLECT_ARRAY(what)	REFLECT_ARRAYN(what,ARRAY_SIZE(what))
	
/**

For static arrays you can just REFLECT_ARRAY , like

	int stuff[4];
	
you can just REFLECT_ARRAY(stuff); and it will make labels "stuff0" , "stuff1", etc.

**/

//===============================================================================
/**

AUTO_REFLECT() special tag :

this create the Auto_Reflection function proto
it also is a trigger to the AutoReflect parser to do its thing

The AutoReflect parser creates :

	"file.aup"
	
with Auto_Reflection and Auto_SetDefaults

You should #include your .aup after your class generally, so it can see your protos.

if you use AUTO_REFLECT , then it is your responsibilty to call Auto_Reflection and Auto_SetDefaults when you want to

if you use AUTO_REFLECT_FULL , they are done for you

you mark a variable for inclusion in autoreflection with //$

NOTE : AUTO_REFLECT must be before the variables !

***/

#define AUTO_REFLECT(MyClass) \
	FORBID_CLASS_STANDARDS(MyClass); \
	template <class T> void Auto_Reflection(T & functor); \
	void Auto_SetDefaults();

#define AUTO_REFLECT_FULL(MyClass) \
	AUTO_REFLECT(MyClass) \
	template <class T> void Reflection(T & functor) { Auto_Reflection(functor); } \
	MyClass() { Auto_SetDefaults(); } \
	virtual ~MyClass() { }

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

};

//============================================================
// Binary IO of members

struct IOMembers
{
	FileRCPtr m_file;

	IOMembers(FileRCPtr & _gf) : m_file(_gf)
	{		
	}

	// IOMembers assumes a type can either be written as binary bits
	//	or it has a Reflection member
	// this is not really quite strong enough, you need IOZ at some point
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
