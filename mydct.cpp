#include "mydct.h"
#include "cblib/Base.h"
#include <stdlib.h>

START_CB

//=============================================================

//namespace MyDCT
//{

#define DCTSIZE 8

typedef float dct_float;
	
static const dct_float aanscalefactor[DCTSIZE] = 
{
	(dct_float)1.0, (dct_float)1.387039845, (dct_float)1.306562965, (dct_float)1.175875602,	(dct_float)1.0, (dct_float)0.785694958, (dct_float)0.541196100, (dct_float)0.275899379
};

const float c_idct_aanfactors[64] = 
{
  0.12500f,0.17338f,0.16332f,0.14698f,0.12500f,0.09821f,0.06765f,0.03449f,
  0.17338f,0.24048f,0.22653f,0.20387f,0.17338f,0.13622f,0.09383f,0.04784f,
  0.16332f,0.22653f,0.21339f,0.19204f,0.16332f,0.12832f,0.08839f,0.04506f,
  0.14698f,0.20387f,0.19204f,0.17284f,0.14698f,0.11548f,0.07955f,0.04055f,
  0.12500f,0.17338f,0.16332f,0.14698f,0.12500f,0.09821f,0.06765f,0.03449f,
  0.09821f,0.13622f,0.12832f,0.11548f,0.09821f,0.07716f,0.05315f,0.02710f,
  0.06765f,0.09383f,0.08839f,0.07955f,0.06765f,0.05315f,0.03661f,0.01866f,
  0.03449f,0.04784f,0.04506f,0.04055f,0.03449f,0.02710f,0.01866f,0.00952f
};

const float c_fdct_aanfactors[64] = 
{
  0.12500f,0.09012f,0.09567f,0.10630f,0.12500f,0.15909f,0.23097f,0.45306f,
  0.09012f,0.06497f,0.06897f,0.07664f,0.09012f,0.11470f,0.16652f,0.32664f,
  0.09567f,0.06897f,0.07322f,0.08136f,0.09567f,0.12177f,0.17678f,0.34676f,
  0.10630f,0.07664f,0.08136f,0.09040f,0.10630f,0.13530f,0.19642f,0.38530f,
  0.12500f,0.09012f,0.09567f,0.10630f,0.12500f,0.15909f,0.23097f,0.45306f,
  0.15909f,0.11470f,0.12177f,0.13530f,0.15909f,0.20249f,0.29397f,0.57664f,
  0.23097f,0.16652f,0.17678f,0.19642f,0.23097f,0.29397f,0.42678f,0.83715f,
  0.45306f,0.32664f,0.34676f,0.38530f,0.45306f,0.57664f,0.83715f,1.64213f
};

const float c_my_dct_csf_luma[64] = 
{
	1.000000f, 1.454545f, 1.600000f, 1.000000f, 0.666667f, 0.400000f, 0.313726f, 0.262295f, 
	1.333333f, 1.333333f, 1.142857f, 0.842105f, 0.615385f, 0.275862f, 0.266667f, 0.290909f, 
	1.142857f, 1.230769f, 1.000000f, 0.666667f, 0.400000f, 0.280702f, 0.231884f, 0.285714f, 
	1.142857f, 0.941176f, 0.727273f, 0.551724f, 0.313726f, 0.183908f, 0.200000f, 0.258065f, 
	0.888889f, 0.727273f, 0.432432f, 0.285714f, 0.235294f, 0.146789f, 0.155340f, 0.207792f, 
	0.666667f, 0.457143f, 0.290909f, 0.250000f, 0.197531f, 0.153846f, 0.141593f, 0.173913f, 
	0.326531f, 0.250000f, 0.205128f, 0.183908f, 0.155340f, 0.132231f, 0.133333f, 0.158416f, 
	0.222222f, 0.173913f, 0.168421f, 0.163265f, 0.142857f, 0.160000f, 0.155340f, 0.161616f, 
};

const float c_my_dct_csf_chroma[64] = 
{ 
	1.000000f, 0.944444f, 0.708333f, 0.361702f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 
	0.944444f, 0.809524f, 0.653846f, 0.257576f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 
	0.708333f, 0.653846f, 0.303571f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 
	0.361702f, 0.257576f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 
	0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 
	0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 
	0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 
	0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 0.171717f, 
};

