#pragma once

#include "cblib/Base.h"

START_CB

// Link : simple linked list template with no allocations
  
//{=======================================================================

class LinkBase
{
public:
	/////////////////////////////////////
	 LinkBase()
	{
		InitLinks();
	} // can't do initializer list cuz it uses this pointer

	~LinkBase()
	{
		Cut();
	}
	
	//! makes this a list of no elements; does NOT remove it if it's in a list
	void Reset()
	{
		InitLinks();
	}

	///////////////////////////////

	const LinkBase * GetNext() const	{ ASSERT( IsValid() ); return m_next; }
		  LinkBase * GetNext()			{ ASSERT( IsValid() ); return m_next; }
	const LinkBase * GetPrev() const	{ ASSERT( IsValid() ); return m_prev; }
		  LinkBase * GetPrev()			{ ASSERT( IsValid() ); return m_prev; }

	///////////////////////////////

	//! remove "this" from the list it's in
	void Cut();
	
	//! add pNode to the list, after "this"
	//!	pNode should not be any lists at this type
	void AddAfter( LinkBase * pNode );

	//! add pNode to the list, before "this"
	//!	pNode should not be any lists at this type
	void AddBefore( LinkBase * pNode );

	//! take pList, another list, and add it into the current list in
	//!	the spot right after 'this'
	//! notez : if pList is already in this list, that's bad news !!
	void AddListAfter(  LinkBase * pList );
	void AddListBefore( LinkBase * pList );

	///////////////////////////////

	bool IsEmpty() const
	{
		ASSERT( IsValid() );
		return (m_next == this);
	}

	//! GetListLength : expensive; walks the list
	int GetListLength() const;

	bool ListIsValid() const;

	bool ListContains(const LinkBase * pNode) const;

	bool IsValid() const;

	/////////////////////////////////////

protected:

	// must allow them for evil users of vector<Link> (it's the freaking Galaxy gMeshConnected)
	// must allow them for evil users of vector<Link>
	//FORBID_COPY_CTOR(LinkBase);
	//FORBID_ASSIGNMENT(LinkBase);

	void Fix()
	{
		m_prev->m_next = this;
		m_next->m_prev = this;
		ASSERT( IsValid() );
	}

	void InitLinks()
	{
		m_next = this;
		m_prev = this;
		ASSERT( IsValid() );
		ASSERT( IsEmpty() );
	}

	LinkBase * m_next;
	LinkBase * m_prev;

}; // end LinkBase class


//{=======================================================================

template <typename t_element> 
class Link : public LinkBase
{
private:
	typedef LinkBase			parent_type;
	typedef Link<t_element>		this_type;
public:
	typedef t_element			value_type;

	/////////////////////////////////////
	 Link()
	{
	}

	explicit Link(const value_type & elem) : m_data(elem)
	{
	}

	~Link()
	{
	}
		
	//! data element manipulation
	void SetData(const value_type & elem)
	{
		m_data = elem;
	}
	const value_type & GetData() const
	{
		return m_data; 
	}
	value_type & MutableData()
	{
		return m_data; 
	}

	///////////////////////////////

	const this_type * GetNext() const	{ ASSERT( IsValid() ); return (const this_type *) m_next; }
		  this_type * GetNext()			{ ASSERT( IsValid() ); return (this_type *) m_next; }
	const this_type * GetPrev() const	{ ASSERT( IsValid() ); return (const this_type *) m_prev; }
		  this_type * GetPrev()			{ ASSERT( IsValid() ); return (this_type *) m_prev; }

	///////////////////////////////
	
	/*
	//! add pNode to the list, after "this"
	//!	pNode should not be any lists at this type
	void AddAfter( this_type * pNode );

	//! add pNode to the list, before "this"
	//!	pNode should not be any lists at this type
	void AddBefore( this_type * pNode );

	//! take pList, another list, and add it into the current list in
	//!	the spot right after 'this'
	//! notez : if pList is already in this list, that's bad news !!
	void AddListAfter(  this_type * pList );
	void AddListBefore( this_type * pList );
	*/
	
	///////////////////////////////

	//bool ListContains(const this_type * pNode) const;

	//bool IsValid() const;

	//! returns NULL if none found
	//!	does not look in the current entry
	this_type * FindNext(const value_type & elem) const;
	this_type * FindPrev(const value_type & elem) const;

	/////////////////////////////////////

private:

	// must allow them for evil users of vector<Link> (it's the freaking Galaxy gMeshConnected)
	//FORBID_COPY_CTOR(Link);
	//FORBID_ASSIGNMENT(Link);

	//this_type * m_next;
	//this_type * m_prev;
	value_type	m_data;

}; // end Link template class

//}{=======================================================================

