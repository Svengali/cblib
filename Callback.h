#pragma once

#include "cblib/RefCounted.h"
#include "cblib/SPtr.h"

START_CB

SPtrFwd(Callback);

#pragma warning(push)
#pragma warning(disable : 4121)// alignment of a member was sensitive to packing

//========================================================================================
// CallbackQueue for executing callbacks at a given time

namespace CallbackQueue
{
	void Add(CallbackPtr cb);
	
	void Tick();
};

//========================================================================================
//
//	Callback : 
//	generic callback mechanism
//
//	will call back to class member functions
//
//	All users should juse use the "make_cb" functions, don't look at the rest of this
//
//	Because of the templating, Callback holds the pointer-type that YOU PASS IN
//
//	YOU ALMOST ALWAYS WANT TO USE A WEAK POINTER !!!!
//
//	If you pass in a Smart Pointer, the Callback will hold a ref !!
//
//-----------------------------------------------------
//
// for CALLBACK_ON_DESTRUCT :
//	the callbacks are destructed in reverse order they're made, which is what you want
//	so you can do like :
//		ptr = malloc();
//		CallbackPtr cb1 = make_cb( free, ptr, CALLBACK_ON_DESTRUCT );
//		junk = dostuff(ptr);
//		CallBackPtr cb2 = make_cb( freestuff, junk, CALLBACK_ON_DESTRUCT );
//	and the call order will be cb2, then cb1
//
// one cool thing about CALLBACK_ON_DESTRUCT is that since it is a smart pointer you can actually
//	pass it around with the object it cleans up, and it will do the work when all refs go away
//
//========================================================================================

#define CALLBACK_ON_DESTRUCT	(-128.0)

class __declspec(novtable) Callback : public RefCounted
{
public:

	virtual ~Callback() { }

	virtual void Do() = 0;

	double GetTime() const { return m_time; }

	// be careful when changing times; it may screw up whoever has you
	//	sorted
	void ChangeTime(double time) { m_time = time; }

protected:
	explicit Callback(double when) : m_time(when) { }

	double	m_time;
private:
	FORBID_CLASS_STANDARDS(Callback);
};

//---
// less() type operator for use in sorting Callback by time :
// Aaron: actually greater, not less. This is used to make a heap somewhere, and we
// want that heap to have the smallest values on top. To do that, we gotta do this. 
struct CompareCallbackPtrTimes
{
	bool operator() (const CallbackPtr & cb1,const CallbackPtr & cb2)
	{
		return (cb1->GetTime() > cb2->GetTime());
	}
};

//========================================================================================

template < typename T_fun_type >
class CallbackG0 : public Callback
{
public:
	
	explicit CallbackG0(T_fun_type f, double when) : Callback(when)
	{
		ASSERT( f != NULL );
		_fun = f;
	}

	~CallbackG0() { if ( m_time == CALLBACK_ON_DESTRUCT ) Do(); }

	virtual void Do()
	{
		(*_fun)();
	}


	T_fun_type	_fun;
};

template < typename T_fun_type, typename Arg1 >
class CallbackG1 : public Callback
{
public:
	
	explicit CallbackG1(T_fun_type f, Arg1 a1, double when) : Callback(when)
	{
		ASSERT( f != NULL );
		_fun = f;
		_arg1 = a1;
	}

	~CallbackG1() { if ( m_time == CALLBACK_ON_DESTRUCT ) Do(); }
	
	virtual void Do()
	{
		(*_fun)(_arg1);
	}


	T_fun_type	_fun;
	Arg1		_arg1;
};

template < typename T_fun_type, typename Arg1, typename Arg2 >
class CallbackG2 : public Callback
{
public:
	
	explicit CallbackG2(T_fun_type f, Arg1 a1, Arg2 a2, double when) : Callback(when)
	{
		ASSERT( f != NULL );
		_fun = f;
		_arg1 = a1;
		_arg2 = a2;
	}

	~CallbackG2() { if ( m_time == CALLBACK_ON_DESTRUCT ) Do(); }
	
	virtual void Do()
	{
		(*_fun)(_arg1,_arg2);
	}


	T_fun_type	_fun;
	Arg1		_arg1;
	Arg2		_arg2;
};

template < typename T_fun_type, typename Arg1, typename Arg2, typename Arg3 >
class CallbackG3 : public Callback
{
public:
	
	explicit CallbackG3(T_fun_type f, Arg1 a1, Arg2 a2, Arg3 a3, double when) : Callback(when)
	{
		ASSERT( f != NULL );
		_fun = f;
		_arg1 = a1;
		_arg2 = a2;
		_arg3 = a3;
	}

