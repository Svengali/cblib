#pragma once

#include "String.h"
#include "vector.h"

START_CB

//=====================================================================
// Google Chart encoding :

namespace GoogleChart
{

	// simpleEncode : single char encoding
	// This function scales the submitted values so that
	//   maxVal becomes the highest value.
	// "s:"
	char simpleEncode(double value,double minValue,double maxValue) ;
	
	// extendedEncode : two char encoding
	// Same as simple encoding, but for extended encoding.
	// "e:"
	cb::String extendedEncode(double value,double minValue,double maxValue);
	
	struct PlotSeries
	{
		cb::String			name;
		cb::vector<double>	x;
		cb::vector<double>	y;
	};

	cb::String MakeXYPlot(const cb::vector<PlotSeries> & series,
		const char * chartTitle,
		const char * xLabel,
		double xMin,double xMax,
		int xStep,
		const char * yLabel,
		double yMin,double yMax,
		int yStep,
		bool doChls=true,bool doChm=true,
		const char ** colors = NULL,int colors_count = 0);
		
	
	// ThinChartSeries : remove points closer together than minD :
	//	distance is in X units; yratio should be (Xsize/Ysize)
	void ThinChartSeries(GoogleChart::PlotSeries * pS,double minD,double yratio);
	
	// ThinChartSeries : remove points until <= numPoints
	void LimitChartSeries(GoogleChart::PlotSeries * pS,int numPoints,double yratio);

	// MakeBarChart uses series.x
	String MakeBarChart(const vector<GoogleChart::PlotSeries> & series,double minVal,double maxVal,int axisStep,
						const vector<String> * pDataLabels = NULL,
						const char ** colors = NULL,int colors_count = 0);

	String MakeScatterChart(const PlotSeries & series,
		double minX,double maxX,
		double minY,double maxY,
		int xStep,int yStep,
		const vector<const char *> & colors,
		const vector<String> & labels,
		const vector<String> & labels2);
};

END_CB