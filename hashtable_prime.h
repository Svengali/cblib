#pragma once

#include <stl_basics.h>
#include <vector.h>
#include <FloatUtil.h> // for intlog2ceil
#include <hash_function.h>
#include <hash_table.h>

/***********

hashtable_prime 

DEPRECATED

prime hash really is too slow

*************/

START_CB

#define c_HASH_PRIME_SIZE 32
extern const uint32 c_prime_list[c_HASH_PRIME_SIZE];

//=======================================================================

//=======================================================================

// could do a default template arg like this :
// = hashtable_prime_ops<t_key,t_key(0),t_key(1)>

template <typename t_key,typename t_data,typename t_ops> class hashtable_prime
{
public:
	typename struct table_entry
	{
		hash_type	hash;
		t_key		key;
		t_data		data;
		
		table_entry() : hash() , key() , data() { operations().make_empty(hash,key); }
	};
	
	typedef t_ops										operations;
	typedef hashtable_prime<t_key,t_data,t_ops>				this_type;
	typedef typename hashtable_prime<t_key,t_data,t_ops>::table_entry	entry_type;
	typedef typename const hashtable_prime<t_key,t_data,t_ops>::table_entry * entry_ptr;
	
	//-------------------------------------------

	hashtable_prime() : m_hashSize(0), m_numInserts(0), m_numDeleted(0), m_numInsertsRebuild(0), m_hashFillRatio(CB_DEFAULT_HASH_FILL_RATIO)
	 { }
	~hashtable_prime() 
	 { }

	//-------------------------------------------
	
	int capacity() const;
	void reserve(int count);
	
	void clear();
	void release();
	void tighten();
	
	int size() const;
	bool empty() const;

	//-------------------------------------------
	
	// find returns NULL if not found
	const table_entry * find(const hash_type & hash,const t_key & key) const;
	
	// insert returns the one just made
	const table_entry * insert(const hash_type & hash,const t_key & key,const t_data & data);

	void erase(const table_entry * entry);

	// aliases :
	
	const table_entry * find(const t_key & key) const;
	const table_entry * insert(const t_key & key,const t_data & data);
	const table_entry * insert(const table_entry & entry);
	
	//-------------------------------------------
	
	void set_fill_ratio(float f)
	{
		m_hashFillRatio = f;
		m_numInsertsRebuild = ftoi( m_hashSize * m_hashFillRatio );
	}
	float get_fill_ratio() const { return m_hashFillRatio; }
	
	//-------------------------------------------
	
	class iterator
	{
	public:
		iterator() : owner(NULL), index(0) { }
		iterator(const this_type * _owner,int _index) : owner(_owner), index(_index) { }
		~iterator() { }
		//iterator(const iterator & rhs)
		
		// pre-increment :
		iterator & operator ++ () { index = owner->iterator_next(index); return *this; }
		/*
		// post-increment :
		iterator & operator ++ (int)
		{
			iterator save(*this);
			index = owner->iterator_next(index);
			return save;
		}
		*/
	
		const entry_type * operator -> () const { return owner->iterator_at(index); }
		const entry_type & operator * () const { return * owner->iterator_at(index); }
		
		bool operator == (const iterator & rhs) const { return owner == rhs.owner && index == rhs.index; }
		bool operator != (const iterator & rhs) const { return ! (*this == rhs); }
	
	private:
		friend class this_type;
		const this_type *	owner;
		int			index;
	};
	
	// head/tail like begin/end :
	//	for walking over all members
	iterator head() const;
	iterator tail() const;
	
	const entry_type * iterator_at(const int it) const;
	int iterator_next(const int it) const;
	
	//-------------------------------------------
	
private:

	void rebuild_table(int newSize);

	int table_index(const hash_type hash) const;
	int table_reindex(const int index,const hash_type hash,const int probeCount) const;

	cb::vector<table_entry>	m_table;
	uint32		m_hashSize;
	uint32		m_numInserts;		// occupancy is the number of *inserts* (not decremented when you remove members)
	uint32		m_numDeleted;		// actual number of items = occupancy - numDeleted
	uint32		m_numInsertsRebuild; //,occupancyDownSize;
	
	float		m_hashFillRatio;
};