	~CallbackG3() { if ( m_time == CALLBACK_ON_DESTRUCT ) Do(); }
	
	virtual void Do()
	{
		(*_fun)(_arg1,_arg2,_arg3);
	}


	T_fun_type	_fun;
	Arg1		_arg1;
	Arg2		_arg2;
	Arg3		_arg3;
};

template < typename T_fun_type, typename Arg1, typename Arg2, typename Arg3, typename Arg4 >
class CallbackG4 : public Callback
{
public:
	
	explicit CallbackG4(T_fun_type f, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, double when) : Callback(when)
	{
		ASSERT( f != NULL );
		_fun = f;
		_arg1 = a1;
		_arg2 = a2;
		_arg3 = a3;
		_arg4 = a4;
	}

	~CallbackG4() { if ( m_time == CALLBACK_ON_DESTRUCT ) Do(); }
	
	virtual void Do()
	{
		(*_fun)(_arg1,_arg2,_arg3,_arg4);
	}


	T_fun_type	_fun;
	Arg1		_arg1;
	Arg2		_arg2;
	Arg3		_arg3;
	Arg4		_arg4;
};
//========================================================================================

template < class T_ClassPtr, typename T_fun_type >
class CallbackM0 : public Callback
{
public:
	
	explicit CallbackM0(T_ClassPtr c, T_fun_type f, double when) : Callback(when)
	{
		ASSERT( c != NULL && f != NULL );
		__p = c;
		_mem_fun = f;
	}

	~CallbackM0() { if ( m_time == CALLBACK_ON_DESTRUCT ) Do(); }
	
	virtual void Do()
	{
		// must check for __p going to null since it could be a WeakPtr
		if ( __p != NULL )
		{
			(GetPtr(__p)->*_mem_fun)();
			//(__p->*_mem_fun)();
			//T_Class * p = GetPtr(__p);
			//(p->*_mem_fun)();
			//(operator->(__p))->*_mem_fun)();
		}
	}


	T_ClassPtr	__p;
	T_fun_type	_mem_fun;
};

template < class T_ClassPtr , typename T_fun_type, typename Arg1 >
class CallbackM1 : public Callback
{
public:
	
	explicit CallbackM1(T_ClassPtr c, T_fun_type f, Arg1 a1, double when) : Callback(when)
	{
		ASSERT( c != NULL && f != NULL );
		__p = c;
		_mem_fun = f;
		_arg1 = a1;
	}

	~CallbackM1() { if ( m_time == CALLBACK_ON_DESTRUCT ) Do(); }

	virtual void Do()
	{
		if ( __p != NULL )
		{
			(GetPtr(__p)->*_mem_fun)(_arg1);
		}
	}


	T_ClassPtr	__p;
	T_fun_type	_mem_fun;
	Arg1		_arg1;
};

template < class T_ClassPtr , typename T_fun_type, typename Arg1 , typename Arg2 >
class CallbackM2 : public Callback
{
public:
	
	explicit CallbackM2(T_ClassPtr c, T_fun_type f, Arg1 a1 , Arg2 a2, double when) : Callback(when)
	{
		ASSERT( c != NULL && f != NULL );
		__p = c;
		_mem_fun = f;
		_arg1 = a1;
		_arg2 = a2;
	}

	~CallbackM2() { if ( m_time == CALLBACK_ON_DESTRUCT ) Do(); }

	virtual void Do()
	{
		if ( __p != NULL )
		{
			(GetPtr(__p)->*_mem_fun)(_arg1,_arg2);
		}
	}


	T_ClassPtr	__p;
	T_fun_type	_mem_fun;
	Arg1		_arg1;
	Arg2		_arg2;
};

template < class T_ClassPtr , typename T_fun_type, typename Arg1 , typename Arg2, typename Arg3 >
class CallbackM3 : public Callback
{
public:
	
	explicit CallbackM3(T_ClassPtr c, T_fun_type f, Arg1 a1 , Arg2 a2, Arg3 a3, double when) : Callback(when)
	{
		ASSERT( c != NULL && f != NULL );
		__p = c;
		_mem_fun = f;
		_arg1 = a1;
		_arg2 = a2;
		_arg3 = a3;
	}

	~CallbackM3() { if ( m_time == CALLBACK_ON_DESTRUCT ) Do(); }

	virtual void Do()
	{
		if ( __p != NULL )
		{
			(GetPtr(__p)->*_mem_fun)(_arg1,_arg2,_arg3);
		}
	}


