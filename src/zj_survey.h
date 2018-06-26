#ifndef _ZJ_SURVEY_H
#define _ZJ_SURVEY_H

#include "nng/nng.h"
#include "nng/protocol/survey0/survey.h"
#include "nng/protocol/survey0/respond.h"

#include "zj_common.h"

struct zj_nng{
	Error * (*leader_new) (const char *, nng_socket *) __prm_nonnull __mustuse;
	Error * (*voter_new) (const char *, nng_socket *) __prm_nonnull __mustuse;
	Error * (*send) (nng_socket, void *, size_t) __prm_nonnull __mustuse;
	Error * (*recv) (nng_socket, void **, size_t *) __mustuse;
};

struct zj_nng zjnng;

#endif //_ZJ_SURVEY_H
