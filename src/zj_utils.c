#include "zj_utils.h"
#include <time.h>

#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>

static void
zj_sleep(_f time){
	struct timespec tsp;
	tsp.tv_sec = (_i)time;
	tsp.tv_nsec = 1e9 * (time - (_f)tsp.tv_sec);

	nanosleep(&tsp, nil);
}
static _ui
zj_urand(void){
	return nng_random();
}


struct zj_utils zjutils = {
	.sleep = zj_sleep,
	.urand = zj_urand,
};
