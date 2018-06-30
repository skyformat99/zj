#include "sha1.c"
#include "utils.c"

#define __str_orig "abcdefg"
#define __res_lowercase "2fb5e13419fc89246865e7a324f476ec624e8740" // printf "abcdefg" | sha1sum

_i
main(void){
    char res[41];

    sha1.gen(__str_orig, sizeof(__str_orig) - 1, res);
    So(0, strcmp(res, __res_lowercase));

    _i fd = open("/tmp/x.txt", O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if(0 > fd){
        __fatal(strerror(errno));
    }

    if((sizeof(__str_orig) - 1) != write(fd, __str_orig, sizeof(__str_orig) - 1)){
        __fatal(strerror(errno));
    }

    sha1.file("/tmp/x.txt", res);
    So(0, strcmp(res, __res_lowercase));

    unlink("/tmp/x.txt");
}