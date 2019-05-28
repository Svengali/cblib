#include "tweakvar.h"
#include "cblib/vector.h"
#include "cblib/String.h"
#include "cblib/SPtr.h"
#include "cblib/FileUtil.h"
#include "cblib/Log.h"
#include "cblib/MoreUtil.h"

/*************

TweakVar currently just polls modtimes to find changes
could use a DirChangeWatcher for more speed

Tweak Register calls -

file scope ones come in during CINIT

ones in functions don't register until the var is touched for the first time


*************/

#if CBLIB_TWEAKVAR_ENABLED

START_CB

SPtrDef(TweakVarBase);

class TweakableFile
{
public:

	String	m_name;
	time_t	m_lastModTime;
	
	vector<TweakVarBasePtr> m_vars;
};

void ParseChangedFile(TweakableFile * TF)
{
	lprintf("TweakVar : %s\n",TF->m_name.CStr());

	char * buf = ReadWholeFile( TF->m_name.CStr() );

	char * ptr = buf;
	
	// look for "TWEAK_VAL"
	
	const char * searchFor = "TWEAK_VAL";
	
	for(;;)
	{
		// try to parse and handle C++ comments right :
	
		// keep stepping until I see "searchFor" :
		ptr = skipwhitespace(ptr);
		ptr = SkipCppComments(ptr);
		if ( ptr == NULL || *ptr == 0 )
			break;		
	
		if ( ! strpresame(ptr,searchFor) )
		{
			ptr++;
			continue;
		}
		
		// saw it
		ptr += strlen(searchFor);
		
		// #define TWEAK_VAL(type, var, val )	
		
		// skip past type :
		ptr = strchr(ptr,',');
		if ( ptr == NULL )
			break;
		ptr++;
		
		ptr = skipwhitespace(ptr);
		
		// point at var name :
		char * pname = ptr;
		
		ptr = strchr(ptr,',');
		if ( ptr == NULL )
			break;
		*ptr = 0; // terminate var name
		ptr++;
		
		// point at value :
		char * pval = skipwhitespace(ptr);
		
		ptr = strchr(ptr,')');
		if ( ptr == NULL )
			break;
		*ptr = 0;
		ptr++;
		
		// kill white at end of var name :
		killtailingwhite(pname);
		
		// look for pname in TF->m_vars :
		bool found = false;
		for(int vi=0; vi < TF->m_vars.size32(); vi++)
		{
			const char * vsname = TF->m_vars[vi]->GetName();
			if ( strsame(pname,vsname) )
			{
				// match
				TF->m_vars[vi]->SetFromText( pval );
				
				found = true;
				break;
			}			
		}
		if ( ! found )
		{
			lprintf("WARNING : tweak var not found : %s\n",pname);
		}		
	}
}

//---------------------------------------------------------------

class TweakVarSingletonImpl : public TweakVarSingleton
{
public:

	vector<TweakableFile>	m_tweakFiles;

	TweakVarSingletonImpl()
	{
		m_tweakFiles.reserve(16);
	}
	
	~TweakVarSingletonImpl()
	{
		
	}	
};

//---------------------------------------------------------------
// singleton :

static TweakVarSingleton * s_theTweakVarSingleton = NULL;

TweakVarSingleton * TweakVarSingleton::The()
{
	if ( ! s_theTweakVarSingleton )
	{
		s_theTweakVarSingleton = new TweakVarSingletonImpl;
	}
	return s_theTweakVarSingleton;
}

void TweakVarSingleton::DestroyThe()
{
	if ( s_theTweakVarSingleton )
	{
		delete s_theTweakVarSingleton;
		s_theTweakVarSingleton = NULL;
	}
}

TweakVarSingleton::TweakVarSingleton()
{
}

TweakVarSingleton::~TweakVarSingleton()
{
}

//---------------------------------------------------------------

