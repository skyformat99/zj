#ifndef _REGEXP_H
#define _REGEXP_H

#include "utils.h"
#include <regex.h>

typedef regex_t RegexHdr;
typedef struct RegexRes RegexRes;
RegexRes regexres;

struct RegexRes{
    _i cnt;       //total num of matched substrings

    char **res;  //matched results
    _ui *len;  // results' strlen
};

struct Regexp{
    Error *(*init) (const char *, regex_t *) __prm_nonnull __mustuse;
    void (*free) (regex_t *) __prm_nonnull;
    void (*resfree) (struct RegexRes *) __prm_nonnull;
    Error *(*match) (const char *, _i, regex_t *, struct RegexRes *) __prm_nonnull __mustuse;
};

struct Regexp regexp;

#endif
