#pragma once

#include "Reflection.h"
#include "PrefBlock.h"
#include "String.h"
#include "SPtr.h"
#include "FileUtil.h"

#include <crtdefs.h>
//typedef long time_t;

START_CB

/*

Prefs is a tiny wrapper on RefCounted ; make Pref structs derive from Prefs
so they can be shared by name in the manager.

-----------------------

WARNING : All prefs should fill themselves with default values before
	trying to Read() ; if the pref read fails, the members will not be touched

See more notes in PrefBlock.h

The primary usage is PrefsMgr::Get()

*/

//===============================================================================
// Dispatcher mechanism (clients ignore this)
//  this lets us get back to type-specialized functions from
//	the generic Prefs class

SPtrFwd(Prefs);
SPtrFwd(PrefDispatcher);

class PrefDispatcher : public RefCounted
{
public:
	PrefDispatcher() { }
	virtual void IO(PrefBlock & block,Prefs * prefs) const = 0;
	virtual const char * GetTypeName() const = 0;
	virtual PrefsPtr Factory() const = 0;
private:
	FORBID_CLASS_STANDARDS(PrefDispatcher);
};

template <class T>
struct PrefDispatcherTyped : public PrefDispatcher
{
public:
	//typedef void (*PrefIOPtr)(PrefBlock & block,T & me);

	PrefDispatcherTyped() //PrefIOPtr func)
	{
		//m_func = func;
	}

	PrefsPtr Factory() const
	{
		return PrefsPtr( new T );
	}

	const char * GetTypeName() const
	{
		return type_name( typeid(T) );
	}

	virtual void IO(PrefBlock & block,Prefs * prefs) const
	{
		//ASSERT( m_func != NULL );
		T * typed = dynamic_cast<T *>(prefs);
		ASSERT( typed != NULL );
		
		// write the type name as the top level label in case I want to do factories later
		const char * label = GetTypeName();
		block.IO(label,typed);
	}

	//PrefIOPtr	m_func;
private:
	FORBID_CLASS_STANDARDS(PrefDispatcherTyped);
};

//===============================================================================

class Prefs : public RefCounted
{
public:
	
	void SetDispatcher(PrefDispatcher * dispatcher) { m_dispatcher = dispatcher; }
	PrefDispatcher * GetDispatcher() const { return m_dispatcher.GetPtr(); }

	void SetResourceName(const char * const name) { m_resourceName = name; }
	const char * const GetResourceName() const { return m_resourceName.CStr(); }

	time_t GetLoadTime() { return m_loadTime; }
	void SetLoadTime(time_t t) { m_loadTime = t; }

	void Reload();
	void Save();

protected:
	Prefs() { }
private:
	String				m_resourceName;
	PrefDispatcherPtr	m_dispatcher;
	time_t				m_loadTime;
	FORBID_COPY_CTOR(	Prefs);
	FORBID_ASSIGNMENT(	Prefs);
};

namespace PrefsMgr
{
	//-----------------------------------------
	// maintenance :

	void Flush();
	void Shutdown();
	bool ReloadChanged(); // returns if any changed
	void ReloadAll();
	void SaveAll();

	void GetDirsToWatch(vector<String> * pInto);
		
	//-----------------------------------------
	// internal use only :

	Prefs * GetExisting(const char * resourceName);
	void Add(const PrefsPtr & pref,const char * resourceName,bool doAutoSave);

	//-----------------------------------------
	// templated Get() is the primary accessor

	template <class T>
	SPtr<T>	Get(const char * resourceName)
	{
		SPtr<T> ret;
		Prefs * p = GetExisting(resourceName);
		if ( p != NULL )
		{
			T * casted = dynamic_cast<T *>(p);
			ASSERT( casted != NULL );
			ret = casted;
			return ret;
		}

		// not found, read it
		ret = new T;

		// remember the Dispatcher for later :
		//PrefDispatcher<T>::PrefIOPtr ptr = PrefIO;
		PrefDispatcherTyped<T> * dispatcher = new PrefDispatcherTyped<T>(); //ptr);
		ret->SetDispatcher(dispatcher);

		PrefBlock block(PrefBlock::eRead,resourceName);

		dispatcher->IO(block,ret.GetPtr());
		//block.IO("prefs",ret.GetPtr());

		Add(PrefsPtr(ret.GetPtr()),resourceName,true);

		return ret;
	}
	
	template <class T>
	SPtr<T>	CreateNull()
	{
		SPtr<T> ret(new T);

		// remember the Dispatcher for later :
		//PrefDispatcher<T>::PrefIOPtr ptr = PrefIO;
		PrefDispatcherTyped<T> * dispatcher = new PrefDispatcherTyped<T>(); //ptr);
		ret->SetDispatcher(dispatcher);
		
		return ret;
	}
	
};

//===============================================================================

END_CB
