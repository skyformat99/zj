#ifndef _REGEX_H
#define _REGEX_H

#include "utils.h"

struct regex_res_t {
    _i cnt;       //total num of matched substrings

    char **res;  //matched results
    _i *reslen;  // results' strlen
};

#endif
