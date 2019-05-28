#include "GoogleChart.h"
#include "FloatUtil.h"
#include "Log.h"

START_CB

//=====================================================================
// Google Chart encoding :

namespace GoogleChart
{

const char simpleEncoding[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const char EXTENDED_MAP[] =   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.";

const int simpleEncodingLength = ARRAY_SIZE(simpleEncoding)-1;
COMPILER_ASSERT( simpleEncodingLength == 62 );
	
const int EXTENDED_MAP_LENGTH = ARRAY_SIZE(EXTENDED_MAP) - 1;
COMPILER_ASSERT( EXTENDED_MAP_LENGTH == 64 );
	
const int extendedEncodeMax = (EXTENDED_MAP_LENGTH * EXTENDED_MAP_LENGTH - 1);
	
// simpleEncode : single char encoding
// This function scales the submitted values so that
//   maxVal becomes the highest value.
// "s:"
char simpleEncode(double value,double minValue,double maxValue) 
{
	int i = cb::froundint( (float)( (value - minValue) * simpleEncodingLength / (maxValue - minValue) ) );
	i = cb::Clamp(i,0,simpleEncodingLength-1);
	return simpleEncoding[i];
}

// extendedEncode : two char encoding
// Same as simple encoding, but for extended encoding.
// "e:"
cb::String extendedEncode(double value,double minValue,double maxValue)
{
	// Scale the value to maxVal.
	int scaledVal = cb::froundint( (float)( (value - minValue) * extendedEncodeMax / (maxValue - minValue) ) );
	scaledVal = cb::Clamp(scaledVal,0,extendedEncodeMax);

	int top = scaledVal / EXTENDED_MAP_LENGTH;
	int bottom = scaledVal - EXTENDED_MAP_LENGTH * top;

	char str[3];
	str[0] = EXTENDED_MAP[top];;
	str[1] = EXTENDED_MAP[bottom];;
	str[2] = 0;
	
	return cb::String(str);
}

cb::String textEncode(double value,double minValue,double maxValue)
{
	// Scale the value to maxVal.
	int scaledVal = cb::froundint( (float)( (value - minValue) * 100.f / (maxValue - minValue) ) );
	scaledVal = cb::Clamp(scaledVal,0,99);

	char str[3];
	str[0] = (char)(scaledVal/10) + '0';
	str[1] = (char)(scaledVal%10) + '0';
	str[2] = 0;
	
	return cb::String(str);
}

static cb::String StringFV_E(const vector<double> & v,double lo,double hi)
{
	cb::String ret;
	int s = v.size32();
	for(int i=0;i<s;i++)
	{
		double f = v[i];
		ret += GoogleChart::extendedEncode(f,lo,hi);
	}
	return ret;
}

static cb::String StringFV_T(const vector<double> & v,double lo,double hi)
{
	cb::String ret;
	int s = v.size32();
	for(int i=0;i<s;i++)
	{
		if ( i > 0 )
			ret += ",";
		double f = v[i];
		ret += GoogleChart::textEncode(f,lo,hi);
	}
	return ret;
}

static const char * s_colors[] =
{
	"008a00",
	"3072F3",
	"ff0000",
	"00aaaa",
	"aaaa00",
	"aa00aa",
	"f6D900",
	"800000",
	"f080f0"
};
static const int s_colors_count = ARRAY_SIZE(s_colors);

cb::String GetChCo(int count, const char * const * colors, int colors_count, char seperator = ',')
{
	String ret;
	for LOOP(i,count)
	{
		ret += colors[ i % colors_count ];
		if ( i != count-1 )
			ret += seperator;
	}
	return ret;
}
		
cb::String GetChm(int count, const char ** colors, int colors_count)
{
	//const char * chm  = "s,008a00,0,-1,4|s,3072F3,1,-1,4|s,ff0000,2,-1,4|s,00aaaa,3,-1,4|s,aaaa00,4,-1,4|s,aa00aa,5,-1,4|s,808080,6,-1,4|s,000080,7,-1,4";
	String ret;
	for LOOP(i,count)
	{
		ret += "s,";
		ret += colors[ i % colors_count ];
		ret += StringPrintf(",%d,-1,4",i);
		if ( i != count-1 )
			ret += "|";
	}
	return ret;
}

static const char * s_chls[] = { "1,8,0","1,4,1","1,2,3","1,4,2","1,3,3","1,2,3","1,4,1","1,2,3" };
static const int s_chls_count = ARRAY_SIZE(s_chls);
cb::String GetChls(int count)
{
	String ret;
	for LOOP(i,count)
	{
		ret += s_chls[ i % s_chls_count ];
		if ( i != count-1 )
			ret += "|";
	}
	return ret;
}
	
cb::String MakeXYPlot(const vector<PlotSeries> & series,
	const char * chartTitle,
	const char * xLabel,
	double xMin,double xMax,
	int xStep,
	const char * yLabel,
	double yMin,double yMax,
	int yStep,
	bool doChls,bool doChm,
	const char ** colors,int colors_count)
{
	//-----------------------------------------
	
	if ( colors == NULL ) colors = s_colors, colors_count = s_colors_count;
	
	cb::String html;
	
	int numSeries = series.size32();
	String chco = GetChCo(numSeries,colors,colors_count);
	String chls = GetChls(numSeries);
	String chm = GetChm(numSeries,colors,colors_count);
	
	int yMinI = (int) floor(yMin);
	int yMaxI = (int) ceil(yMax);
	
	int xMinI = (int) floor(xMin);
	int xMaxI = (int) ceil(xMax);
	
	cb::String xRangeStr;
	xRangeStr.Printf("%d,%d",xMinI,xMaxI);
	
	cb::String yRangeStr;
	yRangeStr.Printf("%d,%d",yMinI,yMaxI);
	
	/*
	cb::String plotRangeStr;
	plotRangeStr += xRangeStr;
	plotRangeStr += ",";
	plotRangeStr += yRangeStr;
	*/
	
	cb::String chxr = autoToString("0,%a,%d|1,%a,%d",xRangeStr,xStep,yRangeStr,yStep);
	
	html += "http://chart.googleapis.com/chart?chtt=";
	html += chartTitle;
	// @@ chxt twice? really?
	html += "&cht=lxy&chs=800x360&chxt=x,y,x,y&chxr=";
	// @@ chxr twice too?
	html += chxr;
	html += "|";
	html += chxr;
	html += "&chxl=2:|";
	html += xLabel;
	html += "|3:|";
	html += yLabel;
	html += "&chxp=2,50|3,50&";
	/*
	html += "chds=";
	for LOOPVEC(i,series)
	{
		html += plotRangeStr;
		if ( i != series.size32()-1 )
			html += ",";
	}
	html += "&chd=t:";
	for LOOPVEC(i,series)
	{
		html += StringFV(series[i].x);
		html += ("|");
		html += StringFV(series[i].y);
		if ( i != series.size32()-1 )
			html += "|";
	}
	/**/
	html += "chd=e:";
	
	for LOOPVEC(i,series)
	{
		// @@ I think using the float xMin/Max here is wrong
		// it should use the ints too match what was given to chxr
		html += StringFV_E(series[i].x,xMin,xMax);
		html += ",";
		html += StringFV_E(series[i].y,yMin,yMax);
		if ( i != series.size32()-1 )
			html += ",";
	}
	html += "&chdl=";
	for LOOPVEC(i,series)
	{
		html += series[i].name;
		if ( i != series.size32()-1 )
			html += "|";
	}
	html += "&chdlp=r";
	html += "&chco=";
	html += chco;
	if ( doChls )
	{
		html += "&chls=";
		html += chls;
	}
	if ( doChm )
	{
		html += "&chm=";
		html += chm;
	}
	
	return html;
}


// ThinChartSeries : remove points closer together than minD :
void ThinChartSeries(GoogleChart::PlotSeries * pS,double minD,double yratio)
{
	ASSERT( pS->x.size() == pS->y.size() );
	double minDsqr = minD*minD;
	bool didAny =false;
	do
	{
	didAny =false;
	for(int i=1;i< pS->x.size()-1;i++)
	{
		double pX = pS->x[i-1];
		double pY = pS->y[i-1]*yratio;
		
		double cX = pS->x[i];
		double cY = pS->y[i]*yratio;
		
		double nX = pS->x[i+1];
		double nY = pS->y[i+1]*yratio;
	
		double pD = fsquare(cX - pX) + fsquare(cY - pY);	
		double nD = fsquare(cX - nX) + fsquare(cY - nY);	
	
		if ( cY == pY && cY == nY ) 
		{
			// okay to remove
		}
		else
		{
			// don't remove a true local max,
			// but also don't remove the point where we change from sloping to flat
			//  eg. where I am equal to an extremum, but not all 3 points are the same
		
			// don't remove an extremum :
			if ( cY >= MAX(pY,nY) ) continue;
			if ( cY <= MIN(pY,nY) ) continue;
		}
		
		// remove if prev dist and next dist are both smaller :
		if ( pD > minDsqr || nD > minDsqr )
			continue;
		
		/*
		double mX = (pX + nX)*0.5;
		double mY = (pY + nY)*0.5;
			
		double mD = fsquare(cX - mX) + fsquare(cY - mY);
		if ( mD > minDsqr * 0.1 )
			continue;
		/**/
		
		// cut me
		pS->x.erase(pS->x.begin()+i);
		pS->y.erase(pS->y.begin()+i);
		//i--; // no, don't stay in place
		// you get more even thinning if you step ahead
		// the i++ in the "for" will take me past "next"
		didAny = true;
	}
	
	} while(didAny);
}

// ThinChartSeries : remove points until <= numPoints
void LimitChartSeries(GoogleChart::PlotSeries * pS,int numPoints,double yratio)
{
	ASSERT( pS->x.size() == pS->y.size() );
	
	while( pS->x.size() > numPoints )
	{
		int bestI = -1;
		double bestDsqr = FLT_MAX;
		
		for(int i=1;i< pS->x.size()-1;i++)
		{
			double pX = pS->x[i-1];
			double pY = pS->y[i-1]*yratio;
			
			double cX = pS->x[i];
			double cY = pS->y[i]*yratio;
			
			double nX = pS->x[i+1];
			double nY = pS->y[i+1]*yratio;
		
			double pD = fsquare(cX - pX) + fsquare(cY - pY);	
			double nD = fsquare(cX - nX) + fsquare(cY - nY);	
		
			if ( cY == pY && cY == nY ) 
			{
				// okay to remove
			}
			else
			{
				// don't remove a true local max,
				// but also don't remove the point where we change from sloping to flat
				//  eg. where I am equal to an extremum, but not all 3 points are the same
			
				// don't remove an extremum :
				if ( cY >= MAX(pY,nY) ) continue;
				if ( cY <= MIN(pY,nY) ) continue;
			}
			
			double dsqr = MAX(pD,nD);
			
			if ( dsqr < bestDsqr )
			{
				bestDsqr = dsqr;
				bestI = i;
			}
		}
	
		if ( bestI == -1 )
		{
			// this can happen if all points are local extremum
			lprintf("FAILED!\n");
			return;
		}
			
		// cut me
		pS->x.erase(pS->x.begin()+bestI);
		pS->y.erase(pS->y.begin()+bestI);	
	}
}


String MakeBarChart(const vector<GoogleChart::PlotSeries> & series,double minVal,double maxVal,int axisStep,
	const vector<String> * pDataLabels,
	const char ** colors,int colors_count)
{
	if ( colors == NULL ) colors = s_colors, colors_count = s_colors_count;
	
	String ret;

	int numSeries = series.size();
	int numValues = 0;
	for LOOPVEC(i,series)
	{
		numValues = MAX(numValues, series[i].x.size() );
	}
	
/*	
	chbh=
  <bar_width_or_scale>,
  <space_between_bars>,
  <space_between_groups>
*/
  
	int groupspace = 3;
	int pixelsAvail = 900;
	pixelsAvail -= 150; // borders
	pixelsAvail -= groupspace * numValues;
	int numBars = numSeries * numValues;
	double barSize = (double) pixelsAvail / numBars;
	int barw = (int) (barSize + 0.25); // round mostly down
	if ( numBars < 10 )
		barw = Clamp(barw,5,25);
	else
		barw = Clamp(barw,5,15);
	int pixelsUsed = barw * numBars;
	pixelsAvail = 900 - 150;
	pixelsAvail -= pixelsUsed;
	groupspace = pixelsAvail/numValues;
	groupspace = Clamp(groupspace,3,10);
	
	String chco = GetChCo(numSeries,colors,colors_count);
	
	// @@ :
	//int betweenbarspace = 0;
	int betweenbarspace = 2;
	if ( (pixelsAvail/numBars) > 100 )
		betweenbarspace = 5;
	
	maxVal = ceil(maxVal);
	bool is_signed = minVal < 0;
	if ( is_signed )
	{
		minVal = - ceil(-minVal);
	}
	else
	{
		minVal = 0;
	}
	
	int imaxVal = (int)maxVal;
	int iminVal = (int)minVal;
	
	//ret += autoToString("http://chart.apis.google.com/chart?cht=bvg&chs=900x320&chbh=%d,0,%d&chxt=x,y",barw,groupspace);
	ret += autoToString("http://chart.apis.google.com/chart?cht=bvg&chs=900x320&chbh=%d,%d,%d&chxt=x,y",barw,betweenbarspace,groupspace);
	ret += autoToString("&chco=%a",chco);
	ret += autoToString("&chxr=1,0,%d,%d",imaxVal,axisStep);
	if ( is_signed )
	{
		ret += autoToString("&chds=%d,%d",iminVal,imaxVal);
		ret += autoToString("&chxr=1,%d,%d,%d",iminVal,imaxVal,axisStep);
	}
	ret += autoToString("&chxs=0,000000,12,0,lt|1,000000,10,1,lt&chdl=");
	
	for LOOPVEC(i,series)
	{
		if ( i > 0 )
			ret += String("|");
		ret += autoToString("%a", series[i].name);
	}
	
	if ( ! is_signed )
	{
		ret += String("&chd=e:");
		
		for LOOPVEC(i,series)
		{
			if ( i > 0 )
				ret += (",");
			for LOOPVEC(j,series[i].x)
			{
				double cur = series[i].x[j];
				String s = GoogleChart::extendedEncode(cur,minVal,maxVal);
				ret += s; //autoToString("%a",s);
			}
		}
	}
	else
	{
		ret += String("&chd=t:");
		
		for LOOPVEC(i,series)
		{
			if ( i > 0 )
				ret += ("|");
			for LOOPVEC(j,series[i].x)
			{
				if ( j > 0 )
					ret += (",");
				double cur = series[i].x[j];
				ret += StringPrintf("%.3f",cur);
			}
		}
	}
	
	if ( pDataLabels != NULL )
	{
		ret += String("&chxl=0:");
		for LOOPVEC(i,(*pDataLabels))
		{
			ret += "|";
			ret += pDataLabels->at(i);
		}
	}
	
	/*
	lprintf("&chxl=0:");
	for LOOPVEC(j,testFileNames)
	{
		lprintf("|%a",testFileNames[j]);
	}
	*/
			
	return ret;
}

struct FuckC
{
	double x;
	String label;
	int pi;
	
	bool operator < (const FuckC & rhs) const
	{
		return x < rhs.x;
	}
};

cb::String MakeScatterChart(const PlotSeries & series,double minX,double maxX,double minY,double maxY,int xStep,int yStep,
	const vector<const char *> & colors,
	const vector<String> & labels,
	const vector<String> & labels2)
{
	
	String html;

	html += autoToString("http://chart.apis.google.com/chart?cht=s&chs=800x360");
	
	/*
	html += "&chtt=";
	html += chartTitle;
	*/
	
	/*
	double yMin = FLT_MAX;
	double xMin = FLT_MAX;
	double yMax = 0;
	double xMax = 0;
	for LOOPVEC(i,series.x)
	{
		double x = series.x[i];
		double y = series.y[i];
		xMin = MIN(x,xMin);
		yMin = MIN(y,yMin);
		xMax = MAX(x,xMax);
		yMax = MAX(y,yMax);
	}
	
	int yMinI = (int) floor(yMin);
	int yMaxI = (int) ceil(yMax);
	
	int xMinI = (int) floor(xMin);
	int xMaxI = (int) ceil(xMax);
	*/
	
	int xMinI = (int) floor(minX);
	int yMinI = (int) floor(minY);
	int xMaxI = (int) ceil(maxX);
	int yMaxI = (int) ceil(maxY);
	
	cb::String xRangeStr;
	xRangeStr.Printf("%d,%d",xMinI,xMaxI);
	
	cb::String yRangeStr;
	yRangeStr.Printf("%d,%d",yMinI,yMaxI);
	
	cb::String plotRangeStr;
	plotRangeStr += xRangeStr;
	plotRangeStr += ",";
	plotRangeStr += yRangeStr;
	
	html += "&chxt=x,y";
	
	cb::String chxr = autoToString("0,%a,%d|1,%a,%d",xRangeStr,xStep,yRangeStr,yStep);
		
	html += "&chxr=";
	html += chxr;
	
	//*
	html += "&chd=e:";
	
	html += StringFV_E(series.x,(double)xMinI,(double)xMaxI);
	html += ",";
	html += StringFV_E(series.y,(double)yMinI,(double)yMaxI);
	/**/

	/*
	html += ",";
	vector<double> chart_sizes;
	chart_sizes.resize(series.y.size());
	for LOOPVEC(i,chart_sizes)
		chart_sizes[i] = 10.f;
	html += StringFV_E(chart_sizes,0.f,100.f);
	/**/
	
	/*

	html += "&chd=t:";
	
	html += StringFV_T(series.x,(double)xMinI,(double)xMaxI);
	html += "|";
	html += StringFV_T(series.y,(double)yMinI,(double)yMaxI);
	/**/
	
	html += "&chdl=";
	for LOOPVEC(i,labels)
	{
		html += labels[i];
		if ( i != labels.size32()-1 )
			html += "|";
	}
	html += "&chdlp=r";
	
	String chco = GetChCo(labels.size32(),colors.data(),colors.size32(),'|');
	
	html += "&chco=";
	html += chco;
	
	html += "&chm=";
	html += "s,000000,0,0,0";
	const char * chm_shapes = "odcsx";
	int chm_shapes_count = strlen32(chm_shapes);
	int ncolors = colors.size32();
	// add a halo when I need to repeat a shape :
	// pass 1 for halos :
	for LOOPVEC(i,series.x)
	{
		//int c = i % ncolors;
		int s = i / ncolors;
		if ( s >= chm_shapes_count )
		{
			html += "|o,bbbbbb,0,";
			html += autoToString("%d,",i);
			html += "15";
		}
	}
	
	vector<int> labels2_index;
	labels2_index.resize( labels2.size() , -1 );
	
	// pass 2 for shapes :
	for LOOPVEC(i,series.x)
	{
		int c = i % ncolors;
		int s = i / ncolors;
		if (labels2_index[s] == -1 || series.x[i] > series.x[ labels2_index[s] ] )
			labels2_index[s] = i;
		
		int si = s % chm_shapes_count;
		char shape = chm_shapes[si];
		html += autoToString("|%c,",shape);
		html += colors[c];
		html += ",0,";
		html += autoToString("%d,",i);
		html += "10";
	}
	
	// label the right-most occurance of each file :
	if ( labels2.size() > 1 ) // don't do it if only 1 label
	{
	
		// sort labels by x ?
		// (trying to fix the "A" placement)
		vector<FuckC> fuckers;
		
		// labels2 ?
		for LOOPVEC(i,labels2)
		{
			int pi = labels2_index[i];
			if ( pi == -1 ) continue;
		
			FuckC f;
			f.x = series.x[pi];
			f.pi = pi;
			f.label = labels2[i];
			
			fuckers.push_back(f);
		}
		
		std::sort(fuckers.begin(),fuckers.end());
		
		for LOOPVEC(i,fuckers)
		{	
			//html += "|A"; // <- crap
			html += "|t";
			html += fuckers[i].label;
			html += ",000000,0,";
			html += autoToString("%d,",fuckers[i].pi);
			html += "10,,:5:0";
		}

	}
	
	// draw lines between files :
	#if 0
	
	int nLabels = labels.size32();
	for(int i=0;i<series.x.size32();i+=nLabels)
	{
		html += autoToString("|D,808080,0,%d:%d,1,-1",i,i+nLabels-1);
	}
	
	#endif
	
	return html;
}

};

END_CB
