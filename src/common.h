#ifndef _COMMON_H
#define _COMMON_H

#define weak __attribute__ ((__weak__))  // function that can be overrided(reimpl)
#define mustuse __attribute__ ((__warn_unused_result__));

#define pub __attribute__ ((visibility("default")))  // default to private(only available in *.so), when compile with `-fvisibility=hidden`

#define __ __attribute__ ((__unused__))
#define return_nonnull __attribute__((__returns_nonnull__))
#define params_nonnull __attribute__((__nonnull__))

#define init(idx) __attribute__((constructor(idx)))
#define clean(idx) __attribute__((destructor(idx)))


#define _c char
#define _uc unsigned char

#define _i int
#define _ui unsigned int

#define _li unsigned long int
#define _uli unsigned long int

#define _lli unsigned long long int
#define _ulli unsigned long long int

#define _f float
#define _d double

#define nil NULL

typedef struct Error {
	int code;
	char *desc;
	struct Error *cause;
} Error;

Error *
err_new(int code, char *desc, Error *prev) return_nonnull mustuse;

void
display_errchain(Error *e);

#include <stdio.h>
#include <stdlib.h>
#include "jemalloc.h"

Error *
err_new(int code, char *desc, Error *prev) {
	Error *new = malloc(sizeof(Error));
	if(nil == new) {
		perror("Fatal");
		exit(1);
	}

	new->code = code;
	new->desc = desc;
	new->cause = prev;

	return new;
}

void
display_errchain(Error *e){
	while(nil != e){
		if(nil == e->desc){
			e->desc = "";
		}

		fprintf(stderr,
				"\x1b[31;01mcause by:\x1b[00m"
				"\t├──\x1b[31mfile:\x1b[00m %s\n"
				"\t├──\x1b[31mline:\x1b[00m %d\n"
				"\t├──\x1b[31merrno:\x1b[00m %d\n"
				"\t└──\x1b[31mdetail:\x1b[00m %s\n",
				__FILE__,
				__LINE__,
				e->code,
				e->desc);

		e = e->cause;
	};
}

#endif //_COMMON_H
