#include "LogUtil.h"

START_CB

// log the command line :
void lprintfCommandLine(int argc,const char ** argv)
{
	for(int i=0;i<argc;i++)
	{
		lprintf("%s ",argv[i]);
	}
	lprintf("\n");	
}


// lprintfBinary prints bits with the MSB first
void lprintfBinary(uint64 val,int bits)
{
    if ( bits == 0 )
        return;
    for(--bits;bits>=0;bits--)
    {
        uint64 b = (val>>bits)&1;

        if ( b ) lprintf("1");
        else lprintf("0");
    }
}

void lprintfCByteArray(const uint8 * data,int size,const char * name,int cols)
{
    lprintf("\nstatic const int %s_size = %d;\n",name,size);
    lprintf("static const uint8 %s[] = \n",name);
    lprintf("{\n");
    for(int i=0;i<size;i++)
    {
        if ( (i%cols) == 0 )
            lprintf("  ");
        lprintf("0x%02X",data[i]);
        if ( i < size-1 )
            lprintf(",");
        if ( (i%cols) == (cols-1) )
            lprintf("\n");
    }
    lprintf("};\n\n");
}

void lprintfCIntArray(const int * data,int size,const char * name,int columns,int width)
{
    lprintf("\nstatic const int %s_size = %d;\n",name,size);
    lprintf("static const int %s[] = \n",name);
    lprintf("{\n");
    for(int i=0;i<size;i++)
    {
        if ( (i%columns) == 0 )
			lprintf("  ");
        rawlprintf("%*d",width,data[i]);
        if ( i < size-1 )
            lprintf(",");
        if ( (i%columns) == (columns-1) )
            lprintf("\n");
    }
    if ( (size%columns) != 0 ) lprintf("\n");
    lprintf("};\n");
}

void lprintfCHexArray(const uint32 * data,int size,const char * name,int columns,int width)
{
    lprintf("\nstatic const int %s_size = %d;\n",name,size);
    lprintf("static const uint32 %s[] = \n",name);
    lprintf("{\n");
    for(int i=0;i<size;i++)
    {
        if ( (i%columns) == 0 )
			lprintf("  ");
        rawlprintf("0x%0*X",width,data[i]);
        if ( i < size-1 )
            lprintf(",");
        if ( (i%columns) == (columns-1) )
            lprintf("\n");
    }
    if ( (size%columns) != 0 ) lprintf("\n");
    lprintf("};\n");
}

void lprintfCFloatArray(const float * data,int size,const char * name,int columns,int width,int decimals)
{
    lprintf("\nstatic const int %s_size = %d;\n",name,size);
    lprintf("static const float %s[] = \n",name);
    lprintf("{\n");
    for(int i=0;i<size;i++)
    {
        if ( (i%columns) == 0 )
			lprintf("  ");
        rawlprintf("%*.*ff",width,decimals,data[i]);
        if ( i < size-1 )
            lprintf(",");
        if ( (i%columns) == (columns-1) )
            lprintf("\n");
    }
    if ( (size%columns) != 0 ) lprintf("\n");
    lprintf("};\n");
}

void lprintfCDoubleArray(const double * data,int size,const char * name,int columns,int width,int decimals)
{
    lprintf("\nstatic const int %s_size = %d;\n",name,size);
    lprintf("static const double %s[] = \n",name);
    lprintf("{\n  ");
    for(int i=0;i<size;i++)
    {
        if ( (i%columns) == 0 )
			lprintf("  ");
        rawlprintf("%*.*f",width,decimals,data[i]);
        if ( i < size-1 )
            lprintf(",");
        if ( (i%columns) == (columns-1) )
            lprintf("\n  ");
    }
    if ( (size%columns) != 0 ) lprintf("\n");
    lprintf("};\n");
}

/*
 * you have to drop a \n on the string yourself
 *
 */
void lprintfCompression(int64 UnPackedLen,int64 PackedLen)
{
/*
	uint32 BPB,BPBdec;
	uint32 Ratio,Ratiodec;
	int64 PackedLenT,UnPackedLenT;

	PackedLenT = PackedLen;
	UnPackedLenT = UnPackedLen;

	while ( PackedLenT >= 536870 || UnPackedLenT >= 4294966 )
	{
		PackedLenT >>= 1;
		UnPackedLenT >>= 1;
	}
	if ( !PackedLenT) PackedLenT=1;
	if ( !UnPackedLenT) UnPackedLenT=1;


	BPB = (uint32)(PackedLenT * 8000 / UnPackedLenT); 
	BPBdec = BPB - (BPB/1000)*1000;
	BPB /= 1000;

	Ratio = (uint32)(UnPackedLenT * 1000 / PackedLenT); 
	Ratiodec = Ratio - (Ratio/1000)*1000;
	Ratio /= 1000;
*/

	ASSERT( UnPackedLen >= 0 && PackedLen >= 0 );
	if ( UnPackedLen <= 0 || PackedLen <= 0 )
	{
		lprintf("%10I64d -> 10I64d (abnormal)", UnPackedLen,PackedLen);
		return;
	}

	uint32 BPB,BPBdec;
	uint32 Ratio,Ratiodec;
	
	float BPBF = (float) ( PackedLen * 8.0 / UnPackedLen );

	BPB = (uint32) (0.5 + BPBF * 1000 );
	BPBdec = BPB - (BPB/1000)*1000;
	BPB /= 1000;

	Ratio = (uint32) (0.5 + (float)(UnPackedLen * 1000.0 / PackedLen)); 
	Ratiodec = Ratio - (Ratio/1000)*1000;
	Ratio /= 1000;
	
	//rrprintf(" %7lu -> %7lu = %2u.%03u bpb = %2u.%03u to 1 ",
	//	UnPackedLen,PackedLen,BPB,BPBdec,Ratio,Ratiodec);

	char l1[60];
	char l2[60];
	sprintfcommas(l1,UnPackedLen);
	sprintfcommas(l2,PackedLen);

	lprintf("%10s ->%10s = %2u.%03u bpb = %2u.%03u to 1 ",
		l1,l2,BPB,BPBdec,Ratio,Ratiodec);
}


void lprintfThroughput(const char * message, double seconds, uint64 ticks, int64 count )
{
    if ( count == 1 )
    {
        lprintf("%-16s: %.3f seconds, " "%I64d" " clocks\n",message,seconds,(ticks));
    }
    else
    {
		ticks = MAX(ticks,1);
		seconds = MAX(seconds,0.000000000001);
        double clocksPer = (double) (ticks) / count;
        
        double throughput = count/seconds;
        const char * throughType = "";
        // kb = 1000 or 1024 ?
        if ( throughput > 1000000 )
        {
            throughput /= 1000000;
            throughType = "MB/s";
        }
        else if ( throughput > 1000 )
        {
            throughput /= 1000;
            throughType = "kB/s";
        }
        
        const char * clocksPerType = "c/b";
        if ( clocksPer >= 1000.0 )
        {
			clocksPer /= 1000.0;
			clocksPerType = "kc/b";
        }
        
        double time = seconds*1000.0;
        const char * timeType = "millis";
        if ( time >= 1000.0 )
        {
			time = seconds;
			timeType = "seconds";
        }
        
        lprintf("%-16s: %.3f %s, %.2f %s, rate= %.2f %s\n",message,
							time,timeType,
							clocksPer,clocksPerType,
                            throughput,throughType);        
    }

}


END_CB
