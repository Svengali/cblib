
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt)
{
	SPI_CALLRAW ( SPI_CALLARG  );
}

template < typename t1 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt, t1 arg1)
{
	TRY
	{
		checkarg(fmt,fmt,arg1);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template < typename t1 , typename t2 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt, t1 arg1, t2 arg2)
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1 , 
			typename t2, 
			typename t3  >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt, t1 arg1, t2 arg2, t3 arg3)
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt, t1 arg1, t2 arg2, t3 arg3, t4 arg4)
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5)
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5, 
			typename t6 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, 
										t6 arg6)
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		next = checkarg(fmt,next,arg6);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5,arg6);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5, 
			typename t6, 
			typename t7 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, 
										t6 arg6, t7 arg7)
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		next = checkarg(fmt,next,arg6);
		next = checkarg(fmt,next,arg7);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5,arg6,arg7);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5, 
			typename t6, 
			typename t7, 
			typename t8 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, 
										t6 arg6, t7 arg7, t8 arg8)
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		next = checkarg(fmt,next,arg6);
		next = checkarg(fmt,next,arg7);
		next = checkarg(fmt,next,arg8);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5, 
			typename t6, 
			typename t7, 
			typename t8, 
			typename t9 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, 
										t6 arg6, t7 arg7, t8 arg8, t9 arg9)
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		next = checkarg(fmt,next,arg6);
		next = checkarg(fmt,next,arg7);
		next = checkarg(fmt,next,arg8);
		next = checkarg(fmt,next,arg9);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}


template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5, 
			typename t6, 
			typename t7, 
			typename t8, 
			typename t9, 
			typename t10 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, 
										t6 arg6, t7 arg7, t8 arg8, t9 arg9, 
										t10 arg10 )
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		next = checkarg(fmt,next,arg6);
		next = checkarg(fmt,next,arg7);
		next = checkarg(fmt,next,arg8);
		next = checkarg(fmt,next,arg9);
		next = checkarg(fmt,next,arg10);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5, 
			typename t6, 
			typename t7, 
			typename t8, 
			typename t9, 
			typename t10, 
			typename t11 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, 
										t6 arg6, t7 arg7, t8 arg8, t9 arg9, 
										t10 arg10, t11 arg11 )
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		next = checkarg(fmt,next,arg6);
		next = checkarg(fmt,next,arg7);
		next = checkarg(fmt,next,arg8);
		next = checkarg(fmt,next,arg9);
		next = checkarg(fmt,next,arg10);
		next = checkarg(fmt,next,arg11);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5, 
			typename t6, 
			typename t7, 
			typename t8, 
			typename t9, 
			typename t10, 
			typename t11, 
			typename t12 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, 
										t6 arg6, t7 arg7, t8 arg8, t9 arg9, 
										t10 arg10, t11 arg11, t12 arg12 )
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		next = checkarg(fmt,next,arg6);
		next = checkarg(fmt,next,arg7);
		next = checkarg(fmt,next,arg8);
		next = checkarg(fmt,next,arg9);
		next = checkarg(fmt,next,arg10);
		next = checkarg(fmt,next,arg11);
		next = checkarg(fmt,next,arg12);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11,arg12);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5, 
			typename t6, 
			typename t7, 
			typename t8, 
			typename t9, 
			typename t10, 
			typename t11, 
			typename t12, 
			typename t13 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, 
										t6 arg6, t7 arg7, t8 arg8, t9 arg9, 
										t10 arg10, t11 arg11, t12 arg12, t13 arg13 )
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		next = checkarg(fmt,next,arg6);
		next = checkarg(fmt,next,arg7);
		next = checkarg(fmt,next,arg8);
		next = checkarg(fmt,next,arg9);
		next = checkarg(fmt,next,arg10);
		next = checkarg(fmt,next,arg11);
		next = checkarg(fmt,next,arg12);
		next = checkarg(fmt,next,arg13);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11,arg12,arg13);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}

template <	typename t1, 
			typename t2, 
			typename t3, 
			typename t4, 
			typename t5, 
			typename t6, 
			typename t7, 
			typename t8, 
			typename t9, 
			typename t10, 
			typename t11, 
			typename t12, 
			typename t13, 
			typename t14 >
inline SPI_SAFEDECL ( SPI_PREARG const char * fmt,t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, 
										t6 arg6, t7 arg7, t8 arg8, t9 arg9, 
										t10 arg10, t11 arg11, t12 arg12, t13 arg13, t14 arg14 )
{
	TRY
	{
		const char * next = fmt;
		next = checkarg(fmt,next,arg1);
		next = checkarg(fmt,next,arg2);
		next = checkarg(fmt,next,arg3);
		next = checkarg(fmt,next,arg4);
		next = checkarg(fmt,next,arg5);
		next = checkarg(fmt,next,arg6);
		next = checkarg(fmt,next,arg7);
		next = checkarg(fmt,next,arg8);
		next = checkarg(fmt,next,arg9);
		next = checkarg(fmt,next,arg10);
		next = checkarg(fmt,next,arg11);
		next = checkarg(fmt,next,arg12);
		next = checkarg(fmt,next,arg13);
		next = checkarg(fmt,next,arg14);
		SPI_CALLRAW ( SPI_CALLARG  ,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11,arg12,arg13,arg14);
	}
	CATCH
	{
		SPI_BADRETURN
	}	
}
