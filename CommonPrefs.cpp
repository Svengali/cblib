#include "CommonPrefs.h"
// some standard PrefIO's :
#include "Vec2.h"
#include "Vec3.h"
#include "AxialBox.h"
#include "Sphere.h"
#include "Color.h"
#include "Rect.h"

START_CB

void PrefIO(PrefBlock & block,Vec3 & what)
{
	block.IO("x",&what.x);
	block.IO("y",&what.y);
	block.IO("z",&what.z);
}

void PrefIO(PrefBlock & block,Vec2 & what)
{
	block.IO("x",&what.x);
	block.IO("y",&what.y);
}

void PrefIO(PrefBlock & block,AxialBox & what)
{
	block.IO("min",&what.MutableMin());
	block.IO("max",&what.MutableMax());
}

void PrefIO(PrefBlock & block,Sphere & what)
{
	block.IO("center",&what.MutableCenter());
	block.IO("radius",&what.MutableRadius());
}

void PrefIO(PrefBlock & block,ColorDW & what)
{
	int r = what.GetR();
	int g = what.GetG();
	int b = what.GetB();
	int a = what.GetA();
	block.IO("r",&r);
	block.IO("g",&g);
	block.IO("b",&b);
	block.IO("a",&a);
	what.Set(r,g,b,a);
}

void PrefIO(PrefBlock & block,ColorF & me)
{
	block.IO("r",&me.m_r);
	block.IO("g",&me.m_g);
	block.IO("b",&me.m_b);
	block.IO("a",&me.m_a);
}


void PrefIO(PrefBlock & block,RectI & me)
{
	block.IO("xlo",&me.m_xLo);
	block.IO("xhi",&me.m_xHi);
	block.IO("ylo",&me.m_yLo);
	block.IO("yhi",&me.m_yHi);
}

void PrefIO(PrefBlock & block,RectF & me)
{
	block.IO("xlo",&me.m_xLo);
	block.IO("xhi",&me.m_xHi);
	block.IO("ylo",&me.m_yLo);
	block.IO("yhi",&me.m_yHi);
}

void PrefIO(PrefBlock & block,POINT & me)
{
	block.IO("x",&me.x);
	block.IO("y",&me.y);
}

void PrefIO(PrefBlock & block,RECT & me)
{
	block.IO("l",&me.left);
	block.IO("t",&me.top);
	block.IO("r",&me.right);
	block.IO("b",&me.bottom);
}

void PrefIO(PrefBlock & block,COLORREF & me)
{
	int r = GetRValue(me);
	int g = GetGValue(me);
	int b = GetBValue(me);
	int a = (me>>24);
	block.IO("r",&r);
	block.IO("g",&g);
	block.IO("b",&b);
	block.IO("a",&a);
	me = RGB(r,g,b) | (a<<24);
}

void PrefIO(PrefBlock & block,RGBQUAD & me)
{
	block.IO("r",&me.rgbRed);
	block.IO("g",&me.rgbGreen);
	block.IO("b",&me.rgbBlue);
	block.IO("a",&me.rgbReserved);
}

END_CB



//===============================================================================

//=======================================================================
// Test :
#if 0 //{

#include "AxialBox.h"

USE_CB

struct MyStruct
{
	String str;
	int i;
	vector<int> vec;
	vector<String> vecs;
};

void PrefIO(PrefBlock & block,MyStruct & me)
{
	block.IO("hello",&me.str);

	block.IO("int",&me.i);

	block.IOV("vectest",&me.vec);
	
	block.IOV("vstest",&me.vecs);
}


struct MyStruct2
{
	String str;
	MyStruct sub;
	AxialBox box;
};

void PrefIO(PrefBlock & block,MyStruct2 & me)
{
	block.IO("poo",&me.str);

	block.IO("pee",&me.sub);
	
	block.IO("poo2",&me.box);
}

#include "Prefs.h"

SPtrFwd(MyPref);
class MyPref : public Prefs
{
public:
	MyPref() { }
	MyStruct2	member;	
private:
	FORBID_CLASS_STANDARDS(MyPref);
};

void PrefIO(PrefBlock & block,MyPref & me)
{
	PrefIO(block,me.member);
}

void PrefBlock_Reader_test()
{
	const char * data = "hello:world\nint:7\n"
						"vectest:3,7,11,17\n"
						"vstest:4,a,bb,ccc,dddd\n";

	PrefBlock_Reader reader;
	reader.Read(data);

	MyStruct me;
	PrefBlock b1(&reader);
	PrefIO( b1, me );

	PrefBlock_Writer writer;

	PrefBlock b2(&writer);
	PrefIO( b2, me );

	//const String & out = writer.GetBuffer();

	MyStruct2 me2;
	me2.str = "test";
	me2.box = AxialBox::unitBox;
	me2.sub = me;

	writer.Reset();
	b2.IO("me2",&me2);
	//PrefIO(b2,me2);
	
	reader.Read( writer.GetBuffer().CStr() );

	MyStruct2 me2out;
	//PrefIO(b1,me2out);
	b1.IO("me2",&me2out);

	PrefBlock bout(PrefBlock::eWrite,"r:\\zz");
	bout.IO("prefs",&me2);
	bout.FinishWriting();


	PrefBlock bin(PrefBlock::eRead,"r:\\zz");
	MyStruct2 me2in;
	bin.IO("prefs",&me2in);

	MyPrefPtr pref = PrefsMgr::Get<MyPref>("r:\\zz");

	PrefsMgr::SaveAll();
	PrefsMgr::ReloadAll();
}

#endif //} // TEST CODE
//===============================================================================

