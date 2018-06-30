#include "http_cli.c"

#include "utils.c"
#include <string.h>

#define __env__ {\
    urlstr = "http://www.baidu.com";\
    s.data = "POST";\
    s.dsiz = sizeof("POST") - 1;\
    s.drop = nil;\
\
    status_code = -1;\
    e = nil;\
};

_i
main(void){
    error_t *e;
    const char *urlstr;
    source_t s;
    _i status_code;

    __env__
    e = httpcli.get(urlstr, &s, &status_code);
    if(nil != e){
        __display_and_fatal(e);
    }
    s.drop(&s);
    SoN(200, status_code);

    __env__
    e = httpcli.post(urlstr, &s, &status_code);
    if(nil != e){
        __display_and_fatal(e);
    }
    s.drop(&s);
    SoN(200, status_code);
}
