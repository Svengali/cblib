#pragma once

#include "Base.h"

/*! 
	CRC32 class

	very robust data certificate creation
	this is the preferred way to do data integrity checking
	similar to the algorithm in Z-Lib, but with an interface for
	us to use.

	streamable as raw data

	Use like this :

	struct my_data
	{
		int x;
		byte y;
	} my_struct;

	crc my_certificate;
	my_certificate.AddL(my_struct.x);
	my_certificate.AddB(my_struct.y);
	Stream out my_certificate; (or whatever)
*/

START_CB

class CRC
{
public:
	enum ECase
	{
		eSensitive,
		eInsensitive
	};

	CRC()
	{
		Reset();
	}

	explicit CRC( const uint32 crc ) :
		m_crc( crc )
	{
	}

	explicit CRC( const char * const pStr, const ECase caseStr = eInsensitive );

	// default operator = and copy constructor are fine

	void Reset();

	void AddB(const uint8  b);
	void AddW(const uint16 w);
	void AddL(const uint32 l);
	void AddF(const float  f);

	// cast the refs, not the values, so the bits aren't changed :
	void AddL(const int   i) { AddL(reinterpret_cast<const uint32 &>(i) ); }
	void AddW(const short s) { AddW(reinterpret_cast<const uint16 &>(s) ); }
	void AddB(const char  c) { AddB(reinterpret_cast<const uint8  &>(c) ); }

	void AddArray(const uint8 * pBuf,const int buflen);
	void AddStringInsensitive(const char * const pStr); // NOT case sensitive!

	void AddStringSensitive(const char * const pStr);

	// compare via the raw data :
	bool operator == (const CRC & other) const { return (m_crc == other.m_crc); }
	bool operator <  (const CRC & other) const { return (m_crc <  other.m_crc); }
	bool operator <= (const CRC & other) const { return (m_crc <= other.m_crc); }
	bool operator >= (const CRC & other) const { return (m_crc >= other.m_crc); }
	bool operator >  (const CRC & other) const { return (m_crc >  other.m_crc); }
	bool operator != (const CRC & other) const { return (m_crc != other.m_crc); }

	uint32 GetHash() const { return m_crc; }

private:
	uint32 m_crc;
};

extern uint32 crc_table[256];

static inline void stepCRC(uint32 & crc,const int byte)
{
	crc = crc_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
}

END_CB
