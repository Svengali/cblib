#include "Reflection.h"
#include "Prefs.h"

START_CB

#if 1 // just test code

//============================================================

class TestClassSub
{
public:
	TestClassSub()
	{
		m_a = 2;
		m_b = 'x';
	}

	template <class T>
	void Reflection(T & functor)
	{
		REFLECT(m_a);
		REFLECT(m_b);
	}

	/*
	virtual void vPrintMembers()
	{
		Reflection( PrintMembers() );
	}
	*/

private:
	int		m_a;
	char	m_b;
};

SPtrFwd(TestClass);
class TestClass : public Prefs
{
public:
	TestClass() : m_b(7.5f)
	{
	}

	template <class T>
	void Reflection(T &)
	{
		//REFLECT(m_a);
		//REFLECT(m_b);
	}

	virtual void vPrintMembers()
	{
		PrintMembers p;
		Reflection( p );
	}
	virtual int vSizeMembers()
	{
		SizeMembers s;
		Reflection( s );
		return s.m_total;
	}
	virtual void vIO(FileRCPtr & file)
	{
		IOMembers z(file);
		Reflection( z );
	}

private:
	TestClassSub	m_a;
	float			m_b;

	FORBID_CLASS_STANDARDS(TestClass);
};

//============================================================

void Reflection_Test()
{
	TestClass c;

	c.vPrintMembers();

	int x = c.vSizeMembers();

	lprintf("Size : %d (%d) , Waste : %d\n",x,sizeof(c),sizeof(c) - x);

	{
		PrefBlock block(PrefBlock::eWrite,"r:\\reflection_prefs.txt");
		block.IO("prefs",&c);
	}

	TestClassPtr prefs = PrefsMgr::Get<TestClass>("r:\\reflection_prefs.txt");
	prefs->vPrintMembers();
}

//============================================================
#endif // test code

END_CB