//=========================================================================================

#define T_PRE1	template <typename t_key,typename t_data,typename t_ops>
#define T_PRE2	hashtable_prime<t_key,t_data,t_ops>
#define T_TABLEENTRY	typename hashtable_prime<t_key,t_data,t_ops>::entry_type
#define T_ITERATOR	typename hashtable_prime<t_key,t_data,t_ops>::iterator

T_PRE1 int T_PRE2::size() const
{
	return m_numInserts - m_numDeleted;
}

T_PRE1 int T_PRE2::capacity() const
{
	return m_table.size();
}

T_PRE1 bool T_PRE2::empty() const
{
	return size() == 0;
}

T_PRE1 void T_PRE2::reserve(int count)
{
	if ( count <= capacity() )
		return;
	
	rebuild_table(count);
}

T_PRE1 void T_PRE2::clear()
{
	// clear out table but don't free :
	m_numInserts = 0;
	m_numDeleted = 0;
	for(int i=0;i<m_table.size();i++)
	{
		make_empty( m_table[i].hash, m_table[i].key );
		m_table[i].data = t_data();
	}
}	

T_PRE1 void T_PRE2::release()
{
	m_table.release();
	m_hashSize = 0;
	m_numInserts = 0;
	m_numDeleted = 0;
	m_numInsertsRebuild = 0;
}

T_PRE1 void T_PRE2::tighten()
{
	if ( m_numDeleted == 0 )
		return;
	rebuild_table(0);
}

T_PRE1 int T_PRE2::table_reindex(const int index,const hash_type hash,const int probeCount) const
{
	// triangular step :
	return (int) ( ( index + probeCount ) % m_hashSize );
}

T_PRE1 int T_PRE2::table_index(const hash_type hash) const
{
	// xor fold : @@ ??
	//return (int) ( ( (hash>>16) ^ hash ) % m_hashSize);
	return hash % m_hashSize;
}
	
T_PRE1 void T_PRE2::rebuild_table(int newOccupancy)
{
	if ( m_table.empty() )
	{
		// first time :
		const int c_minHashCount = 16; // this will turn into 64 after the rounding up below ...
		newOccupancy = MAX(newOccupancy, c_minHashCount );

		// newSize = ( newOccupancy / m_hashFillRatio ) would put us right at the desired fill
		//	so bias up a little bit (assume more inserts will follow)

		// alternative way : just bias a little but ceil the log2
		//float newSize = 1.1f * newOccupancy / m_hashFillRatio;
		//m_hashBits = intlog2ceil(newSize);
		
		int minNewSize = ftoi( 1.2f * newOccupancy / m_hashFillRatio );

		for(int i=0;i<c_HASH_PRIME_SIZE;i++)
		{
			if ( c_prime_list[i] >= (uint32)minNewSize )
			{
				m_hashSize = c_prime_list[i];
				break;
			}
		}
				
		table_entry zero; // = { 0 };
		m_table.resize(m_hashSize,zero);
		
		m_numInserts = 0;
		m_numDeleted = 0;
		m_numInsertsRebuild = ftoi( m_hashSize * m_hashFillRatio );
		
		// make sure that just reinserting the existing items won't cause a rebuild :
		ASSERT( m_numInsertsRebuild > (uint32)newOccupancy );
	}
	else
	{
		// rebuild :
		
		cb::vector<table_entry>	old_table;
		m_table.swap(old_table);
		ASSERT( m_table.empty() );
		
		int occupancy = m_numInserts - m_numDeleted + 16;
		newOccupancy = MAX(occupancy,newOccupancy);
		// call self : should get the other branch
		rebuild_table(newOccupancy);
		
		// not true if I'm rebuilding because of deletes
		//rrassert( sh->hashMask > newSize );
		//rrassert( sh->occupancyUpSize > newSize );
		
		for(int i=0;i<old_table.size32();i++)
		{
			if ( operations().is_empty(  old_table[i].hash,old_table[i].key) ||
				 operations().is_deleted(old_table[i].hash,old_table[i].key) )
			{
				continue;
			}

			insert(old_table[i]);
		}
		
		// old_table descructs here
	}
}