const float c_fdct_aanfactors_times_csf_luma[64] = 
{
   0.125000f, 0.131084f, 0.153072f, 0.106300f, 0.083333f, 0.063636f, 0.072461f, 0.118835f,
   0.120160f, 0.086627f, 0.078823f, 0.064539f, 0.055458f, 0.031641f, 0.044405f, 0.095023f,
   0.109337f, 0.084886f, 0.073220f, 0.054240f, 0.038268f, 0.034181f, 0.040992f, 0.099074f,
   0.121486f, 0.072132f, 0.059171f, 0.049876f, 0.033349f, 0.024883f, 0.039284f, 0.099432f,
   0.111111f, 0.065542f, 0.041371f, 0.030371f, 0.029412f, 0.023353f, 0.035879f, 0.094142f,
   0.106060f, 0.052434f, 0.035424f, 0.033825f, 0.031425f, 0.031152f, 0.041624f, 0.100285f,
   0.075419f, 0.041630f, 0.036263f, 0.036123f, 0.035879f, 0.038872f, 0.056904f, 0.132618f,
   0.100680f, 0.056807f, 0.058402f, 0.062906f, 0.064723f, 0.092262f, 0.130043f, 0.265394f
};

const float c_fdct_aanfactors_times_csf_chroma[64] = 
{
   0.125000f, 0.085113f, 0.067766f, 0.038449f, 0.021465f, 0.027318f, 0.039661f, 0.077798f,
   0.085113f, 0.052595f, 0.045096f, 0.019741f, 0.015475f, 0.019696f, 0.028594f, 0.056090f,
   0.067766f, 0.045096f, 0.022227f, 0.013971f, 0.016428f, 0.020910f, 0.030356f, 0.059545f,
   0.038449f, 0.019741f, 0.013971f, 0.015523f, 0.018254f, 0.023233f, 0.033729f, 0.066163f,
   0.021465f, 0.015475f, 0.016428f, 0.018254f, 0.021465f, 0.027318f, 0.039661f, 0.077798f,
   0.027318f, 0.019696f, 0.020910f, 0.023233f, 0.027318f, 0.034771f, 0.050480f, 0.099019f,
   0.039661f, 0.028594f, 0.030356f, 0.033729f, 0.039661f, 0.050480f, 0.073285f, 0.143753f,
   0.077798f, 0.056090f, 0.059545f, 0.066163f, 0.077798f, 0.099019f, 0.143753f, 0.281982f
};

void make_aanscaledqtable(float * idct_aanscaledqtable,float * fdct_aanscaledqtable, float * qtable)
{
	int i = 0;
	for LOOP(row,8)
	{
	  for LOOP(col,8)
	  {
		dct_float qt = qtable ? qtable[i] : 1.f;
		
	    double aans = aanscalefactor[row] * aanscalefactor[col];
	    double qtaans = qt * aans;

	    idct_aanscaledqtable[i] = (float)( qtaans  / 8.0 );
	    fdct_aanscaledqtable[i] = (float)( 1.0 / (qtaans * 8.0) );
	
	    i++;
	  }
	}
}

