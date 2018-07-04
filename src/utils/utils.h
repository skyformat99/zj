#ifndef _UTILS_H
#define _UTILS_H

#include "env.h"

#ifdef _OS_LINUX
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#endif

//log management
//FreeBSD: MUST define it before all 'stdio.h'
#define _WITH_DPRINTF
#include <stdio.h>

//memory alloc
#include <stdlib.h>
#ifdef _OS_FREEBSD
#include <malloc_np.h>
#endif

#define __mustuse __attribute__ ((__warn_unused_result__));
#define __pub __attribute__ ((visibility("default")))  // default to private(only available in *.so), when compile with `-fvisibility=hidden`

#define __unuse __attribute__ ((__unused__))
#define __ret_nonnull __attribute__ ((__returns_nonnull__))
#define __prm_nonnull __attribute__ ((__nonnull__))

#define __init __attribute__ ((constructor(1000)))
#define __init_with_pri(idx) __attribute__ ((constructor(idx)))
#define __final __attribute__ ((destructor(1000)))
#define __final_with_pri(idx) __attribute__ ((destructor(idx)))

#define __drop(cb) __attribute__ ((cleanup(cb))) // release object when it is out of scope

#define _si short int
#define _usi unsigned short int

#define _i int
#define _ui unsigned int

#define _li unsigned long int
#define _uli unsigned long int

#define _lli unsigned long long int
#define _ulli unsigned long long int

//Public Interface
struct Utils utils;

#define nil NULL
typedef struct Error Error;

#include <sys/types.h>
typedef struct Source {
    void *data;
    size_t dsiz;
    void (*drop)(struct Source *);
}Source;

struct Utils{
    void (*ncpu) (_i *);

    void (*info) (const char *, const char * const, const _i, const char *const);
    void (*fatal) (const char *, const char * const, const _i, const char *const);
    void (*display_errchain) (Error *, const char * const, const _i, const char *const);

    void (*msleep) (_i);
    _ui (*urand) (void);
    void (*nng_drop) (Source *);
    void (*sys_drop) (Source *);
    void (*non_drop) (Source *);

    void* (*alloc) (size_t, const char * const, const _i, const char *const) __mustuse;
    void * (*ralloc)(void *, size_t, const char * const, const _i, const char *const) __mustuse;
};

//For Error Print
struct Error{
    int code;
    const char *desc;
    struct Error *cause;

    const char *file;
    int line;
    const char *func;
};

#define __err_new(__code/*int*/, __desc/*str*/, __prev/*Error_ptr*/) ({\
    Error *new = __alloc(sizeof(Error));\
    new->code = (__code);\
    new->desc = (__desc);\
    new->cause = (__prev);\
    new->file = __FILE__;\
    new->line = __LINE__;\
    new->func = __func__;\
    new;\
});

#define __display_errchain(__e) do{\
    utils.display_errchain(__e, __FILE__, __LINE__, __func__);\
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

#define __display_and_fatal(__e) do{\
    __display_and_clean(__e);\
    exit(1);\
}while(0)

#define __check_fatal(__e, __expr) do{\
    if(nil != ((__e) = (__expr))){\
        __display_and_fatal(__e);\
    }\
}while(0)

#define __fatal(__msg) do{\
    utils.fatal((__msg), __FILE__, __LINE__, __func__);\
}while(0)

#define __info(__msg) do{\
    utils.info((__msg), __FILE__, __LINE__, __func__);\
}while(0)

// For Unit Tests
#define So(__v1, __v2) do{\
    if((__v1) == (__v2)){\
        printf("\x1b[32;01mPass < %s == %s >\x1b[00m\tat: %s(), %s, %d\n",\
                #__v1, #__v2, __func__, __FILE__, __LINE__);\
    } else {\
        printf("\x1b[33;01mFailed < %s == %s >: %lld != %lld\x1b[00m\n"\
                "\tfunc: %s\n"\
                "\tfile: %s\n"\
                "\tline: %d\n",\
                #__v1, #__v2, (_lli)(__v1), (_lli)(__v2),\
                __func__, __FILE__, __LINE__);\
        exit(1);\
    }\
}while(0)

