#include "Prefs.h"
#include "TokenHash.h"
#include "FileUtil.h"
#include "Log.h"
#include <time.h>

START_CB

void Prefs::Reload()
{
	ASSERT( m_dispatcher != NULL );

	PrefBlock block(PrefBlock::eRead,m_resourceName.CStr());

	m_dispatcher->IO(block,this);
	
	m_loadTime = time(NULL);
}

void Prefs::Save()
{
	ASSERT( m_dispatcher != NULL );

	{
		PrefBlock block(PrefBlock::eWrite,m_resourceName.CStr());

		m_dispatcher->IO(block,this);
		
		// this would be done by our destructor but easier to trace in if I call it manually :
		block.FinishWriting();
	}
	
	m_loadTime = time(NULL);
}

namespace PrefsMgr
{
	// stupid fucking hash_map does an alloc on construction even when it's empty
	#ifdef _XHASH_
	typedef stdext::hash_map<Token,PrefsPtr> t_hash;
	#else
	typedef std::hash_map<Token,PrefsPtr> t_hash;
	#endif
	static t_hash * g_pHash = NULL;

	void Init()
	{
		if ( g_pHash == NULL )
		{
			g_pHash = new t_hash;
		}
	}

	Prefs * GetExisting(const char * resourceName)
	{
		Init();
	
		ASSERT( resourceName != NULL );
		Token tok(resourceName);

		t_hash::iterator it = g_pHash->find(tok);

		if ( it == g_pHash->end() )
			return NULL;

		return it->second.GetPtr();
	}
	
	void Add(const PrefsPtr & pref,const char * resourceName,bool doAutoSave)
	{
		Init();
		
		ASSERT( pref != NULL );
		pref->SetResourceName(resourceName);
		pref->SetLoadTime( time(NULL) );
		Token tok(resourceName);
		g_pHash->insert( t_hash::value_type(tok,pref) );
		
		// auto save when we make a default one (??)
		if ( doAutoSave && ! FileExists(resourceName) )
		{
			pref->Save();
		}
	}
	
	void Flush()
	{
		Init();
		for(t_hash::iterator it = g_pHash->begin();
			it != g_pHash->end();
			)
		{
			if ( it->second->GetRefCount() == 1 )
			{
				// I have the only ref !
				it->second = NULL;
				t_hash::iterator next = it;
				++next;
				g_pHash->erase(it);
				it = next;
			}
			else
			{
				++it;
			}
		}
	}
	
	bool ReloadChanged()
	{
		Init();
		bool changes = false;
		for(t_hash::iterator it = g_pHash->begin();
			it != g_pHash->end();
			++it)
		{
			PrefsPtr pref = it->second;
			time_t loadTime = pref->GetLoadTime();
			time_t fileTime = FileModTime( pref->GetResourceName() );
			if ( fileTime > loadTime )
			{
				lprintf("Pref changed, reloading (%s)\n",pref->GetResourceName());
				pref->Reload();
				changes = true;
			}
		}
		return changes;
	}
	
	void GetDirsToWatch(vector<String> * pInto)
	{		
		// find the dirs needed from the list of files with tweak vars :

		if ( ! g_pHash )
			return;
			
		vector<String> dirs;
		
		for(t_hash::iterator it = g_pHash->begin();
			it != g_pHash->end();
			++it)
		{
			PrefsPtr pref = it->second;

			char path[_MAX_PATH];
			getpathpart(pref->GetResourceName(),path);
			
			dirs.push_back( String(path) );
		}
		
		// eliminate duplicates :
		
		std::sort(dirs.begin(),dirs.end());
		vector<String>::iterator vit = std::unique(dirs.begin(),dirs.end());
		dirs.erase(vit,dirs.end());

		// do it :

		pInto->appendv(dirs);
	}
	
	void ReloadAll()
	{
		Init();
		for(t_hash::iterator it = g_pHash->begin();
			it != g_pHash->end();
			++it)
		{
			it->second->Reload();
		}
	}
	void SaveAll()
	{
		Init();
		for(t_hash::iterator it = g_pHash->begin();
			it != g_pHash->end();
			++it)
		{
			it->second->Save();
		}
	}
	
	void Shutdown()
	{
		delete g_pHash;
		g_pHash = NULL;
	}
}

END_CB