void idctf(const float * in_data, float * out_data, const float * aanscaledqtable)
{
	float workspace[64]; // buffers data between passes

  /* Pass 1: process columns from input, store into work array. */

	const float * inptr = in_data;
	float * wsptr = workspace;
	
	const float * quantptr = aanscaledqtable;
		
  for (int ctr = DCTSIZE; ctr > 0; ctr--) 
  {
    /* Even part */

#define GET_COLUMN(i)	( inptr[DCTSIZE*i] * quantptr[DCTSIZE*i] )

    float tmp0 = GET_COLUMN(0);
        
    {    
		dct_float tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
		dct_float tmp10, tmp11, tmp12, tmp13;
		dct_float z5, z10, z11, z12, z13;

		tmp1 = GET_COLUMN(2);
		tmp2 = GET_COLUMN(4);
		tmp3 = GET_COLUMN(6);

		tmp10 = tmp0 + tmp2;	/* phase 3 */
		tmp11 = tmp0 - tmp2;

		tmp13 = tmp1 + tmp3;	/* phases 5-3 */
		tmp12 = (tmp1 - tmp3) * ((dct_float) 1.414213562) - tmp13; /* 2*c4 */

		tmp0 = tmp10 + tmp13;	/* phase 2 */
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;
	    
		/* Odd part */

		tmp4 = GET_COLUMN(1);
		tmp5 = GET_COLUMN(3);
		tmp6 = GET_COLUMN(5);
		tmp7 = GET_COLUMN(7);

		z13 = tmp6 + tmp5;		/* phase 6 */
		z10 = tmp6 - tmp5;
		z11 = tmp4 + tmp7;
		z12 = tmp4 - tmp7;

		tmp7 = z11 + z13;		/* phase 5 */
		tmp11 = (z11 - z13) * ((dct_float) 1.414213562); /* 2*c4 */

		z5 = (z10 + z12) * ((dct_float) 1.847759065); /* 2*c2 */
		tmp10 = ((dct_float) 1.082392200) * z12 - z5; /* 2*(c2-c6) */
		tmp12 = ((dct_float) -2.613125930) * z10 + z5; /* -2*(c2+c6) */

		tmp6 = tmp12 - tmp7;	/* phase 2 */
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		wsptr[DCTSIZE*0] = tmp0 + tmp7;
		wsptr[DCTSIZE*1] = tmp1 + tmp6;
		wsptr[DCTSIZE*2] = tmp2 + tmp5;
		wsptr[DCTSIZE*3] = tmp3 - tmp4;
		wsptr[DCTSIZE*4] = tmp3 + tmp4;
		wsptr[DCTSIZE*5] = tmp2 - tmp5;
		wsptr[DCTSIZE*6] = tmp1 - tmp6;
		wsptr[DCTSIZE*7] = tmp0 - tmp7;
	}
	
    /* advance pointers to next column */
    wsptr++;
    quantptr++;
    inptr++;
  }
  
  /* Pass 2: process rows from work array, store into output array. */
  /* Note that we must descale the results by a factor of 8 == 2**3. */

  wsptr = workspace;
  dct_float * outptr = out_data;
  for (int ctr = 0; ctr < DCTSIZE; ctr++) 
  {
	dct_float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	dct_float tmp10, tmp11, tmp12, tmp13;
	dct_float z5, z10, z11, z12, z13;
    
    /* Even part */

    tmp10 = wsptr[0] + wsptr[4];
    tmp11 = wsptr[0] - wsptr[4];

    tmp13 = wsptr[2] + wsptr[6];
    tmp12 = (wsptr[2] - wsptr[6]) * ((dct_float) 1.414213562) - tmp13;

    tmp0 = tmp10 + tmp13;
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;

    /* Odd part */

    z13 = wsptr[5] + wsptr[3];
    z10 = wsptr[5] - wsptr[3];
    z11 = wsptr[1] + wsptr[7];
    z12 = wsptr[1] - wsptr[7];

    tmp7 = z11 + z13;
    tmp11 = (z11 - z13) * ((dct_float) 1.414213562);

    z5 = (z10 + z12) * ((dct_float) 1.847759065); /* 2*c2 */
    tmp10 = ((dct_float) 1.082392200) * z12 - z5; /* 2*(c2-c6) */
    tmp12 = ((dct_float) -2.613125930) * z10 + z5; /* -2*(c2+c6) */

    tmp6 = tmp12 - tmp7;
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 + tmp5;

    /* Final output stage: scale down by a factor of 8 and range-limit */

    outptr[0] = (tmp0 + tmp7);
    outptr[7] = (tmp0 - tmp7);
    outptr[1] = (tmp1 + tmp6);
    outptr[6] = (tmp1 - tmp6);
    outptr[2] = (tmp2 + tmp5);
    outptr[5] = (tmp2 - tmp5);
    outptr[4] = (tmp3 + tmp4);
    outptr[3] = (tmp3 - tmp4);
    
	outptr += DCTSIZE;
    wsptr  += DCTSIZE;		/* advance pointer to next row */
  }
}
	