#define SoN(__v1, __v2) do{\
    if((__v1) == (__v2)){\
        printf("\x1b[33;01mFailed < %s != %s >: %lld != %lld\x1b[00m\n"\
                "\tfunc: %s\n"\
                "\tfile: %s\n"\
                "\tline: %d\n",\
                #__v1, #__v2, (_lli)(__v1), (_lli)(__v2),\
                __func__, __FILE__, __LINE__);\
        exit(1);\
    } else {\
        printf("\x1b[32;01mPass < %s != %s >\x1b[00m\tat: %s(), %s, %d\n",\
                #__v1, #__v2, __func__, __FILE__, __LINE__);\
    }\
}while(0)

#define SoGt(__v1, __v2) do{\
    if((__v1) > (__v2)){\
        printf("\x1b[32;01mPass < %s -gt %s >\x1b[00m\tat: %s(), %s, %d\n",\
                #__v1, #__v2, __func__, __FILE__, __LINE__);\
    } else {\
        printf("\x1b[33;01mFailed < %s -gt %s >: %lld -le %lld\x1b[00m\n"\
                "\tfunc: %s\n"\
                "\tfile: %s\n"\
                "\tline: %d\n",\
                #__v1, #__v2, (_lli)(__v1), (_lli)(__v2),\
                __func__, __FILE__, __LINE__);\
        exit(1);\
    }\
}while(0)

#define SoLt(__v1, __v2) do{\
    if((__v1) < (__v2)){\
        printf("\x1b[32;01mPass < %s -lt %s >\x1b[00m\tat: %s(), %s, %d\n",\
                #__v1, #__v2, __func__, __FILE__, __LINE__);\
    } else {\
        printf("\x1b[33;01mFailed < %s -lt %s >: %lld -ge %lld\x1b[00m\n"\
                "\tfunc: %s\n"\
                "\tfile: %s\n"\
                "\tline: %d\n",\
                #__v1, #__v2, (_lli)(__v1), (_lli)(__v2),\
                __func__, __FILE__, __LINE__);\
        exit(1);\
    }\
}while(0)

/**
 * Memory Management
 */
#define __alloc(__siz) ({\
    utils.alloc(__siz, __FILE__, __LINE__, __func__);\
})

#define __ralloc(__orig, __newsiz) ({\
    utils.ralloc(__orig, __newsiz, __FILE__, __LINE__, __func__);\
})

/**
 * Bit Management
 */
// Set bit meaning set a bit to 1; Index from 1
#define __set_bit(__obj, __idx) do {\
    (__obj) |= ((((__obj) >> ((__idx) - 1)) | 1) << ((__idx) - 1));\
} while(0)

// Unset bit meaning set a bit to 0; Index from 1
#define __unset_bit(__obj, __idx) do {\
    (__obj) &= ~(((~(__obj) >> ((__idx) - 1)) | 1) << ((__idx) - 1));\
} while(0)

// Check bit meaning check if a bit is 1; Index from 1
#define __check_bit(__obj, __idx) ((__obj) ^ ((__obj) & ~(((~(__obj) >> ((__idx) - 1)) | 1) << ((__idx) - 1))))

/**
 * Signal Management
 * Ignore most signals, except: SIGKILL、SIGSTOP、SIGSEGV、SIGCHLD、SIGCLD、SIGUSR1、SIGUSR2
 */
#define __ignore_all_signal(){\
    struct sigaction sa;\
    sa.sa_handler = SIG_IGN;\
    sigfillset(&sa.sa_mask);\
    sa.sa_flags = 0;\
\
    sigaction(SIGFPE, &sa, nil);\
    sigaction(SIGINT, &sa, nil);\
    sigaction(SIGQUIT, &sa, nil);\
    sigaction(SIGILL, &sa, nil);\
    sigaction(SIGTRAP, &sa, nil);\
    sigaction(SIGABRT, &sa, nil);\
    sigaction(SIGTERM, &sa, nil);\
    sigaction(SIGBUS, &sa, nil);\
    sigaction(SIGHUP, &sa, nil);\
    sigaction(SIGSYS, &sa, nil);\
    sigaction(SIGALRM, &sa, nil);\
    sigaction(SIGTSTP, &sa, nil);\
    sigaction(SIGTTIN, &sa, nil);\
    sigaction(SIGTTOU, &sa, nil);\
    sigaction(SIGURG, &sa, nil);\
    sigaction(SIGXCPU, &sa, nil);\
    sigaction(SIGXFSZ, &sa, nil);\
    sigaction(SIGPROF, &sa, nil);\
    sigaction(SIGCONT, &sa, nil);\
    sigaction(SIGPIPE, &sa, nil);\
}
#endif //_UTILS_H
