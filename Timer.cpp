#include "Timer.h"
#include "Log.h"
#include "FloatUtil.h"
//#include "Util.h"
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include "stl_basics.h"
#include <algorithm>

//for CallNtPowerInformation
//#include <Powrprof.h>
//#pragma comment(lib,"Powrprof.lib")

START_CB

namespace Timer
{
	static int		s_mhz = 0;
	static double	s_secondsPerTick = 0.0;
	
	//typedef unsigned __int64 uint64;
	
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

	#define READ_CLOCK_THRESHOLD	(10000)

	// protect against the QPC call taking a really long time due to DPC :

again:
	tsc_type tsc1 = rdtsc();
	ptr->qpc = RawQPC();	
	ptr->tickCount = GetMillis32();
	tsc_type tsc_delta = rdtsc() - tsc1;
	if ( tsc_delta > READ_CLOCK_THRESHOLD ) goto again;
	
	#ifdef DO_SAMPLE_TSC
	// be careful of wraps
	ptr->tsc = tsc1;
	ptr->tsc += tsc_delta/2;
	#endif
}

//
// CallNtPowerInformation provides lots of info about speedstep and mhz and such

double Timer::DeltaSamples(const Sample & s1,const Sample & s2)
{
	#ifdef DO_SAMPLE_TSC

	tick_count_type dTickMillis = s2.tickCount - s1.tickCount;
	uint64 dQPC   = s2.qpc - s1.qpc;
	tsc_type dTSC   = s2.tsc - s1.tsc;
	
	double qpcRes = GetSecondsPerQPC();
	double dQPCSeconds = dQPC * qpcRes;
	double dTSCSeconds = double(dTSC) * GetSecondsPerTick();
	double dTickSeconds = dTickMillis * 0.001;
		
	double offQPCMillis = fabs( dQPCSeconds - dTickSeconds ) * 1000.0;
	double offTSCMillis = fabs( dTSCSeconds - dTickSeconds ) * 1000.0;
	
	// if everything is right, then offQPCMillis and offTSCMillis should be <= 1.0	

	// check if the qpc is faster than 200 MHz , if it is, it's probably using the TSC,
	//	so don't treat it as separate from the TSC
	if ( s_qpf > 200000000 )
	{
		// QPC is probably TSC
		
		// just check TSC
		if ( offTSCMillis <= 1.0 )
		{
			return dTSCSeconds;
		}
		else
		{	
			return dTickSeconds;
		}
	}
	
	// check TSC vs QPC :
	
	// first check if QPC is okay :
	// check a large tolerance ; QPC is only bad by really large amounts
	//	QPC can jump about 4 seconds
	if ( offQPCMillis > 10.0 )
	{
		// QPC is no good
		return dTickSeconds;
	}
		
	double dTSCQPC = fabs( dTSCSeconds - dQPCSeconds );
	
	// TSC should be within a few QPC res steps of QPC :
	
	if ( dTSCQPC <= 10.0 * qpcRes )
	{
		return dTSCSeconds;
	}
	else
	{
		return dQPCSeconds;
	}

	#else // DO_SAMPLE_TSC
	
	// no TSC : 
	
	tick_count_type dTickMillis = s2.tickCount - s1.tickCount;
	uint64 dQPC   = s2.qpc - s1.qpc;
	
	double qpcRes = GetSecondsPerQPC();
	double dQPCSeconds = dQPC * qpcRes;
	double dTickSeconds = dTickMillis * 0.001;
		
	double offQPCMillis = fabs( dQPCSeconds - dTickSeconds ) * 1000.0;
	
	// if everything is right, then offQPCMillis and offTSCMillis should be <= 1.0	

	// first check if QPC is okay :
	// check a large tolerance ; QPC is only bad by really large amounts
	//	QPC can jump about 4 seconds
	if ( offQPCMillis > 10.0 )
	{
		// QPC is no good
		return dTickSeconds;
	}
	else
	{
		return dQPCSeconds;
	}
	
	#endif // DO_SAMPLE_TSC

}

