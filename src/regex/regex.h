#include <regex.h>

typedef struct __zRegRes__ {
    char **pp_rets;  //matched results
    _i *p_resLen;  // results' strlen
    _i cnt;         //total num of matched substrings

    void * (* alloc_fn) (size_t);
} zRegRes__ ;

typedef regex_t zRegInit__;

TODO
