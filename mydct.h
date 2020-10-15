#pragma once

#include "Base.h"

START_CB

//namespace MyDCT
//{

extern const float c_fdct_aanfactors[64];
extern const float c_idct_aanfactors[64];

extern const float c_my_dct_csf_luma[64];
extern const float c_my_dct_csf_chroma[64];

extern const float c_fdct_aanfactors_times_csf_luma[64];
extern const float c_fdct_aanfactors_times_csf_chroma[64];

void make_aanscaledqtable(float * idct_aanscaledqtable,float * fdct_aanscaledqtable, float * qtable = NULL);

void fdctf(const float * in_data, float * out_data, const float * aanscaledqtable = c_fdct_aanfactors);
void idctf(const float * in_data, float * out_data, const float * aanscaledqtable = c_idct_aanfactors);

//};

END_CB