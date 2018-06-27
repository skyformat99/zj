#include "nng/nng.h"
#include "zj_common.h"

__clean void
nng_clean(void){
	nng_fini();
}
