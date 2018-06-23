#ifndef _COMMON_H
#define _COMMON_H

#if (700 != _XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif

#define __mustuse __attribute__ ((__warn_unused_result__));
#define __pub __attribute__ ((visibility("default")))  // default to private(only available in *.so), when compile with `-fvisibility=hidden`

#define __unuse __attribute__ ((__unused__))
#define __ret_nonnull __attribute__ ((__returns_nonnull__))
#define __prm_nonnull __attribute__ ((__nonnull__))

#define __init(idx) __attribute__ ((constructor(idx)))
#define __clean(idx) __attribute__ ((destructor(idx)))

#define __drop(cb) __attribute__ ((cleanup(cb))) // release object when it is out of scope

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
    time_t ts = time(NULL);\
    struct tm *now = localtime(&ts);\
    fprintf(stderr, "\n[ %d-%d-%d %d:%d:%d ]\n\x1b[31;01m** ERROR **\x1b[00m\n",\
            now->tm_year + 1900,\
            now->tm_mon + 1,  /* Month (0-11) */\
            now->tm_mday,\
            now->tm_hour,\
            now->tm_min,\
            now->tm_sec);\
\
    Error *err = __e;\
    while(nil != err){\
        if(nil == err->desc){\
            err->desc = "";\
        }\
\
        fprintf(stderr,\
                "\x1b[01m===> (%d) \x1b[00m%s\n"\
                "   ├── file: %s\n"\
                "   ├── line: %d\n"\
                "   └── func: %s\n",\
				err->code,\
                err->desc,\
                err->file,\
                err->line,\
                err->func);\
\
        err = err->cause;\
    };\
}while(0)

#define __clean_errchain(__e) do{\
    Error *err = nil;\
    while(nil != __e){\
        err = __e;\
        __e = __e->cause;\
        free(err);\
    };\
}while(0)

#define __info(__msg) do{\
    fprintf(stderr, "[%s, %d, %s]: %s\n", __FILE__, __LINE__, __func__, __msg);\
}while(0)

#endif //_COMMON_H
