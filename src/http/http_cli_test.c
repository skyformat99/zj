#include "http_cli.c"

#include "utils.c"
#include <string.h>

Error *e;
const char *urlstr;
Source s;
_i status;

#define __env__ {\
    urlstr = "http://www.baidu.com";\
    s.data = "POST";\
    s.dsiz = sizeof("POST") - 1;\
    s.drop = nil;\
\
    status = -1;\
    e = nil;\
};

void
get(void){
    __env__
    e = httpcli.get(urlstr, &s, &status);
    if(nil != e){
        __display_and_fatal(e);
    }
    s.drop(&s);
    SoN(200, status);
}

void
post(void){
    __env__
    e = httpcli.post(urlstr, &s, &status);
    if(nil != e){
        __display_and_fatal(e);
    }
    s.drop(&s);
    SoN(200, status);
}

void
bad_req_invalid_body_siz(void){
    //test bad request info
    __env__
    s.dsiz = 1000000000;
    if(nil != (e = httpcli.post("http://localhost:9000/body_add_one", &s, &status))){
        __clean_errchain(e);
    }
    So(-1, status);
    So(utils.non_drop, s.drop);
    s.drop(&s);
}

_i
main(void){
    get();
    post();
    bad_req_invalid_body_siz();
}
