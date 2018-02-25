#include "Base.h"
#include "Link.h"

START_CB

template Link<int>;

void test()
{
	Link<int> i;
}

END_CB