// GetSeconds is seconds from startup using the Sample routines

// GetSeconds crucially uses the *last* sample to generate a delta
//	not the first sample at startup
//	this lets us keep tracking forward when the QPC jumps back or something

// @@ not thread safe :
static bool s_GetSeconds_init = false;
static Timer::Sample s_GetSeconds_lastSample = { 0 };
static double s_GetSeconds_lastSeconds = 0.0;
	
double Timer::GetSeconds()
{
	if ( ! s_GetSeconds_init )
	{
		s_GetSeconds_init = true;
		GetSample(&s_GetSeconds_lastSample);
		s_GetSeconds_lastSeconds = 0.0;
	}
	
	Sample cur;
	GetSample(&cur);	
	
	double deltaSeconds = DeltaSamples(s_GetSeconds_lastSample,cur);
	
	// 365 days would be a really big clock delta :
	#define ONE_YEAR_SECONDS	(365*24*3600)
	
	s_GetSeconds_lastSample = cur;
	
	if ( deltaSeconds < 0.0 || deltaSeconds > ONE_YEAR_SECONDS )
	{
		// don't let time ever run backwards or jump way forward
		return s_GetSeconds_lastSeconds;
	}
	
	s_GetSeconds_lastSeconds += deltaSeconds;
	
	return s_GetSeconds_lastSeconds;
}
		
//---------------------------------------------------------------
//	GetMillis uses the windows GetTickCount
//	it's reliable, but only measures millisecond accuracy
uint32 Timer::GetMillis32()
{
	//return GetTickCount();
	return timeGetTime();
}

// Vista has a GetTickCount64

// @@ not thread safe
static uint32 s_GetTickCount64_lastTicks = 0;
static uint32 s_GetTickCount64_carry = 0;
	
uint64 Timer::GetMillis64()
{
	uint32 ticks = timeGetTime();
	
	// check for going backwards :
	if ( ticks < s_GetTickCount64_lastTicks )
	{
		// carry :
		s_GetTickCount64_carry ++;
	}
	
	s_GetTickCount64_lastTicks = ticks;
	
	return (((uint64)s_GetTickCount64_carry)<<32) + ticks;
}

//---------------------------------------------------------------

uint64 Timer::RawQPC()
{
	uint64 qpc;
	QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
	return qpc;
}

uint64 Timer::QPC()
{
	static uint64 s_first = RawQPC();
	uint64 cur = RawQPC();
	return (cur - s_first);
}

double Timer::GetSecondsPerQPC()
{
	if ( s_qpf == 0 )
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&s_qpf);
		s_secondsPerQPC = 1.0 / double(s_qpf);
	}
	
	return s_secondsPerQPC;
}

double Timer::GetQPCSeconds()
{
	uint64 qpc = QPC();
	
	return qpc * GetSecondsPerQPC();	
}

//---------------------------------------------------------------
// tsc counts the number of clocks passed
//	it's fast and always reliable when measured in clocks
//	the tsc conversion to seconds does weird things on laptops with speedstep

Timer::tsc_type	Timer::rdtsc()
{
	#ifdef CB_64
	return __rdtsc();
	#else
	__asm rdtsc;
	// eax/edx returned
	#endif
}
	
int	Timer::GetMHZ()
{
	if ( s_mhz == 0 )
	{
		ComputeMHZ();
	}
	return s_mhz;
}

