
//autoprintf.inl autogen on Aug 23 2010 12:14:31 to 20 max args

template < typename T1 >
inline String autoToStringSub( T1 arg1)
{
	return autoToStringFunc( 1,
			safeprintf_type(arg1), 
			arg1, 0 );
}

template < typename T1, typename T2 >
inline String autoToStringSub( T1 arg1, T2 arg2)
{
	return autoToStringFunc( 2,
			safeprintf_type(arg1), safeprintf_type(arg2), 
			arg1, arg2, 0 );
}

template < typename T1, typename T2, typename T3 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3)
{
	return autoToStringFunc( 3,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), 
			arg1, arg2, arg3, 0 );
}

template < typename T1, typename T2, typename T3, typename T4 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4)
{
	return autoToStringFunc( 4,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), 
			arg1, arg2, arg3, arg4, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
{
	return autoToStringFunc( 5,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), 
			arg1, arg2, arg3, arg4, arg5, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6)
{
	return autoToStringFunc( 6,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), 
			arg1, arg2, arg3, arg4, arg5, arg6, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7)
{
	return autoToStringFunc( 7,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8)
{
	return autoToStringFunc( 8,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9)
{
	return autoToStringFunc( 9,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10)
{
	return autoToStringFunc( 10,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11)
{
	return autoToStringFunc( 11,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), safeprintf_type(arg11), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12)
{
	return autoToStringFunc( 12,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), safeprintf_type(arg11), safeprintf_type(arg12), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13)
{
	return autoToStringFunc( 13,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), safeprintf_type(arg11), safeprintf_type(arg12), safeprintf_type(arg13), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14)
{
	return autoToStringFunc( 14,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), safeprintf_type(arg11), safeprintf_type(arg12), safeprintf_type(arg13), safeprintf_type(arg14), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15)
{
	return autoToStringFunc( 15,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), safeprintf_type(arg11), safeprintf_type(arg12), safeprintf_type(arg13), safeprintf_type(arg14), safeprintf_type(arg15), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16)
{
	return autoToStringFunc( 16,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), safeprintf_type(arg11), safeprintf_type(arg12), safeprintf_type(arg13), safeprintf_type(arg14), safeprintf_type(arg15), safeprintf_type(arg16), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16, typename T17 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16, T17 arg17)
{
	return autoToStringFunc( 17,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), safeprintf_type(arg11), safeprintf_type(arg12), safeprintf_type(arg13), safeprintf_type(arg14), safeprintf_type(arg15), safeprintf_type(arg16), safeprintf_type(arg17), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16, typename T17, typename T18 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16, T17 arg17, T18 arg18)
{
	return autoToStringFunc( 18,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), safeprintf_type(arg11), safeprintf_type(arg12), safeprintf_type(arg13), safeprintf_type(arg14), safeprintf_type(arg15), safeprintf_type(arg16), safeprintf_type(arg17), safeprintf_type(arg18), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, 0 );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16, typename T17, typename T18, typename T19 >
inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16, T17 arg17, T18 arg18, T19 arg19)
{
	return autoToStringFunc( 19,
			safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4), safeprintf_type(arg5), safeprintf_type(arg6), safeprintf_type(arg7), safeprintf_type(arg8), safeprintf_type(arg9), safeprintf_type(arg10), safeprintf_type(arg11), safeprintf_type(arg12), safeprintf_type(arg13), safeprintf_type(arg14), safeprintf_type(arg15), safeprintf_type(arg16), safeprintf_type(arg17), safeprintf_type(arg18), safeprintf_type(arg19), 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19, 0 );
}

template < typename T1 >
inline String autoToString( T1 arg1)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ));
}

template < typename T1, typename T2 >
inline String autoToString( T1 arg1, T2 arg2)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ));
}

template < typename T1, typename T2, typename T3 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ));
}

template < typename T1, typename T2, typename T3, typename T4 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ), autoprintf_StringToChar( autoArgConvert(arg11) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ), autoprintf_StringToChar( autoArgConvert(arg11) ), autoprintf_StringToChar( autoArgConvert(arg12) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ), autoprintf_StringToChar( autoArgConvert(arg11) ), autoprintf_StringToChar( autoArgConvert(arg12) ), autoprintf_StringToChar( autoArgConvert(arg13) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ), autoprintf_StringToChar( autoArgConvert(arg11) ), autoprintf_StringToChar( autoArgConvert(arg12) ), autoprintf_StringToChar( autoArgConvert(arg13) ), autoprintf_StringToChar( autoArgConvert(arg14) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ), autoprintf_StringToChar( autoArgConvert(arg11) ), autoprintf_StringToChar( autoArgConvert(arg12) ), autoprintf_StringToChar( autoArgConvert(arg13) ), autoprintf_StringToChar( autoArgConvert(arg14) ), autoprintf_StringToChar( autoArgConvert(arg15) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ), autoprintf_StringToChar( autoArgConvert(arg11) ), autoprintf_StringToChar( autoArgConvert(arg12) ), autoprintf_StringToChar( autoArgConvert(arg13) ), autoprintf_StringToChar( autoArgConvert(arg14) ), autoprintf_StringToChar( autoArgConvert(arg15) ), autoprintf_StringToChar( autoArgConvert(arg16) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16, typename T17 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16, T17 arg17)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ), autoprintf_StringToChar( autoArgConvert(arg11) ), autoprintf_StringToChar( autoArgConvert(arg12) ), autoprintf_StringToChar( autoArgConvert(arg13) ), autoprintf_StringToChar( autoArgConvert(arg14) ), autoprintf_StringToChar( autoArgConvert(arg15) ), autoprintf_StringToChar( autoArgConvert(arg16) ), autoprintf_StringToChar( autoArgConvert(arg17) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16, typename T17, typename T18 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16, T17 arg17, T18 arg18)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ), autoprintf_StringToChar( autoArgConvert(arg11) ), autoprintf_StringToChar( autoArgConvert(arg12) ), autoprintf_StringToChar( autoArgConvert(arg13) ), autoprintf_StringToChar( autoArgConvert(arg14) ), autoprintf_StringToChar( autoArgConvert(arg15) ), autoprintf_StringToChar( autoArgConvert(arg16) ), autoprintf_StringToChar( autoArgConvert(arg17) ), autoprintf_StringToChar( autoArgConvert(arg18) ));
}

template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15, typename T16, typename T17, typename T18, typename T19 >
inline String autoToString( T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12, T13 arg13, T14 arg14, T15 arg15, T16 arg16, T17 arg17, T18 arg18, T19 arg19)
{
	return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ), autoprintf_StringToChar( autoArgConvert(arg4) ), autoprintf_StringToChar( autoArgConvert(arg5) ), autoprintf_StringToChar( autoArgConvert(arg6) ), autoprintf_StringToChar( autoArgConvert(arg7) ), autoprintf_StringToChar( autoArgConvert(arg8) ), autoprintf_StringToChar( autoArgConvert(arg9) ), autoprintf_StringToChar( autoArgConvert(arg10) ), autoprintf_StringToChar( autoArgConvert(arg11) ), autoprintf_StringToChar( autoArgConvert(arg12) ), autoprintf_StringToChar( autoArgConvert(arg13) ), autoprintf_StringToChar( autoArgConvert(arg14) ), autoprintf_StringToChar( autoArgConvert(arg15) ), autoprintf_StringToChar( autoArgConvert(arg16) ), autoprintf_StringToChar( autoArgConvert(arg17) ), autoprintf_StringToChar( autoArgConvert(arg18) ), autoprintf_StringToChar( autoArgConvert(arg19) ));
}
