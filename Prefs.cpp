#include "cblib/Prefs.h"
#include "cblib/TokenHash.h"
#include "cblib/FileUtil.h"
#include "cblib/Log.h"
#include <time.h>

#include <hash_map>

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
	typedef stdext::hash_map<Token,PrefsPtr> t_hash;
	t_hash g_hash;

	Prefs * GetExisting(const char * resourceName)
	{
		ASSERT( resourceName != NULL );
		Token tok(resourceName);

		t_hash::iterator it = g_hash.find(tok);

		if ( it == g_hash.end() )
			return NULL;

		return it->second.GetPtr();
	}
	
	void Add(const PrefsPtr & pref,const char * resourceName)
	{
		ASSERT( pref != NULL );
		pref->SetResourceName(resourceName);
		pref->SetLoadTime( time(NULL) );
		Token tok(resourceName);
		g_hash.insert( t_hash::value_type(tok,pref) );
		
		// auto save when we make a default one (??)
		if ( ! FileExists(resourceName) )
		{
			pref->Save();
		}
	}
	
	void Flush()
	{
		for(t_hash::iterator it = g_hash.begin();
			it != g_hash.end();
			++it)
		{
			if ( it->second->GetRefCount() == 1 )
			{
				// I have the only ref !
				it->second = NULL;
				g_hash.erase(it);
			}
		}
	}
	
	bool ReloadChanged()
	{
		bool changes = false;
		for(t_hash::iterator it = g_hash.begin();
			it != g_hash.end();
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
	
	void ReloadAll()
	{
		for(t_hash::iterator it = g_hash.begin();
			it != g_hash.end();
			++it)
		{
			it->second->Reload();
		}
	}
	void SaveAll()
	{
		for(t_hash::iterator it = g_hash.begin();
			it != g_hash.end();
			++it)
		{
			it->second->Save();
		}
	}
	
	void Shutdown()
	{
		g_hash = t_hash();
	}
}

END_CB
