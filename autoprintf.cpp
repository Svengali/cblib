#include "autoprintf.h"
#include "FileUtil.h"
#include "Profiler.h"
#include "vector.h"
#include <stdarg.h>

START_CB

#define APF_MAX_ARGS	(20)

// "%n" is not an output, it stores the # of chars written so far
//	I don't support that right yet

//=======================================================================
// configure how wchars are treated :

static bool s_wcharAnsi = false; // output to Console CP is default

void autoPrintfSetWCharAnsi(bool ansi) // el
{
	s_wcharAnsi = ansi;
}

String autoPrintfWChar(const wchar_t * ws)
{
	if ( s_wcharAnsi)
		return UnicodeToAnsi(ws);
	else
		return UnicodeToConsole(ws);
}

//=======================================================================
// implementation :

static inline const char * safeprintf_fmtForType(ESafePrintfType fmtType)
{
	switch(fmtType)
	{
	case safeprintf_int32 : return "d";
	case safeprintf_int64 : 
		return FMT_I64 "d";
	case safeprintf_uint32 : return "u";
	case safeprintf_uint64 : 
		return FMT_I64 "u";
	case safeprintf_float : return "f";
	//case safeprintf_ptrint : return "n"; // do NOT return here - intentional failure
	case safeprintf_ptrvoid : return "p";
	case safeprintf_wcharptr : return "S";
	case safeprintf_charptr : return "s";
	default : 
		safeprintf_throwerror("","",fmtType,fmtType);
		return "";
	}
}

static inline String DoAutoType(const char *fmt_base,int & numArgs, const ESafePrintfType * pArgTypes )
{
	String ret(fmt_base);

	// @@ could have a fast variant if there are no %a's, I can return fmt_base unchanged

	numArgs = 0;
	
	// need a bit of extra space cuz "%a" and turn into longer things :
	char * ptr = ret.WriteableCStr( ret.Length() + APF_MAX_ARGS * 4 );
	while(ptr)
	{
		const char * p = ptr;
		ESafePrintfType fmttype = safeprintf_findfmtandadvance(&p);
		ptr = const_cast<char *>(p);
		
		// it's perfectly valid to just not have %'s for your args
		if ( fmttype == safeprintf_none )
		{
			return ret;
		}
		else if ( ptr == NULL )
		{
			safeprintf_throwerror(fmt_base,ptr,fmttype,safeprintf_none);
			return ret;
		}
		
		ESafePrintfType argtype = pArgTypes[ numArgs ];
		numArgs += 1;
		
		if ( fmttype == safeprintf_unknown )
		{
			if ( tolower(ptr[-1]) == 'a' )
			{
				// autotype :
				if ( argtype <= safeprintf_unknown )
				{
					safeprintf_throwerror(fmt_base,ptr,fmttype,argtype);
				}
			
				const char * insFmt = safeprintf_fmtForType(argtype);
				// change 'a' to insFmt :
				int insLen = strlen32(insFmt);
				if ( insLen == 1 )
				{
					ptr[-1] = insFmt[0];
				}
				else
				{
					char * insAt = ptr-1;
					memmove(insAt+insLen,ptr,strlen(ptr)+1);
					memcpy(insAt,insFmt,insLen);
					ptr = insAt + insLen;
				}
			}
			else
			{
				// syntax error failure (failed to parse a type from % in fmt)
				safeprintf_throwsyntaxerror(fmt_base,ptr);
			}
		}
		else
		{
			// check type :
			if ( fmttype != argtype )
			{
				safeprintf_throwerror(fmt_base,ptr,fmttype,argtype);
			}
		}
	}
	
	return ret;
}

//===============================================================

static inline void SkipVAArg(ESafePrintfType argtype, va_list & vl)
{
	switch(argtype)
	{
	case safeprintf_charptr:	{ char * arg = va_arg(vl,char *); REFERENCE_TO_VARIABLE(arg); return; }
	case safeprintf_wcharptr:	{ wchar_t * arg = va_arg(vl,wchar_t *); REFERENCE_TO_VARIABLE(arg); return; }
	case safeprintf_int32:		{ int arg = va_arg(vl,int); REFERENCE_TO_VARIABLE(arg); return; }
	case safeprintf_int64:		{ __int64 arg = va_arg(vl,__int64); REFERENCE_TO_VARIABLE(arg); return; }
	case safeprintf_uint32:		{ unsigned int arg = va_arg(vl,unsigned int); REFERENCE_TO_VARIABLE(arg); return; }
	case safeprintf_uint64:		{ unsigned __int64 arg = va_arg(vl,unsigned __int64); REFERENCE_TO_VARIABLE(arg); return; }
	case safeprintf_float:		{ double arg = va_arg(vl,double); REFERENCE_TO_VARIABLE(arg); return; }
	case safeprintf_ptrint:		{ int * arg = va_arg(vl,int*); REFERENCE_TO_VARIABLE(arg); return; }
	case safeprintf_ptrvoid:	{ void * arg = va_arg(vl,void*); REFERENCE_TO_VARIABLE(arg); return; }
	default:
		// BAD
		safeprintf_throwsyntaxerror("SkipVAArg","unknown arg type");
		return;
	}
}

