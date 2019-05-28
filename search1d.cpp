#include "floatutil.h"
#include "search1d.h"

START_CB

//=====================================================

// code from wikipedia
// Brent's = combination of various root finders

double FindRoot_BrentsMethod(tfunc_doubledouble * function, double lowerLimit, double upperLimit, double errorTol)
{
    double a = lowerLimit;
    double b = upperLimit;
    double c = 0;
    double d = FLT_MAX;

    double fa = function(a);
    double fb = function(b);

    double fc = 0;
    double s = 0;
    double fs = 0;

    // if f(a) f(b) >= 0 then error-exit
    if (fa * fb >= 0)
    {
        if (fa < fb)
            return a;
        else
            return b;
    }

    // if |f(a)| < |f(b)| then swap (a,b) end if
    if (fabs(fa) < fabs(fb))
    { double tmp = a; a = b; b = tmp; tmp = fa; fa = fb; fb = tmp; }

    c = a;
    fc = fa;
    bool mflag = true;
    int i = 0;

    while (!(fb==0) && (fabs(a-b) > errorTol))
    {
        if ((fa != fc) && (fb != fc))
        {
            // Inverse quadratic interpolation
            s = a * fb * fc / (fa - fb) / (fa - fc) + b * fa * fc / (fb - fa) / (fb - fc) + c * fa * fb / (fc - fa) / (fc - fb);
        }
        else
        {
            // Secant Rule
            s = b - fb * (b - a) / (fb - fa);
		}
		
        double tmp2 = (3 * a + b) / 4;
        if ((!(((s > tmp2) && (s < b)) || ((s < tmp2) && (s > b)))) || (mflag && (fabs(s - b) >= (fabs(b - c) / 2))) || (!mflag && (fabs(s - b) >= (fabs(c - d) / 2))))
        {
            s = (a + b) / 2;
            mflag = true;
        }
        else
        {
            if ((mflag && (fabs(b - c) < errorTol)) || (!mflag && (fabs(c - d) < errorTol)))
            {
                s = (a + b) / 2;
                mflag = true;
            }
            else
                mflag = false;
        }
        fs = function(s);
        d = c;
        c = b;
        fc = fb;
        if (fa * fs < 0) { b = s; fb = fs; }
        else { a = s; fa = fs; }

        // if |f(a)| < |f(b)| then swap (a,b) end if
        if (fabs(fa) < fabs(fb))
        { double tmp = a; a = b; b = tmp; tmp = fa; fa = fb; fb = tmp; }
        i++;
        if (i > 1000)
        {
			ASSERT_RELEASE("root finder can't converge");
			return FLT_MAX;
        }
    }
    return b;
}

struct square_funcptr
{
	tfunc_doubledouble * m_funcptr;
	double operator() ( double x)
	{
		double y = (*m_funcptr)(x);
		return y*y;
	}
};

double FindRoot_GoldenMinSqr(tfunc_doubledouble * function, double lo, double hi, int fixedSteps, double tolerance)
{
	double step = (hi - lo)/fixedSteps;

	square_funcptr f;
	f.m_funcptr = function;	
	double x = Search1d_FixedStepAndDownEach(f,lo,step,fixedSteps,tolerance);

	return x;	
}

END_CB
