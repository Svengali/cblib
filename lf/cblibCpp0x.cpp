#include "cblibCpp0x.h"

using namespace CBLIB_LF_NS;

void cblibCpp0x_test()
{
	atomic<__int64> a;
	a.fetch_add(1,mo_acq_rel);
	a.fetch_or(1,mo_acq_rel);
	__int64 old = 3;
	a.compare_exchange_strong(old,7,mo_acq_rel,mo_acquire);
	__int64 z = a.load(mo_relaxed);
	a.store(z,mo_seq_cst);
	a.exchange(7,mo_acq_rel);
}