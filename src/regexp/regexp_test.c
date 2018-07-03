#include "regexp.c"
#include "utils.c"

#define s1 "abc_123_de 45678 f"

void
match_correct(void){
    RegexHdr hdr;
    RegexRes res;
    Error *e;

    __check_fatal(e, regexp.init("[0-9]+", &hdr));
    __check_fatal(e, regexp.match(s1, 8, &hdr, &res));

    So(2, res.cnt);

    So(3, strlen(res.res[0]));
    So(0, strcmp("123", res.res[0]));

    So(5, strlen(res.res[1]));
    So(0, strcmp("45678", res.res[1]));

    regexp.free(&hdr);
    regexp.resfree(&res);
}

_i
main(void){
    match_correct();
}