void TweakVarSingleton::RegisterBase( const char * fileName, TweakVarBase * pBase )
{
	TweakVarSingletonImpl * pimpl = (TweakVarSingletonImpl *)this;

	//lprintf("TweakVar::Register : %s : %s\n",fileName,pBase->GetName());

	// find file :
	TweakableFile * TF = NULL;
	for(int i=0;i < pimpl->m_tweakFiles.size32(); i++)
	{
		if ( pimpl->m_tweakFiles[i].m_name == fileName )
		{
			TF = &(pimpl->m_tweakFiles[i]);
			break;
		}
	}
	
	// if not found, add :
	if ( TF == NULL )
	{
		pimpl->m_tweakFiles.push_back();
		TF = &(pimpl->m_tweakFiles.back());
		TF->m_name = fileName;
		
		//TF->m_lastModTime = FileModTime(fileName);
		
		// add with modtime = 0
		//	this forces us to reparse on startup !
		// that's good !
		TF->m_lastModTime = 0;
	}
	
	// put var in file list :
	TF->m_vars.push_back( TweakVarBasePtr(pBase) );
}
	
bool TweakVarSingleton::CheckChanges()
{
	TweakVarSingletonImpl * pimpl = (TweakVarSingletonImpl *)this;

	bool anyChanged = false;

	for(int i=0;i < pimpl->m_tweakFiles.size32(); i++)
	{
		TweakableFile * TF = &(pimpl->m_tweakFiles[i]);

		time_t curModTime = FileModTime(TF->m_name.CStr());
		if ( curModTime == TF->m_lastModTime )
			continue;
		
		TF->m_lastModTime = curModTime;
		
		// parse it :
		
		ParseChangedFile(TF);
	
		anyChanged = true;
	}
	
	return anyChanged;	
}

// StartDirWatcher is optional
//	it starts a DirChangeWatcher to make CheckChanges more efficient
//	the DirChangeWatcher is automatically queried when you CheckChanges
void TweakVarSingleton::GetDirsToWatch(vector<String> * pInto)
{
	TweakVarSingletonImpl * pimpl = (TweakVarSingletonImpl *)this;

	// find the dirs needed from the list of files with tweak vars :

	vector<String> dirs;
	
	for(int i=0;i < pimpl->m_tweakFiles.size32(); i++)
	{
		TweakableFile * TF = &(pimpl->m_tweakFiles[i]);

		char path[_MAX_PATH];
		getpathpart(TF->m_name.CStr(),path);
		
		dirs.push_back( String(path) );
	}
	
	// eliminate duplicates :
	
	std::sort(dirs.begin(),dirs.end());
	vector<String>::iterator it = std::unique(dirs.begin(),dirs.end());
	dirs.erase(it,dirs.end());

	// do it :

	pInto->appendv(dirs);
}

void TweakVarSingleton::LogAll()
{
	TweakVarSingletonImpl * pimpl = (TweakVarSingletonImpl *)this;

	for(int i=0;i < pimpl->m_tweakFiles.size32(); i++)
	{
		TweakableFile * TF = &(pimpl->m_tweakFiles[i]);

		lprintf("TweakFile : ",TF->m_name,"\n");
		
		for LOOPVEC(vi,TF->m_vars)
		{
			const char * name = TF->m_vars[vi]->GetName();
			String value;
			TF->m_vars[vi]->GetAsText(&value);
			lprintf("  ",name," = ",value,"\n");
		}
	}
}

bool TweakVarSingleton::SetFromText(const char * var, const char * val)
{
	TweakVarSingletonImpl * pimpl = (TweakVarSingletonImpl *)this;

	for(int i=0;i < pimpl->m_tweakFiles.size32(); i++)
	{
		TweakableFile * TF = &(pimpl->m_tweakFiles[i]);

		//lprintf("TweakFile : ",TF->m_name,"\n");
		
		for LOOPVEC(vi,TF->m_vars)
		{
			TweakVarBase * pVar = TF->m_vars[vi].GetPtr();
			const char * pName = pVar->GetName();
			if ( strisame(pName,var) )
			{
				pVar->SetFromText(val);
				return true;
			}
		}
	}
	
	return false;
}

END_CB

#endif // CBLIB_TWEAKVAR_ENABLED
