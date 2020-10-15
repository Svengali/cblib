#pragma once

#include "Base.h"

START_CB

//-----------------------------------------------------------

namespace Journal
{
	enum EMode
	{
		eNone,
		eSaving,
		eLoading
	};

	void StartSaving();
	void StartLoading();

	EMode GetMode();

	void Read(void * bits,int size);
	void Write(const void * bits,int size);

	void Checkpoint();

	char getch();

	inline bool IsSaving()  { return GetMode() == eSaving; }
	inline bool IsLoading() { return GetMode() == eLoading; }
	
	template <typename T>
	void IO(T & data)
	{
		if ( IsSaving() )
		{
			Write(&data,sizeof(T));
		}
		else if ( IsLoading() )
		{
			Read(&data,sizeof(T));
		}
	}
};

//-----------------------------------------------------------

END_CB