	T_ClassPtr	__p;
	T_fun_type	_mem_fun;
	Arg1		_arg1;
	Arg2		_arg2;
	Arg3		_arg3;
};

//========================================================================================

template < typename T_fun_type >
CallbackPtr make_cb(T_fun_type f, double when)
{
	return CallbackPtr( new CallbackG0<T_fun_type>(f,when) );
}

template < typename T_fun_type , typename Arg1 >
CallbackPtr make_cb(T_fun_type f,Arg1 a, double when)
{
	return CallbackPtr( new CallbackG1<T_fun_type,Arg1>(f,a,when) );
}

template < typename T_fun_type, typename Arg1, typename Arg2 >
CallbackPtr make_cb(T_fun_type f,Arg1 a1,Arg2 a2, double when)
{
	return CallbackPtr( new CallbackG2<T_fun_type,Arg1,Arg2>(f,a1,a2,when) );
}

template < typename T_fun_type, typename Arg1, typename Arg2, typename Arg3 >
CallbackPtr make_cb(T_fun_type f,Arg1 a1,Arg2 a2,Arg3 a3, double when)
{
	return CallbackPtr( new CallbackG3<T_fun_type,Arg1,Arg2,Arg3>(f,a1,a2,a3,when) );
}

template < typename T_fun_type, typename Arg1, typename Arg2, typename Arg3, typename Arg4 >
CallbackPtr make_cb(T_fun_type f,Arg1 a1,Arg2 a2,Arg3 a3,Arg4 a4, double when)
{
	return CallbackPtr( new CallbackG4<T_fun_type,Arg1,Arg2,Arg3,Arg4>(f,a1,a2,a3,a4,when) );
}

//========================================================================================
// make_cbm : for member functions
//	NOTEZ : T_ClassPtr should generally be a WeakPtr !!

template < class T_ClassPtr , typename T_fun_type >
CallbackPtr make_cbm(T_ClassPtr c, T_fun_type f, double when)
{
	return CallbackPtr( new CallbackM0<T_ClassPtr,T_fun_type>(c,f,when) );
}

template < class T_ClassPtr , typename T_fun_type , typename Arg1 >
CallbackPtr make_cbm(T_ClassPtr c, T_fun_type f,Arg1 a, double when)
{
	return CallbackPtr( new CallbackM1<T_ClassPtr,T_fun_type,Arg1>(c,f,a,when) );
}

template < class T_ClassPtr, typename T_fun_type, typename Arg1, typename Arg2 >
CallbackPtr make_cbm(T_ClassPtr c, T_fun_type f,Arg1 a1,Arg2 a2, double when)
{
	return CallbackPtr( new CallbackM2<T_ClassPtr,T_fun_type,Arg1,Arg2>(c,f,a1,a2,when) );
}

template < class T_ClassPtr, typename T_fun_type, typename Arg1, typename Arg2, typename Arg3 >
CallbackPtr make_cbm(T_ClassPtr c, T_fun_type f,Arg1 a1,Arg2 a2,Arg3 a3, double when)
{
	return CallbackPtr( new CallbackM3<T_ClassPtr,T_fun_type,Arg1,Arg2,Arg3>(c,f,a1,a2,a3,when) );
}

//========================================================================================
//
// ON_DESTRUCT calls a func with args when it goes out of scope
//	if you just want to run some local code, you can use AT_SHUTDOWN,
//		eg. :
//	ON_DESTRUCT( free, ptr );
//	AT_SHUTDOWN(  puts("doing shutdown junk") );

#define ON_DESTRUCT0(func)		CallbackPtr NUMBERNAME(OnDestruct_) = make_cb(func, CALLBACK_ON_DESTRUCT)
#define ON_DESTRUCT1(func,a1)	CallbackPtr NUMBERNAME(OnDestruct_) = make_cb(func,a1, CALLBACK_ON_DESTRUCT)
#define ON_DESTRUCT2(func,a1,a2)	CallbackPtr NUMBERNAME(OnDestruct_) = make_cb(func,a1,a2, CALLBACK_ON_DESTRUCT)
#define ON_DESTRUCT3(func,a1,a2,a3)	CallbackPtr NUMBERNAME(OnDestruct_) = make_cb(func,a2,a2,a3, CALLBACK_ON_DESTRUCT)
#define ON_DESTRUCT4(func,a1,a2,a3,a4)	CallbackPtr NUMBERNAME(OnDestruct_) = make_cb(func,a1,a2,a3,a4, CALLBACK_ON_DESTRUCT)

//========================================================================================

#pragma warning(pop)

END_CB