static inline String GetVAArgToString(ESafePrintfType argtype, va_list & vl)
{
	switch(argtype)
	{
	case safeprintf_charptr:	{ char * arg = va_arg(vl,char *); return ToString(arg); }
	case safeprintf_wcharptr:	{ wchar_t * arg = va_arg(vl,wchar_t *); return ToString(arg); }
	case safeprintf_int32:		{ int arg = va_arg(vl,int); return ToString(arg); }
	case safeprintf_int64:		{ __int64 arg = va_arg(vl,__int64); return ToString(arg); }
	case safeprintf_uint32:		{ unsigned int arg = va_arg(vl,unsigned int); return ToString(arg); }
	case safeprintf_uint64:		{ unsigned __int64 arg = va_arg(vl,unsigned __int64); return ToString(arg); }
	case safeprintf_float:		{ double arg = va_arg(vl,double); return ToString(arg); }
	//case safeprintf_ptrint:		{ int * arg = va_arg(vl,int*); return ToString(arg); }
	case safeprintf_ptrint:		{ int * arg = va_arg(vl,int*); REFERENCE_TO_VARIABLE(arg);  /* *arg = 0; */ return String(); } // no output
	case safeprintf_ptrvoid:	{ void * arg = va_arg(vl,void*); return ToString(arg); }
	default:
		// BAD
		safeprintf_throwsyntaxerror("GetVAArgToString","unknown arg type");
		return String::GetEmpty();
	}
}

// autoToStringFunc is the main function :
//
String autoToStringFunc(int nArgs, ... )
{
	//PROFILE(autoToStringFunc);
	
	TRY
	{
		ESafePrintfType argtypes[APF_MAX_ARGS] = { safeprintf_none };

		//lprintf("autoToStringFunc:\n");
		
		// first pop all the argtypes off the varargs :
		va_list vl;
		va_start(vl,nArgs);
		for (int i=0;i<nArgs;i++)
		{
			argtypes[i] = va_arg(vl,ESafePrintfType);
	//		void * arg = va_arg(vl,void *);
			//lprintf ("\t %d : %s : %08X\n",i,c_safeprintftypenames[argtype],(uint32)(intptr_t)arg);
			//lprintf ("\t %d : %s \n",i,c_safeprintftypenames[argtypes[i]]);
		}
		
		// build up ret :
		String ret;
		//ret.Reserve(128);
		
		// step through our varargs :
		int n = 0;
		for(;;)
		{
			if ( n >= nArgs )
				break;
			
			// whatever arg we get now will be treated as a string :
			String str = GetVAArgToString(argtypes[n],vl);
			n++;
			
			const char * oldFmt = str.CStr();
			
			// find number of args, check arg types, and do autotyping :
			int numPercents = 0;
			String useFmt = DoAutoType(oldFmt,numPercents,argtypes+n);
			const char * newFmt = useFmt.CStr();
			// check argtypes[n] vs format string fmt
			
			//lprintf ("\t oldFmt : \"%s\" \n",oldFmt);
			//lprintf ("\t newFmt : \"%s\" \n",newFmt);
			
			// check that we have enough args left in varargs :
			if ( n+numPercents > nArgs )
			{
				safeprintf_throwsyntaxerror(oldFmt,"too many precents for args");
			} 
			
			va_list vlsave = vl;
			
			// now do regular old vsnprintf using the varargs :
			ret += StringRawPrintfVA(newFmt,vl);
			
			vl = vlsave;
			
			// step past the args we think we used in the varargs list :
			for(int i=0;i<numPercents;i++)
			{
				SkipVAArg(argtypes[n],vl);
				n++;
			}
		}
		va_end(vl);
		
		return ret;
	}
	CATCH
	{
		return String::GetEmpty();
	}
}

//===============================================================
// autogen the INL :

#if 1

static inline String argstr(int i)
{
	return String("arg") + ToString(i);
}