uint64	Timer::GetAbsoluteTicks()
{
	static uint64 s_first = rdtsc();
	uint64 cur = rdtsc();
	return (cur - s_first);
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

	if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,path,0,KEY_QUERY_VALUE,&hKey)
		 != ERROR_SUCCESS )
	{
		return false;
	}

	// Set the value for the given name as a binary value

	result = RegQueryValueEx
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
	//printf("@@ComputeMHZ{\n");

	/*
	// non-standard, requires pulling in funny libs, etc.
	PROCESSOR_POWER_INFORMATION ppi;
	NTSTATUS nts = CallNtPowerInformation(ProcessorInformation,NULL,0,&ppi,sizeof(ppi));
	if ( nts == STATUS_SUCCESS )
	{
		int i = 1;
	}
	/**/
	
	// call Threading_Init cuz it does some speedstep stuff and timeBeginPeriod
	//Threading_Init();
	
	timeBeginPeriod(1);
	
	GetSecondsPerQPC();
	
	if ( ! RegistryGetValue(TSC_PATH,TSC_FILE,&s_mhz) )
	{
		// can't get to the registry value !
		// calibrate myself
		// just do a shit calibration :

		// bump up PRiority temporarily :		
		int oldpri = GetThreadPriority( GetCurrentThread() );
		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );

		// spin a second to get the speedstep out :
		volatile int vi = 0;
		
		for(int tries=0;tries<5;tries++)
		{
			DWORD tick1 = GetMillis32();			
			while ( GetMillis32() == tick1 )
			{
				vi ++;
			}
		}
		
		int mhz[5];
		
		// try a max of 4 times
		int retries = 0;
		for(int tries=0;tries<5;tries++)
		{
			// first busy loop to align to a milli
			DWORD tick1 = GetMillis32();
			while ( GetMillis32() == tick1 ) vi++;
			
			double sec1 = GetQPCSeconds();
			tsc_type tsc1 = rdtsc();
			
			// go a few :
			for(int m=0;m<5;m++)
			{
				DWORD tick2 = GetMillis32();
				while ( GetMillis32() == tick2 ) vi++;
			}
			
			double sec2 = GetQPCSeconds();
			tsc_type tsc2 = rdtsc();	
			
			// 1 millisecond has passed
			tsc_type deltaTsc = tsc2 - tsc1;
			double hz = deltaTsc / (sec2 - sec1);
			// round to the nearest million :
			mhz[tries] = froundint( (float) (hz/1000000.0) );
		
			//printf(" %f : %d \n",(sec2-sec1),mhz[tries]);
		
			if ( mhz[tries] < 200 )
			{
				tries--; // don't count it, redo
				retries++;
				if ( retries >= 10 )
				{
					lprintf("FUCK FUCK ComputeMHZ retries!\n");
				}
				continue;
			}
		}
		
		// take the median :		
		std::sort(mhz,mhz+5);
		s_mhz = mhz[2];
		
		SetThreadPriority( GetCurrentThread(), oldpri );
	}
	
	// @@ ?? check s_qpf vs. s_mhz
	//	- is QPC using the TSC ?
	//	if so, then QPF is a better measure
	
	s_secondsPerTick = 1.0 / (s_mhz * 1000000.0);
	
	//printf("@@ComputeMHZ}\n");
}	

void Timer::LogInfo() // call at startup if you like
{
	// @@ should log SpeedStep info also
	GetSeconds();
	GetMHZ();
	lprintf("Timer : TSC MHZ=%d , QPC MHZ = %.3f\n",s_mhz,(double)s_qpf/1000000.0);	
}

TimeScopeLog::~TimeScopeLog()
{
	Timer::Sample end;
	Timer::GetSample(&end);
	double delta = Timer::DeltaSamples(m_start,end);
	if ( m_count <= 1 )
	{
		lprintf("Timer : %s : %f s\n",m_tag,delta);
	}
	else
	{
		delta /= m_count;
		lprintf("Timer : %s : %.2f s per M, %.2f M per sec\n",m_tag,delta*1000000,1/(1000000*delta));
	}
}

TSCScopeLog::~TSCScopeLog()
{
	Timer::tsc_type end = Timer::rdtsc();
	double delta = (double)(end - m_start);
	if ( m_count <= 1 )
	{
		lprintf("Timer : %s : %f clocks\n",m_tag,delta);
	}
	else
	{
		delta /= m_count;
		lprintf("Timer : %s : %f clocks per (%d)\n",m_tag,delta,m_count);
	}
}

END_CB