inline void LinkBase::Cut()
{
	ASSERT(IsValid());
	m_prev->m_next = m_next;
	m_next->m_prev = m_prev;
	InitLinks();
}
	
inline void LinkBase::AddAfter( LinkBase * pNode )
{
	ASSERT( pNode );
	ASSERT( pNode != this && pNode != this->m_next );
	ASSERT( IsValid() );
	ASSERT( pNode->IsValid() );
	ASSERT( pNode->IsEmpty() );

	pNode->m_prev = this;
	pNode->m_next = this->m_next;
	pNode->Fix();
	
	ASSERT( IsValid() );
	ASSERT( pNode->IsValid() );
}

inline void LinkBase::AddBefore( LinkBase * pNode )
{
	ASSERT( pNode );
	ASSERT( pNode != this && pNode != this->m_prev );
	ASSERT( IsValid() );
	ASSERT( pNode->IsValid() );
	ASSERT( pNode->IsEmpty() );

	pNode->m_next = this;
	pNode->m_prev = this->m_prev;
	pNode->Fix();
	
	ASSERT( IsValid() );
	ASSERT( pNode->IsValid() );
}

/*! take pList, another list, and add it into the current list in
	the spot right after 'this'
 notez : if pList is already in this list, that's bad news !!
*/
inline void LinkBase::AddListAfter( LinkBase * pList )
{
	ASSERT( pList );
	ASSERT( pList != this && pList != this->m_next && pList != this->m_prev );
	ASSERT( IsValid() );
	ASSERT( pList->IsValid() );
	ASSERT( ! ListContains(pList) );

	// this -> pList -> (this->m_next)
	LinkBase * pListEnd = pList->GetPrev();

	   pList->m_prev = this;
	pListEnd->m_next = this->m_next;

	   pList->m_prev->m_next = pList;
	pListEnd->m_next->m_prev = pListEnd;
	
	ASSERT( IsValid() );
	ASSERT( pList->IsValid() );
}

inline void LinkBase::AddListBefore( LinkBase * pList )
{
	ASSERT( pList );
	ASSERT( pList != this && pList != this->m_next && pList != this->m_prev );
	ASSERT( IsValid() );
	ASSERT( pList->IsValid() );
	ASSERT( ! ListContains(pList) );

	// this -> pList -> (this->m_next)
	LinkBase * pListEnd = pList->GetPrev();

	   pList->m_prev = this->m_prev;
	pListEnd->m_next = this;

	   pList->m_prev->m_next = pList;
	pListEnd->m_next->m_prev = pListEnd;
	
	ASSERT( IsValid() );
	ASSERT( pList->IsValid() );
}


inline int LinkBase::GetListLength() const
{
	ASSERT( IsValid() );

	int iLength = 0;
	for(const LinkBase *pNode = GetNext(); pNode != this; pNode = pNode->GetNext() )
	{
		ASSERT( pNode->IsValid() );
		iLength ++;
		ASSERT( iLength <= 99999 );
	}
	return iLength;
}

inline bool LinkBase::ListContains(const LinkBase * pQuery) const
{
	if ( pQuery == this )
		return true;

	for(const LinkBase *pNode = GetNext(); pNode != this; pNode = pNode->GetNext() )
	{
		ASSERT( pNode->IsValid() );
		if ( pNode == pQuery )
			return true;
	}
	return false;
}

inline bool LinkBase::IsValid() const
{
	ASSERT( m_next != NULL );
	ASSERT( m_prev != NULL );
	ASSERT( m_next->m_prev == this );
	ASSERT( m_prev->m_next == this );
	return true;
}

inline bool LinkBase::ListIsValid() const
{
	// I know GetListLength does an IsValid() on all members of the list
	const int l = GetListLength();
	UNUSED_PARAMETER(l);
	return true;
}

//}=======================================================================

#define T_PRE1	template <class t_element>
#define T_THIS_TYPE	Link<t_element>
#define T_PRE2	T_THIS_TYPE

T_PRE1 T_THIS_TYPE * T_PRE2::FindNext(const value_type & elem) const
{
	for(const this_type * pNode = GetNext(); pNode != this; pNode = pNode->GetNext() )
	{
		if ( pNode->GetData() == elem )
			return const_cast<this_type *>(pNode);
	}
	return NULL;
}
T_PRE1 T_THIS_TYPE * T_PRE2::FindPrev(const value_type & elem) const
{
	for(const this_type * pNode = GetPrev(); pNode != this; pNode = pNode->GetPrev() )
	{
		if ( pNode->GetData() == elem )
			return const_cast<this_type *>(pNode);
	}
	return NULL;
}

#undef T_PRE1
#undef T_THIS_TYPE
#undef T_PRE2

//}=======================================================================

END_CB