static inline String argset(int lo,int hi)
{
	String s;
	for(int i=lo;i<=hi;i++)
	{
		s += argstr(i);
		if ( i < hi )
			s += ", ";
	}
	return s;
}

static inline String Targstr(int i)
{
	return String("T") + ToString(i) + String(" arg") + ToString(i);
}

static inline String Targset(int lo,int hi)
{
	String s;
	for(int i=lo;i<=hi;i++)
	{
		s += Targstr(i);
		if ( i < hi )
			s += ", ";
	}
	return s;
}

void MakeAutoPrintfINL()
{
	FILE * toF = fopen("r:\\autoprintf.inl","wb");

	#define sfp safefprintf

	sfp(toF,"\n");
	sfp(toF,"//autoprintf.inl autogen on %s %s to %d max args\n",__DATE__,__TIME__,APF_MAX_ARGS);
	sfp(toF,"\n");
	
	/*
	
	template < typename T1, typename T2, typename T3, typename T4 >
	inline String autoToStringSub( T1 arg1, T2 arg2, T3 arg3, T4 arg4 )
	{
		return autoToStringFunc( 4,
					safeprintf_type(arg1), safeprintf_type(arg2), safeprintf_type(arg3), safeprintf_type(arg4),
					arg1, arg2, arg3, arg4 ,
					0 );
	}
	
	*/
	
	for(int nArgs=1;nArgs<APF_MAX_ARGS;nArgs++)
	{
		String s("template < ");
		for(int i=1;i<=nArgs;i++)
		{
			s += "typename T";
			s += ToString(i);
			if ( i < nArgs )
				s += ",";
			s += " ";
		}
		s += ">\n";
		s += "inline String autoToStringSub( ";
		s += Targset(1,nArgs);
		s += ")\n";
		
		s += "{\n\t";
		s += "return autoToStringFunc( ";
		s += ToString(nArgs);
		s += ",\n";
		s += "\t\t\t";
		
		for(int i=1;i<=nArgs;i++)
		{
			s += "safeprintf_type(";
			s += argstr(i);
			s += ")";
			s += ", ";
		}
		s += "\n";
		s += "\t\t\t";
		s += argset(1,nArgs);
		s += ", 0 );\n";
		s += "}\n\n";
		
		sfp(toF,s.CStr());
	}

	/**

	template < typename T1, typename T2, typename T3 >
	inline String autoToString( T1 arg1, T2 arg2, T3 arg3 )
	{
		return autoToStringSub( autoprintf_StringToChar( autoArgConvert(arg1) ), autoprintf_StringToChar( autoArgConvert(arg2) ), autoprintf_StringToChar( autoArgConvert(arg3) ) );
	}

	**/

	for(int nArgs=1;nArgs<APF_MAX_ARGS;nArgs++)
	{
		String s("template < ");
		for(int i=1;i<=nArgs;i++)
		{
			s += "typename T";
			s += ToString(i);
			if ( i < nArgs )
				s += ",";
			s += " ";
		}
		s += ">\n";
		s += "inline String autoToString( ";
		s += Targset(1,nArgs);
		s += ")\n";
		
		s += "{\n\t";
		s += "return autoToStringSub( ";
		
		for(int i=1;i<=nArgs;i++)
		{
			s += "autoprintf_StringToChar( autoArgConvert(";
			s += argstr(i);
			s += ") )";
			if ( i < nArgs )
				s += ", ";
		}
		s += ");\n";
		
		s += "}\n\n";
			
		sfp(toF,s.CStr());
	}

	/*
	template <typename T2, typename T3>
	inline void APF_CALLER ( APF_CALLER_PREARG const char * fmt , T2 arg2, T3 arg3)
	{
		String s = autoToString(fmt,arg2,arg3);
		APF_CALL_SUB( s );
	}
	*/

	for(int nArgs=1;nArgs<APF_MAX_ARGS;nArgs++)
	{
		String s("template < ");
		for(int i=1;i<=nArgs;i++)
		{
			s += "typename T";
			s += ToString(i);
			if ( i < nArgs )
				s += ",";
			s += " ";
		}
		s += ">\n";
		s += "inline void APF_CALLER ( APF_CALLER_PREARG const char * fmt , ";
		s += Targset(1,nArgs);
		s += ")\n";
		s += "{\n";
		s += "\tString s = autoToString(fmt,";
		s += argset(1,nArgs);
		s += ");\n";
		s += "\tAPF_CALL_SUB( s );\n";
		s += "}\n\n";
		
		sfp(toF,s.CStr());
	}
	
	fclose(toF);
}

#endif

END_CB
