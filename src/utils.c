#include "utils.h"

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef OS_FREEBSD
#include <malloc_np.h>
#endif

#include "nng/nng.h"
#include "nng/supplemental/util/platform.h"

static _i ncpus(void);

static void print_time(void);
static void info(const char *msg, const char * const file, const _i line, const char *const func);
static void fatal(const char *msg, const char * const file, const _i line, const char *const func);
static void display_errchain(error_t *e, const char * const file, const _i line, const char *const func);

static void msleep(_i ms);
static _ui urand(void);
static void nng_drop(source_t *s);
static void sys_drop(source_t *s);
static void non_drop(source_t *s);
static void *must_alloc(size_t siz, const char * const file, const _i line, const char *const func);

static pthread_mutex_t loglock = PTHREAD_MUTEX_INITIALIZER;

struct utils utils = {
    .ncpus = ncpus,
    .info = info,
    .fatal = fatal,
    .display_errchain = display_errchain,

    .msleep = msleep,
    .urand = urand,
    .nng_drop = nng_drop,
    .sys_drop = sys_drop,
    .non_drop = non_drop,
    .alloc = must_alloc,
};

static _i
ncpus(void){
    return sysconf(_SC_NPROCESSORS_ONLN);
}

static void
print_time(void){
    time_t ts = time(NULL);
    struct tm *now = localtime(&ts);
    printf("\n[ %d-%d-%d %d:%d:%d ]\n",
            now->tm_year + 1900,
            now->tm_mon + 1,  /* Month (0-11) */\
            now->tm_mday,
            now->tm_hour,
            now->tm_min,
            now->tm_sec);
}

static void
info(const char *msg, const char * const file, const _i line, const char *const func){
    pthread_mutex_lock(&loglock);
    print_time();
    printf("\x1b[01mINFO:\x1b[00m %s\n"
            "   ├── file: %s\n"
            "   ├── line: %d\n"
            "   └── func: %s\n",
            msg,
            file,
            line,
            func);
    pthread_mutex_unlock(&loglock);
}

static void
fatal(const char *msg, const char * const file, const _i line, const char *const func){
    pthread_mutex_lock(&loglock);
    print_time();
    printf("\x1b[31;01mFATAL:\x1b[00m %s\n"
            "   ├── file: %s\n"
            "   ├── line: %d\n"
            "   └── func: %s\n",
            msg,
            file,
            line,
            func);
    pthread_mutex_unlock(&loglock);

    exit(1);
}

static void
display_errchain(error_t *e, const char * const file, const _i line, const char *const func){
    time_t ts = time(NULL);
    struct tm *now = localtime(&ts);

    pthread_mutex_lock(&loglock);
    printf("\n[ %d-%d-%d %d:%d:%d ]\n\x1b[31;01m** ERROR **\x1b[00m\n"
            "   ├── file: %s\n"
            "   ├── line: %d\n"
            "   └── func: %s\n",
            now->tm_year + 1900,
            now->tm_mon + 1,  /* Month (0-11) */
            now->tm_mday,
            now->tm_hour,
            now->tm_min,
            now->tm_sec,
            file,
            line,
            func);

    while(nil != e){
        if(nil == e->desc){
            e->desc = "";
        }

        printf("\x1b[01m   caused by: \x1b[00m%s (error code: %d)\n"
                "   ├── file: %s\n"
                "   ├── line: %d\n"
                "   └── func: %s\n",
                e->desc,
                e->code,
                e->file,
                e->line,
                e->func);

        e = e->cause;
    };
    pthread_mutex_unlock(&loglock);
}

static void
msleep(_i ms){
    nng_msleep(ms);
}

static _ui
urand(void){
    return nng_random();
}

static void
nng_drop(source_t *s){
    if(nil != s && nil != s->data){
        nng_free(s->data, s->dsiz);
    }
}

static void
sys_drop(source_t *s){
    if(nil != s && nil != s->data){
         free(s->data);
    }
}

static void
non_drop(source_t *s){
    (void)s;
}

static void *
must_alloc(size_t siz, const char * const file, const _i line, const char *const func){
    void *p = malloc(siz);
    if(nil == p){
        fatal("the fucking world is over!!!!", file, line, func);
    };
    return p;
}
