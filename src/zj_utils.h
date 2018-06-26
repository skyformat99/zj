#ifndef _ZJ_UTILS_H
#define _ZJ_UTILS_H

#include "zj_common.h"

struct zj_utils{
	void (* sleep) (_f);
	_ui (* urand) (void);
};

struct zj_utils zjutils;

#endif //_ZJ_UTILS_H
