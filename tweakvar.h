#pragma once

#include "Base.h"
#include "Util.h"
#include "RefCounted.h"
#include "String.h"
#include "PrefBlock.h"
#include "cblib_config.h"

START_CB

/*************

	TweakVar ; this is the variables in your C file = editable in line
		this is a Casey-ism

	see usage at the bottom of this file ; no need for you to see the guts

	the only two interfaces you should call are :
	
	TWEAK_VAL
	TweakVar_CheckChanges

if you turn off CBLIB_TWEAKVAR_ENABLED they just become plain old C variables 
	(actually they're always plain variables, it just turns off the registration)

TweakVar parses C++ comments right, so you can do things like :

	// TWEAK_VAL(float, s_myValF, 1.37f);
	TWEAK_VAL(float, s_myValF, 1.77f);

and it does what you expect.

CheckChanges is polling.  You can either call it periodically, like :

	DO_ONCE_EVERY( TweakVar_CheckChanges(), 1.0 );

or you could use a DirChangeWatcher to only check changes when you see a disk touch.

CheckChanges intentionally reparses all files at startup.  This lets your app see
changes that occured while it was not running - it makes the source code like a "pref".

Vars are found by *name* not by file/line , so it's robust.  __FILE__ is used to know
what file to look in to find a var.

You can put TWEAK_VALs either at file scope or local scope.
If they are in local scope they won't get registered until that scope is entered.
So you may see errors about tweak values not being recognized.  That's fine.

*************/


#ifndef CBLIB_TWEAKVAR_ENABLED
#define CBLIB_TWEAKVAR_ENABLED 1
#endif


//---------------------------------------------------
#if CBLIB_TWEAKVAR_ENABLED

//
// TweakVarSingleton holds a list of the registered TweakVars in by file name

class TweakVarBase;
//class TweakVarSingletonImpl;

class TweakVarSingleton
{
public:

	static TweakVarSingleton * The();
	static void DestroyThe();
	
	//void RegisterFile(const char * file);
	
	// Register must be called by TWEAK_VAL
	void RegisterBase( const char * file, TweakVarBase * pBase );
	
	template <typename T>
	T Register( const char * file, TweakVarBase * pBase , T initVal)
	{
		RegisterBase(file,pBase);
		return initVal;
	}
	
	void GetDirsToWatch(vector<String> * pInto);
	
	// CheckChanges looks for modtime changes on the tweaker-registered files
	bool CheckChanges();

	void LogAll();
	bool SetFromText( const char * varname, const char * value );

protected:

	TweakVarSingleton();
	virtual ~TweakVarSingleton();	

};

//---------------------------------------------------
//
// TweakVar is a template adapter to get at ReadFromText<type>
//	(you should not interact with this class at all)
//	the "val" portion inside the TWEAK_VAL macro must be parsable by ReadFromText

class TweakVarBase : public RefCounted
{
public:
	virtual ~TweakVarBase() { }
	virtual void SetFromText(const char * const text) = 0;
	virtual const char * GetName() = 0;
	virtual void GetAsText(String * pInto) = 0;
};

template <class T>
class TweakVar : public TweakVarBase
{
public:
	typedef TweakVar<T>	this_type;
	
	explicit TweakVar(T * ptr,const char * name) : m_ptr(ptr), m_name(name)
	{
	}
	explicit TweakVar(const this_type & other) : m_ptr(other.m_ptr), m_name(other.m_name)
	{
	}
	void operator = (const this_type & other)
	{
		m_ptr = other.m_ptr;
		m_name = other.m_name;
	}
	
	virtual void SetFromText(const char * const text)
	{
		ReadFromText(m_ptr,text);
	}	
	
	virtual void GetAsText(String * pInto)
	{
		WriteToText(*m_ptr,pInto);
	}	
	
	virtual const char * GetName()
	{
		return m_name;
	}
	
	T * m_ptr;
	const char * m_name;
};

//---------------------------------------------------
// parser looks for "TWEAK_VAL" :
//
// the *pointer* to var is stored, so it must be static
//	when a changed var is found I cram it straight into &var
//
// NOTE : the var is not registered until the code is stepped over once
//	I set up Register() as a value pass-through so that I can put these in file-scope as well
// constructing type on val in Register is needed for String
//	-> nah that was dumb and illegal using the address of something that's not mad yet

//	static type var = NS_CB::TweakVarSingleton::The()->Register<type>( __FILE__, new NS_CB::TweakVar<type>((type *)&var,#var) , type(val) )

#define TWEAK_VAL(type, var, val ) \
	static type var = type(val); \
	static type var##tweakmaker = NS_CB::TweakVarSingleton::The()->Register<type>( __FILE__, new NS_CB::TweakVar<type>((type *)&var,#var) , type(val) )

// examples:
//
// TWEAK_VAL(int, s_myValI, 1);
// TWEAK_VAL(float, s_myValF, 1.37f);
// TWEAK_VAL(double, s_myValD, 1.3710);
// TWEAK_VAL(bool, s_myValB, true);
// TWEAK_VAL(String, s_testVal6, "yeah word up" );	

// bool says if any files changed : (not necessarilly any vars really changed)
static inline bool TweakVar_CheckChanges() { return NS_CB::TweakVarSingleton::The()->CheckChanges(); }

// TweakVar_CheckChanges does hit disk for modtimes
//	so you could probably run it once very few seconds or something
//	not every frame
//	or use a DirChangeWatcher to only run it when a change is detected

// StartDirWatcher is optional
//	it starts a DirChangeWatcher to make CheckChanges more efficient
//	the DirChangeWatcher is automatically queried when you CheckChanges

static inline void TweakVar_GetDirsToWatch(vector<String> * pInto) { NS_CB::TweakVarSingleton::The()->GetDirsToWatch(pInto); }
	
static inline void TweakVar_Shutdown() { NS_CB::TweakVarSingleton::The()->DestroyThe(); }

static inline void TweakVar_LogAll() { NS_CB::TweakVarSingleton::The()->LogAll(); }

static inline bool TweakVar_SetFromText(const char * var, const char * val) { return NS_CB::TweakVarSingleton::The()->SetFromText(var,val); }
	
//---------------------------------------------------

#else // CBLIB_TWEAKVAR_ENABLED

#define TWEAK_VAL(type, var, val )	static type var(val)

// bool says if any files changed : (not necessarilly any vars really changed)
static inline bool TweakVar_CheckChanges() { return false; }
static inline void TweakVar_GetDirsToWatch(vector<String> * pInto) { }
static inline void TweakVar_Shutdown() { }

#endif // CBLIB_TWEAKVAR_ENABLED

//---------------------------------------------------

END_CB