void fdctf(const float * in_data, float * out_data, const float * aanscaledqtable)
{
  /* Pass 1: process rows. */

  const float * in_ptr = in_data;
  dct_float * dataptr = out_data;
  for (int ctr = DCTSIZE-1; ctr >= 0; ctr--) 
  {
	dct_float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	dct_float tmp10, tmp11, tmp12, tmp13;
	dct_float z1, z2, z3, z4, z5, z11, z13;

    tmp0 = in_ptr[0] + in_ptr[7];
    tmp7 = in_ptr[0] - in_ptr[7];
    tmp1 = in_ptr[1] + in_ptr[6];
    tmp6 = in_ptr[1] - in_ptr[6];
    tmp2 = in_ptr[2] + in_ptr[5];
    tmp5 = in_ptr[2] - in_ptr[5];
    tmp3 = in_ptr[3] + in_ptr[4];
    tmp4 = in_ptr[3] - in_ptr[4];
    
    /* Even part */
    
    tmp10 = tmp0 + tmp3;	/* phase 2 */
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;
    
    dataptr[0] = tmp10 + tmp11; /* phase 3 */
    dataptr[4] = tmp10 - tmp11;
    
    z1 = (tmp12 + tmp13) * ((dct_float) 0.707106781); /* c4 */
    dataptr[2] = tmp13 + z1;	/* phase 5 */
    dataptr[6] = tmp13 - z1;
    
    /* Odd part */

    tmp10 = tmp4 + tmp5;	/* phase 2 */
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    /* The rotator is modified from fig 4-8 to avoid extra negations. */
    z5 = (tmp10 - tmp12) * ((dct_float) 0.382683433); /* c6 */
    z2 = ((dct_float) 0.541196100) * tmp10 + z5; /* c2-c6 */
    z4 = ((dct_float) 1.306562965) * tmp12 + z5; /* c2+c6 */
    z3 = tmp11 * ((dct_float) 0.707106781); /* c4 */

    z11 = tmp7 + z3;		/* phase 5 */
    z13 = tmp7 - z3;

    dataptr[5] = z13 + z2;	/* phase 6 */
    dataptr[3] = z13 - z2;
    dataptr[1] = z11 + z4;
    dataptr[7] = z11 - z4;

    dataptr += DCTSIZE;		/* advance pointer to next row */
    in_ptr += DCTSIZE;
  }

  /* Pass 2: process columns. */

  dataptr = out_data;
  for (int ctr = DCTSIZE-1; ctr >= 0; ctr--) 
  {
	dct_float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	dct_float tmp10, tmp11, tmp12, tmp13;
	dct_float z1, z2, z3, z4, z5, z11, z13;

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
    tmp7 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
    tmp6 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
    tmp5 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];
    tmp4 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];
    
    /* Even part */
    
    tmp10 = tmp0 + tmp3;	/* phase 2 */
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;
    
    dataptr[DCTSIZE*0] = tmp10 + tmp11; /* phase 3 */
    dataptr[DCTSIZE*4] = tmp10 - tmp11;
    
    z1 = (tmp12 + tmp13) * ((dct_float) 0.707106781); /* c4 */
    dataptr[DCTSIZE*2] = tmp13 + z1; /* phase 5 */
    dataptr[DCTSIZE*6] = tmp13 - z1;
    
    /* Odd part */

    tmp10 = tmp4 + tmp5;	/* phase 2 */
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    /* The rotator is modified from fig 4-8 to avoid extra negations. */
    z5 = (tmp10 - tmp12) * ((dct_float) 0.382683433); /* c6 */
    z2 = ((dct_float) 0.541196100) * tmp10 + z5; /* c2-c6 */
    z4 = ((dct_float) 1.306562965) * tmp12 + z5; /* c2+c6 */
    z3 = tmp11 * ((dct_float) 0.707106781); /* c4 */

    z11 = tmp7 + z3;		/* phase 5 */
    z13 = tmp7 - z3;

    dataptr[DCTSIZE*5] = z13 + z2; /* phase 6 */
    dataptr[DCTSIZE*3] = z13 - z2;
    dataptr[DCTSIZE*1] = z11 + z4;
    dataptr[DCTSIZE*7] = z11 - z4;

    dataptr++;			/* advance pointer to next column */
  }

  /* step 3: Quantize/descale the coefficients, and store into coef_blocks[] */
      for (int i = 0; i < DCTSIZE*DCTSIZE; i++) 
      {
		/* Apply the quantization and scaling factor */
				
		//out_ptr[i] = quantize( data[i], state->fdct_divisors[i] );
		
		// apply the aann multiplier to data[i] so that dct floats output gets scaled
		//	this makes the float->float Dct unitary
		//	(fdct_aann_multiplier also has the quantization table in it,
		//	 so it's only unitary if you chose qtable = flat)
		out_data[i] *= aanscaledqtable[i];
      }
}

//};

//=============================================================

namespace MiniIDCT
{
	// integer IDCT from MiniJPEG
	//	quality of this is pretty good, very close to float idct
	
    enum 
    {
        W1 = 2841,
        W2 = 2676,
        W3 = 2408,
        W5 = 1609,
        W6 = 1108,
        W7 = 565,
    };

