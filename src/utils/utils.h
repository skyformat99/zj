#ifndef UTILS_H
#define UTILS_H

#include "os_target.h"

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
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

#define _f float
#define _d double

//Public Interface
struct utils utils;

#define nil NULL
typedef struct error_t error_t;

#include <sys/types.h>
typedef struct source_t{
    void *data;
    size_t dsiz;
    void (*drop)(struct source_t *);
}source_t;

struct utils{
    _i (*ncpus) (void);
    void (*info) (const char *msg, const char * const file, const _i line, const char *const func);
    void (*fatal) (const char *msg, const char * const file, const _i line, const char *const func); 
    void (*display_errchain) (error_t *, const char * const, const _i, const char *const);

    void (*msleep) (_i);
    _ui (*urand) (void);
    void (*nng_drop) (source_t *) __prm_nonnull;
    void (*sys_drop) (source_t *) __prm_nonnull;
    void (*non_drop) (source_t *) __prm_nonnull;
    void* (*alloc) (size_t siz, const char * const file, const _i line, const char *const func);
};

//For Error Print
struct error_t{
    int code;
    const char *desc;
    struct error_t *cause;

    const char *file;
    int line;
    const char *func;
};

#define __err_new(__code/*int*/, __desc/*str*/, __prev/*error_t_ptr*/) ({\
    error_t *new = __malloc(sizeof(error_t));\
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
    error_t *err = nil;\
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

#define __fatal(__msg) do{\
    utils.fatal(__msg, __FILE__, __LINE__, __func__);\
}while(0)

#define __info(__msg) do{\
    utils.info(__msg, __FILE__, __LINE__, __func__);\
}while(0)

// For Unit Tests
#define So(__v1, __v2) do{\
    if((__v1) == (__v2)){\
        printf("\x1b[32;01mPass < %s == %s >\x1b[00m\tFile: %s\tLine: %d\n",\
                #__v1, #__v2, __FILE__, __LINE__);\
    } else {\
        printf("\x1b[33;01mFailed < %s == %s >: %lld != %lld\x1b[00m\n"\
                "\tFile: %s\n"\
                "\tLine: %d\n",\
                #__v1, #__v2, (_lli)(__v1), (_lli)(__v2), __FILE__, __LINE__);\
        exit(1);\
    }\
}while(0)

#define SoN(__v1, __v2) do{\
    if((__v1) == (__v2)){\
        printf("\x1b[33;01mFailed < %s != %s >: %lld != %lld\x1b[00m\n"\
                "\tFile: %s\n"\
                "\tLine: %d\n",\
                #__v1, #__v2, (_lli)(__v1), (_lli)(__v2), __FILE__, __LINE__);\
        exit(1);\
    } else {\
        printf("\x1b[32;01mPass < %s != %s >\x1b[00m\tFile: %s\tLine: %d\n",\
                #__v1, #__v2, __FILE__, __LINE__);\
    }\
}while(0)

/**
 * Memory Management
 */
#define __malloc(__siz) ({\
    utils.alloc(__siz, __FILE__, __LINE__, __func__);\
})

/**
 * Bit Management
 */
// Set bit meaning set a bit to 1; Index from 1
#define __set_bit(__obj, __idx) do {\
    (__obj) |= ((((__obj) >> (__idx)) | 1) << (__idx));\
} while(0)

// Unset bit meaning set a bit to 0; Index from 1
#define __unset_bit(__obj, __idx) do {\
    (__obj) &= ~(((~(__obj) >> (__idx)) | 1) << (__idx));\
} while(0)

// Check bit meaning check if a bit is 1; Index from 1
#define __check_bit(__obj, __idx) ((__obj) ^ ((__obj) & ~(((~(__obj) >> (__idx)) | 1) << (__idx))))

/**
 * Signal Management
 * Ignore all except: SIGKILL、SIGSTOP、SIGSEGV、SIGCHLD、SIGCLD、SIGUSR1、SIGUSR2
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
    sigaction(SIGWINCH, &sa, nil);\
    sigaction(SIGCONT, &sa, nil);\
    sigaction(SIGPIPE, &sa, nil);\
    sigaction(SIGIOT, &sa, nil);\
    sigaction(SIGIO, &sa, nil);\
}
#endif //UTILS_H
