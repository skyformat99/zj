#include "os.c"
#include "utils.c"

void
rm_all(void){
    error_t *e;
    _i fd;
    struct stat s;

    os.rm_all("/tmp/_");
    mkdir("/tmp/_", 0700);
    mkdir("/tmp/_/a", 0700);
    mkdir("/tmp/_/a/b", 0700);
    mkdir("/tmp/_/a/b/c", 0700);
    fd = open("/tmp/_/a/b/c/file", O_CREAT|O_WRONLY, 0600);
    if(0 > fd){
        __fatal(strerror(errno));
    }
    if(1 != write(fd, "", 1)){
        __fatal(strerror(errno));
    }
    close(fd);

    if(nil != (e = os.rm_all("/tmp/_"))){
        __display_errchain(e);
    }
    So(-1, stat("/tmp/_", &s));
    So(ENOENT, errno);
}

_i
main(void){
    rm_all();
}