	typedef float MiniIDCT_out_type;

    inline void _RowIDCT(int* blk)
    {
        int x0, x1, x2, x3, x4, x5, x6, x7, x8;

        x0 = (blk[0] << 11) + 128;
        x1 = blk[4] << 11;
        x2 = blk[6];
        x3 = blk[2];
        x4 = blk[1];
        x5 = blk[7];
        x6 = blk[5];
        x7 = blk[3];
            
        x8 = W7 * (x4 + x5);
        x4 = x8 + (W1 - W7) * x4;
        x5 = x8 - (W1 + W7) * x5;
        x8 = W3 * (x6 + x7);
        x6 = x8 - (W3 - W5) * x6;
        x7 = x8 - (W3 + W5) * x7;
        x8 = x0 + x1;
        x0 -= x1;
        x1 = W6 * (x3 + x2);
        x2 = x1 - (W2 + W6) * x2;
        x3 = x1 + (W2 - W6) * x3;
        x1 = x4 + x6;
        x4 -= x6;
        x6 = x5 + x7;
        x5 -= x7;
        x7 = x8 + x3;
        x8 -= x3;
        x3 = x0 + x2;
        x0 -= x2;
        x2 = (181 * (x4 + x5) + 128) >> 8;
        x4 = (181 * (x4 - x5) + 128) >> 8;
        blk[0] = (x7 + x1) >> 8;
        blk[1] = (x3 + x2) >> 8;
        blk[2] = (x0 + x4) >> 8;
        blk[3] = (x8 + x6) >> 8;
        blk[4] = (x8 - x6) >> 8;
        blk[5] = (x0 - x4) >> 8;
        blk[6] = (x3 - x2) >> 8;
        blk[7] = (x7 - x1) >> 8;
    }

    inline void _ColIDCT(const int* blk, MiniIDCT_out_type *out, int out_stride)
    {
        int x0, x1, x2, x3, x4, x5, x6, x7, x8;
        
        x0 = (blk[0] << 8) + 8192;
        x1 = blk[8*4] << 8;
        x2 = blk[8*6];
        x3 = blk[8*2];
        x4 = blk[8*1];
        x5 = blk[8*7];
        x6 = blk[8*5];
        x7 = blk[8*3];
            
        x8 = W7 * (x4 + x5) + 4;
        x4 = (x8 + (W1 - W7) * x4) >> 3;
        x5 = (x8 - (W1 + W7) * x5) >> 3;
        x8 = W3 * (x6 + x7) + 4;
        x6 = (x8 - (W3 - W5) * x6) >> 3;
        x7 = (x8 - (W3 + W5) * x7) >> 3;
        x8 = x0 + x1;
        x0 -= x1;
        x1 = W6 * (x3 + x2) + 4;
        x2 = (x1 - (W2 + W6) * x2) >> 3;
        x3 = (x1 + (W2 - W6) * x3) >> 3;
        x1 = x4 + x6;
        x4 -= x6;
        x6 = x5 + x7;
        x5 -= x7;
        x7 = x8 + x3;
        x8 -= x3;
        x3 = x0 + x2;
        x0 -= x2;
        x2 = (181 * (x4 + x5) + 128) >> 8;
        x4 = (181 * (x4 - x5) + 128) >> 8;
        
        *out = (MiniIDCT_out_type) (((x7 + x1) >> 14) );  out += out_stride;
        *out = (MiniIDCT_out_type) (((x3 + x2) >> 14) );  out += out_stride;
        *out = (MiniIDCT_out_type) (((x0 + x4) >> 14) );  out += out_stride;
        *out = (MiniIDCT_out_type) (((x8 + x6) >> 14) );  out += out_stride;
        *out = (MiniIDCT_out_type) (((x8 - x6) >> 14) );  out += out_stride;
        *out = (MiniIDCT_out_type) (((x0 - x4) >> 14) );  out += out_stride;
        *out = (MiniIDCT_out_type) (((x3 - x2) >> 14) );  out += out_stride;
        *out = (MiniIDCT_out_type) (((x7 - x1) >> 14) );
    }

	// WARNING : this does mutate the input block
	void MiniIDCT(int * iblock,MiniIDCT_out_type * out, int out_stride)
	{
		for LOOP(i,8)
			_RowIDCT(&iblock[i*8]);
		    
		for LOOP(i,8)
			_ColIDCT(&iblock[i], &out[i] , out_stride);
	}

};

END_CB
