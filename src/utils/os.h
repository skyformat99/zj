#ifndef _OS_H
#define _OS_H

#include "utils.h"

struct os{
    void (*daemonize) (const char *);
    error_t *(* rm_all) (char *);
};

struct os os;

#endif //_OS_H
