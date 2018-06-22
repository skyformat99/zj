#ifndef _COMMON_H
#define _COMMON_H

#define __mustuse __attribute__ ((__warn_unused_result__));
#define __pub __attribute__ ((visibility("default")))  // default to private(only available in *.so), when compile with `-fvisibility=hidden`

#define __unuse __attribute__ ((__unused__))
#define __ret_nonnull __attribute__((__returns_nonnull__))
#define __prm_nonnull __attribute__((__nonnull__))

#define __init(idx) __attribute__((constructor(idx)))
#define __clean(idx) __attribute__((destructor(idx)))

#define __auto __auto_type


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
	const char *desc;
	struct Error *cause;

	const char *file;
	int line;
	const char *func;
} Error;

#include <stdio.h>

#define __err_new(__code/*int*/, __desc/*str*/, __prev/*Error_ptr*/) ({\
	Error *new = malloc(sizeof(Error));\
	if(nil == new) {\
		perror("Fatal");\
		fprintf(stderr, "└── %s, %d, %s\n", __FILE__, __LINE__, __func__);\
		exit(1);\
	};\
\
	new->code = (__code);\
	new->desc = (__desc);\
	new->cause = (__prev);\
	new->file = __FILE__;\
	new->line = __LINE__;\
	new->func = __func__;\
\
	new;\
});

#define __display_errchain(__e) do{\
	while(nil != __e){\
		if(nil == __e->desc){\
			__e->desc = "";\
		}\
\
		fprintf(stderr,\
				"\x1b[31;01mcause by:\x1b[00m"\
				"\t├── \x1b[31mfile:\x1b[00m %s\n"\
				"\t├── \x1b[31mline:\x1b[00m %d\n"\
				"\t├── \x1b[31mfunc:\x1b[00m %s\n"\
				"\t├── \x1b[31merrcode:\x1b[00m %d\n"\
				"\t└── \x1b[31merrdesc:\x1b[00m %s\n",\
				__e->file,\
				__e->line,\
				__e->func,\
				__e->code,\
				__e->desc);\
\
		__e = __e->cause;\
	};\
}while(0)

#define __clean_errchain(__e) do{\
	__auto err = nil;\
	while(nil != __e){\
		err = __e;\
		__e = __e->cause;\
		free(err);\
	};\
}while(0)

#endif //_COMMON_H
