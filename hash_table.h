#pragma once

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS

#include "hashtable.h"

START_CB

template <typename t_key,typename t_data,typename t_ops> class hash_table :
	public hashtable< hashtableentry_hkd<t_key,t_data,t_ops> >
{
public:

	hash_table() { }
	~hash_table() { }
};

END_CB