T_PRE1 const T_TABLEENTRY * T_PRE2::find(const hash_type & hash,const t_key & key) const
{
	// don't ever try to insert the magic values :
	ASSERT( ! operations().is_deleted(hash,key) && ! operations().is_empty(hash,key) );
	
	int index = table_index(hash);
	int probeCount = 1;
	
	const table_entry * table = m_table.begin();
		
	while ( ! operations().is_empty(table[index].hash,table[index].key) )
	{
		if ( table[index].hash == hash && 
			! operations().is_deleted(table[index].hash,table[index].key) &&
			operations().key_equal(table[index].key,key) )
		{
			// found it
			return &table[index];
		}
	
		index = table_reindex(index,hash,probeCount);
		probeCount++;
	}
	
	return NULL;
}

T_PRE1 const T_TABLEENTRY * T_PRE2::insert(const hash_type & hash,const t_key & key,const t_data & data)
{
	// don't ever try to insert the magic values :
	ASSERT( ! operations().is_deleted(hash,key) && ! operations().is_empty(hash,key) );

	// this triggers the first table build when m_numInserts == m_numInsertsRebuild == 0
	if ( m_numInserts >= m_numInsertsRebuild )
	{
		rebuild_table(0);
	}
	m_numInserts ++;

	int index = table_index(hash);
	int probeCount = 1;
		
	table_entry * table = m_table.begin();
	
	while ( ! operations().is_empty(table[index].hash,table[index].key) &&
			! operations().is_deleted(table[index].hash,table[index].key) )
	{
		// @@ I could check to see if this exact same object is in here already and just not add it
		//	return pointer to the previous
		// currently StringHash is a "multi" hash - you can add the same key many times with different data
		//	and get them out with Find_Start / Find_Next
	
		//s_collisions++;
		index = table_reindex(index,hash,probeCount);
		probeCount++;
	}
		
	table[index].hash = hash;
	table[index].key = key;
	table[index].data = data;
	
	return &table[index];
}

T_PRE1 void T_PRE2::erase(const table_entry * entry)
{
	ASSERT( entry >= m_table.begin() && entry < m_table.end() );
	table_entry * el = const_cast< table_entry *>(entry);

	operations().make_deleted(el->hash,el->key);	
	//WARNING : leave el->hs.hash the same for count !!
	
	m_numDeleted++;
	
	// do NOT decrease occupancy
}

// aliases :

T_PRE1 const T_TABLEENTRY * T_PRE2::find(const t_key & key) const
{
	return find(hash_function(key),key);
}

T_PRE1 const T_TABLEENTRY * T_PRE2::insert(const t_key & key,const t_data & data)
{
	return insert(hash_function(key),key,data);
}

T_PRE1 const T_TABLEENTRY * T_PRE2::insert(const table_entry & entry)
{
	return insert(entry.hash,entry.key,entry.data);
}
	
T_PRE1 T_ITERATOR T_PRE2::head() const
{
	iterator it(this,-1);
	++it;
	return it;
}

// tail is *past* the last valid one
T_PRE1 T_ITERATOR T_PRE2::tail() const
{
	iterator it(this,m_table.size());
	return it;
}
	
T_PRE1 const T_TABLEENTRY * T_PRE2::iterator_at(const int it) const
{
	if ( it < 0 || it >= m_table.size() ) return NULL;
	const entry_type * E = & m_table[it];
	ASSERT( ! operations().is_deleted(E->hash,E->key) && ! operations().is_empty(E->hash,E->key) );
	return E;
}

T_PRE1 int T_PRE2::iterator_next(int index) const
{
	// step past last :
	++index;
	
	// don't go past tail
	int size = m_table.size();
	if ( index >= size ) return size; // return tail;
	
	// look for a non-empty one :
	while ( operations().is_empty(m_table[index].hash,m_table[index].key) ||
			operations().is_deleted(m_table[index].hash,m_table[index].key) )
	{
		++index;
		if ( index >= size ) return size; // return tail;
	}
	return index;
}
	
	
#undef T_PRE1
#undef T_PRE2
#undef T_TABLEENTRY
#undef T_ITERATOR

//===================================================================

END_CB
