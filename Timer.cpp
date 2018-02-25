#include "cblib/Timer.h"
#include "cblib/Log.h"
//#include "cblib/Util.h"
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>

//for CallNtPowerInformation
//#include <Powrprof.h>
//#pragma comment(lib,"Powrprof.lib")

START_CB

namespace Timer
{
	static int		s_mhz = 0;
	static double	s_secondsPerTick = 0.0;
	
	typedef unsigned __int64 uint64;
	
	static uint64	s_qpf = 0;
	static double	s_secondsPerQPC = 0.0;
	
	//static Sample	s_lastSample = { 0 };
	
	void ComputeMHZ();
};

//---------------------------------------------------------------

void Timer::GetSample(Sample * ptr)
{
	// for the Start sample I should the tsc *last*
	//	for the End sample I should read it first

	ptr->tsc = rdtsc();
	ptr->qpc = GetQPCSeconds();	
	//ptr->millis = GetTickCount();
	
	//s_lastSample = *ptr;
}

//
// CallNtPowerInformation provides lots of info about speedstep and mhz and such

double Timer::DeltaSamples(const Sample & s1,const Sample & s2)
{
	return s2.qpc - s1.qpc;

/*
	double dMillis = s2.millis - s1.millis;
	double dQPC   = s2.qpc - s1.qpc;
	__int64 dTSC = s2.tsc - s1.tsc;
	
	double dQPCMillis = dQPC * 1000.0;
	double dTSCSeconds = double(dTSC) * GetSecondsPerTick();
	double dTSCMillis =  dTSCSeconds * 1000.0;
	
	double offQPCMillis = fabs(dMillis - dQPCMillis);
	
	// check if the qpc is faster than 200 MHz , if it is, it's probably using the TSC,
	//	so don't treat it as separate from the TSC
	if ( s_qpf < 200000000 && offQPCMillis < 10.0 )
	{
		double offTSCQPCMillis = fabs(dQPCMillis - dTSCMillis);
	
		// check a large tolerance ; QPC is only bad by really large amounts
		//	QPC can jump about 4 seconds
		// qpc looks reliable
		if ( offTSCQPCMillis < 0.1 )
		{
			// tsc looks reliable
			return dTSCSeconds;
		}
		else
		{
			return dQPC;
		}
	}
	else 
	{
		double offTSCMillis = fabs(dMillis - dTSCMillis);
		if ( offTSCMillis < 1.5 )
		{
			// qpc looks no good
			// maybe tsc is ok ?
			//	this is hard to test well on a micro level cuz of thread switches
			return dTSCSeconds;
		}
		else
		{
			// qpc and tsc both look bad
			return double(dMillis) / 1000.0;
		}
	}
*/
}

// GetSeconds is seconds from startup using the Sample routines
double Timer::GetSeconds()
{
	Sample cur;
	GetSample(&cur);	
	
	static Sample s_first = { 0 };
	static double s_last = 0.0;

	if ( s_first.tsc == 0 )
	{
		GetSample(&cur);
		s_first = cur;
	}
	
	double seconds = DeltaSamples(s_first,cur);
	
	if ( seconds < s_last )
	{
		// don't let time ever run backwards
		return s_last;
	}
	
	s_last = seconds;
	
	return seconds;
}
		
//---------------------------------------------------------------
//	GetMillis uses the windows GetTickCount
//	it's reliable, but only measures millisecond accuracy
unsigned int Timer::GetMillis()
{
	return GetTickCount();
}

//---------------------------------------------------------------

double Timer::GetQPCSeconds()
{
	uint64 qpc;
	QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
	
	if ( s_qpf == 0 )
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&s_qpf);
		s_secondsPerQPC = 1.0 / double(s_qpf);
	}
	
	return qpc * s_secondsPerQPC;	
}

//---------------------------------------------------------------
// tsc counts the number of clocks passed
//	it's fast and always reliable when measured in clocks
//	the tsc conversion to seconds does weird things on laptops with speedstep

Timer::tsc_type	Timer::rdtsc()
{
    return __rdtsc();
}
	
int	Timer::GetMHZ()
{
	if ( s_mhz == 0 )
	{
		ComputeMHZ();
	}
	return s_mhz;
}

double Timer::GetSecondsPerTick()
{
	if ( s_mhz == 0 )
	{
		ComputeMHZ();
	}
	return s_secondsPerTick;	
}

double Timer::rdtscSeconds()
{
	tsc_type ticks = rdtsc();
	return double(ticks) * GetSecondsPerTick();
}

#define TSC_PATH	"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"
#define TSC_FILE	 "~MHz"

static bool RegistryGetValue(const char * path,const char * file,int * pValue)
{
	HKEY hKey;
	bool result;
	DWORD type;
	DWORD size;

	if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE,path,0,KEY_QUERY_VALUE,&hKey)
		 != ERROR_SUCCESS )
	{
		return false;
	}

	// Set the value for the given name as a binary value

	result = RegQueryValueExA
				(
					hKey,
					file,
					0,
					&type,
					(BYTE *) pValue,
					&size
				) == ERROR_SUCCESS;
 
	RegCloseKey (hKey);

	return result;
}

void Timer::ComputeMHZ()
{
	/*
	// non-standard, requires pulling in funny libs, etc.
	PROCESSOR_POWER_INFORMATION ppi;
	NTSTATUS nts = CallNtPowerInformation(ProcessorInformation,NULL,0,&ppi,sizeof(ppi));
	if ( nts == STATUS_SUCCESS )
	{
		int i = 1;
	}
	/**/
	
	if ( ! RegistryGetValue(TSC_PATH,TSC_FILE,&s_mhz) )
	{
		// can't get to the registry value !
		// calibrate myself
		// just do a shit calibration :

		// try a max of 4 times
		for(int tries=0;tries<4;tries++)
		{
			// first busy loop to align to a milli
			DWORD tick1 = GetTickCount();
			while ( GetTickCount() == tick1 ) ;
			
			// then measure the # of ticks for one more milli to pass :
			tsc_type tsc1 = rdtsc();
			DWORD tick2 = GetTickCount();
			while ( GetTickCount() == tick2 ) ;
			tsc_type tsc2 = rdtsc();	
			
			// 1 millisecond has passed
			tsc_type deltaTsc = tsc2 - tsc1;
			DWORD hz = ((DWORD)deltaTsc) * 1000;
			// round to the nearest million :
			s_mhz = (hz + 500000)/1000000;
			
			if ( s_mhz > 100 )
			{
				// it's good, get out
				break;
			}
		}
	}
	
	s_secondsPerTick = 1.0 / (s_mhz * 1000000.0);
}	

void Timer::LogInfo() // call at startup if you like
{
	// @@ should log SpeedStep info also
	GetSeconds();
	Log("Timer : MHZ=%d , QPC MHZ = %4f\n",GetMHZ(),(double)s_qpf/1000000.0);	
}

TimeScopeLog::~TimeScopeLog()
{
	Timer::Sample end;
	Timer::GetSample(&end);
	double delta = Timer::DeltaSamples(m_start,end);
	Log("Timer : %s : %f s\n",m_tag,delta);
}

END_CB
