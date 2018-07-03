#include "regexp.h"
#include <string.h>

static Error *reg_init(const char *pattern, regex_t *reghdr) __prm_nonnull;
static void regresfree(struct RegexRes *final) __prm_nonnull;
static Error * reg_match(const char *origstr, _i n, regex_t *reghdr, struct RegexRes *final) __prm_nonnull;

struct Regexp regexp = {
    .init = reg_init,
    .free = regfree,
    .resfree = regresfree,
    .match = reg_match,
};

//@param pattern[in]:
//@param reghdr[out]:
static Error *
reg_init(const char *pattern, regex_t *reghdr){
    _i rv;
    if (0 != (rv = regcomp(reghdr, pattern, REG_EXTENDED))){
        char errbuf[256];
        regerror(rv, reghdr, errbuf, 256);
        regfree(reghdr);
        return __err_new(rv, errbuf, nil);
    }

    return nil;
}

static void
regresfree(struct RegexRes *final){
    _i i;
    for(i = 0; i < final->cnt; ++i){
        free(final->res[i]);
    }
    free(final->len);
}

//@param reghdr[in]:
//@param n[in]: max results
//@param origstr[in]:
//@param res[out]:
static Error *
reg_match(const char *origstr, _i n, regex_t *reghdr, struct RegexRes *final){
    regmatch_t matched;
    char errbuf[256];
    _i rv, i;
    _i ttlen/*total len*/, len;

    ttlen = strlen(origstr);

    final->cnt = 0;
    final->len = __alloc(n * (sizeof(_ui) + sizeof(char *)));
    final->res = (char **)(final->len + n * sizeof(_i));

    for(i = 0; i < n && 0 < ttlen; ++i){
        if(0 != (rv = regexec(reghdr, origstr, 1, &matched, 0))){
            if (REG_NOMATCH == rv) {
                break;
            } else {
                regerror(rv, reghdr, errbuf, 256);
                regfree(reghdr);
                regresfree(final);
                return __err_new(rv, errbuf, nil);
            }
        }

        len = matched.rm_eo - matched.rm_so;
        if(0 == len){
            break;
        }

        final->len[final->cnt] = len;
        final->cnt++;

        final->res[i] = __alloc(1 + len);
        strncpy(final->res[i], origstr + matched.rm_so, len);
        final->res[i][len] = '\0';

        origstr += matched.rm_eo + 1;
        ttlen -= matched.rm_eo + 1;
    }

    return nil;
}
