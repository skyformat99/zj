#ifndef _ZJ_COMMON_H
#define _ZJ_COMMON_H

#if (700 != _XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif

#define __mustuse __attribute__ ((__warn_unused_result__));
#define __pub __attribute__ ((visibility("default")))  // default to private(only available in *.so), when compile with `-fvisibility=hidden`

#define __unuse __attribute__ ((__unused__))
#define __ret_nonnull __attribute__ ((__returns_nonnull__))
#define __prm_nonnull __attribute__ ((__nonnull__))

#define __init __attribute__ ((constructor(1000)))
#define __init_with_pri(idx) __attribute__ ((constructor(idx)))
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
    fprintf(stderr, "\n[ %d-%d-%d %d:%d:%d ]\n\x1b[31;01m** ERROR **\x1b[00m\n"\
            "   ├── file: %s\n"\
            "   ├── line: %d\n"\
            "   └── func: %s\n",\
            now->tm_year + 1900,\
            now->tm_mon + 1,  /* Month (0-11) */\
            now->tm_mday,\
            now->tm_hour,\
            now->tm_min,\
            now->tm_sec,\
			__FILE__,\
			__LINE__,\
			__func__);\
\
    Error *err = __e;\
    while(nil != err){\
        if(nil == err->desc){\
            err->desc = "";\
        }\
\
        fprintf(stderr,\
                "\x1b[01m   caused by: \x1b[00m%s (error code: %d)\n"\
                "   ├── file: %s\n"\
                "   ├── line: %d\n"\
                "   └── func: %s\n",\
                err->desc,\
                err->code,\
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

#define __display_and_clean(__e) do{\
	__display_errchain(__e);\
	__clean_errchain(__e);\
}while(0)

#define __info(__msg) do{\
    fprintf(stderr, "[%s, %d, %s]: %s\n", __FILE__, __LINE__, __func__, __msg);\
}while(0)

#define __malloc(__siz) ({\
    void *ptr = malloc(__siz);\
    if(nil == ptr){\
        __info("the fucking world is over!!!!");\
        exit(1);\
    };\
    ptr;\
})

#endif //_ZJ_COMMON_H
