#include "utils.h"

#include <fcntl.h>
#include <pthread.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef _OS_FREEBSD
#include <malloc_np.h>
#endif

#include "nng/nng.h"
#include "nng/supplemental/util/platform.h"

inline static _i ncpus(void);

static void print_time(void);
static void info(const char *msg, const char *const file, const _i line, const char *const func);
static void fatal(const char *msg, const char *const file, const _i line, const char *const func);
static void display_errchain(error_t *e, const char *const file, const _i line, const char *const func);

inline static void msleep(_i ms);
inline static _ui urand(void);
static void nng_drop(source_t *restrict s);
static void sys_drop(source_t *restrict s);
static void non_drop(source_t *restrict s);

static void *must_alloc(size_t siz, const char *const file, const _i line, const char *const func);
static void *must_ralloc(void *orig, size_t newsiz, const char *const file, const _i line, const char *const func);

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
    .ralloc = must_ralloc,
};

inline static _i
ncpus(void){
#ifdef _OS_LINUX
    return sysconf(_SC_NPROCESSORS_ONLN);
#else
    return 8;
#endif
}

inline static void
msleep(_i ms){
    nng_msleep(ms);
}

inline static _ui
urand(void){
    return nng_random();
}

static void
nng_drop(source_t *restrict s){
    if(nil != s && nil != s->data){
        nng_free(s->data, s->dsiz);
    }
}

static void
sys_drop(source_t *restrict s){
    if(nil != s && nil != s->data){
         free(s->data);
    }
}

static void
non_drop(source_t *restrict s){
    (void)s;
}

static void *
must_alloc(size_t siz,
        const char *const file, const _i line, const char *const func){
    void *p = malloc(siz);
    if(nil == p){
        fatal("the fucking world is over!!!!", file, line, func);
    };
    return p;
}

static void *
must_ralloc(void *orig, size_t newsiz,
        const char *const file, const _i line, const char *const func){
    void *p = realloc(orig, newsiz);
    if(nil == p){
        fatal("the fucking world is over!!!!", file, line, func);
    };
    return p;
}

/**
 * LOG
 */
static pthread_mutex_t loglock = PTHREAD_MUTEX_INITIALIZER;
static const char *logvec[10] = {
    "/tmp/zj@log-0",
    "/tmp/zj@log-1",
    "/tmp/zj@log-2",
    "/tmp/zj@log-3",
    "/tmp/zj@log-4",
    "/tmp/zj@log-5",
    "/tmp/zj@log-6",
    "/tmp/zj@log-7",
    "/tmp/zj@log-8",
    "/tmp/zj@log-9",
};

static _i logfd = -1;
static _i log_wr_cnt = 0;

__init static void
logfd_init(void){
#ifdef _RELEASE
    logfd = open(logvec[0], O_WRONLY|O_CREAT|O_APPEND, 0600);
    if(-1 == logfd){
        __fatal("can NOT open logfile");
    }
#else
    logfd = STDOUT_FILENO;
#endif
}

//rotate per 1000 * 10000 logs
void
logrotate(void){
    if(1000 * 10000 < log_wr_cnt){
        close(logfd);
        unlink(logvec[9]);
        _i i = 8;
        for(; i >= 0; --i){
            rename(logvec[i], logvec[i + 1]);
        }

        logfd_init();
        log_wr_cnt = 0;
    }
    ++log_wr_cnt;
}

static void
print_time(void){
    time_t ts = time(nil);
    struct tm *now = localtime(&ts);
    dprintf(logfd, "\n[ %d-%d-%d %d:%d:%d ]\n",
            now->tm_year + 1900,
            now->tm_mon + 1,  /* Month (0-11) */\
            now->tm_mday,
            now->tm_hour,
            now->tm_min,
            now->tm_sec);
}

static void
info(const char *msg, const char *const file, const _i line, const char *const func){
    pthread_mutex_lock(&loglock); logrotate();
    print_time();
    dprintf(logfd, "\x1b[01mINFO:\x1b[00m %s\n"
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
fatal(const char *msg, const char *const file, const _i line, const char *const func){
    pthread_mutex_lock(&loglock); logrotate();
    print_time();
    dprintf(logfd, "\x1b[31;01mFATAL:\x1b[00m %s\n"
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
display_errchain(error_t *e, const char *const file, const _i line, const char *const func){
    time_t ts = time(nil);
    struct tm *now = localtime(&ts);

    pthread_mutex_lock(&loglock); logrotate();
    dprintf(logfd, "\n[ %d-%d-%d %d:%d:%d ]\n\x1b[31;01m** ERROR **\x1b[00m\n"
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

        dprintf(logfd, "\x1b[01m   caused by: \x1b[00m%s (error code: %d)\n"
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

/**
 * exit clean
 */
__final void
sys_clean(){
    close(logfd);
}
