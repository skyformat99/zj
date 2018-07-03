#include "os.c"
#include "utils.c"
#include "threadpool.c"
#include <sys/wait.h>

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

void *
accept_worker(void *serv_fds){
    _i *fd = serv_fds;
    _i rv;
    error_t *e;

    char recvbuf[sizeof("abc")];

    while(1){
        if(0 > (rv = accept(fd[0], nil, nil))){
            __fatal(strerror(errno));
        } else {
            memset(recvbuf, 0, sizeof("abc"));
            __check_fatal(e, os.recv(rv, recvbuf, sizeof("abc")));
            So(0, strcmp("abc", recvbuf));

            __check_fatal(e, os.send(rv, "def", sizeof("def")));
        }

        if(0 > (rv = accept(fd[1], nil, nil))){
            __fatal(strerror(errno));
        } else {
            memset(recvbuf, 0, sizeof("abc"));
            __check_fatal(e, os.recv(rv, recvbuf, sizeof("abc")));
            So(0, strcmp("abc", recvbuf));

            struct iovec iv[1];
            iv[0].iov_base = "def";
            iv[0].iov_len = sizeof("def");
            __check_fatal(e, os.sendmsg(rv, iv, 1));
        }
    }

    return nil;
}

void
serv_fd_frop(_i **fd){
    close((*fd)[0]);
    close((*fd)[1]);
    free(*fd);
}

void
tcp_communication(void){
    error_t *e;
    pid_t pid;

    const char *unix_path = "/tmp/__un__";
    const char *ip_addr = "localhost";
    const char *ip_port = "9527";

    unlink(unix_path);

    pid = fork();
    if(0 > pid){
        __fatal(strerror(errno));
    } else if(0 == pid){
        _i unix_cli_fd, ip_cli_fd;
        char recvbuf[sizeof("def")];

        sleep(1);

        unix_cli_fd = 0;
        __set_bit(unix_cli_fd, _UNIX_SOCKET_BIT_IDX);
        //__unset_bit(fd[0], _PROTO_UDP_BIT_IDX);
        if(nil != (e = os.connect(unix_path, nil, &unix_cli_fd))){
            __display_and_fatal(e);
        }
        SoLt(0, unix_cli_fd);

        __check_fatal(e, os.send(unix_cli_fd, "abc", sizeof("abc")));

        memset(recvbuf, 0, sizeof("def"));
        __check_fatal(e, os.recv(unix_cli_fd, recvbuf, sizeof("def")));
        So(0, strcmp("def", recvbuf));

        ip_cli_fd = 0;
        //__unset_bit(fd[0], _UNIX_SOCKET_BIT_IDX);
        //__unset_bit(fd[0], _PROTO_UDP_BIT_IDX);
        if(nil != (e = os.connect(ip_addr, ip_port, &ip_cli_fd))){
            __display_and_fatal(e);
        }
        SoLt(0, ip_cli_fd);

        __check_fatal(e, os.send(ip_cli_fd, "abc", sizeof("abc")));

        memset(recvbuf, 0, sizeof("def"));
        __check_fatal(e, os.recv(ip_cli_fd, recvbuf, sizeof("def")));
        So(0, strcmp("def", recvbuf));
    } else {
        __drop(serv_fd_frop) _i *fd = __alloc(2 * sizeof(_i));

        fd[0] = 0;
        __set_bit(fd[0], _UNIX_SOCKET_BIT_IDX);
        //__unset_bit(fd[0], _PROTO_UDP_BIT_IDX);
        if(nil != (e = os.socket_new(unix_path, nil, &fd[0]))){
            __display_and_fatal(e);
        }
        SoLt(0, fd[0]);

        fd[1] = 0;
        //__unset_bit(fd[0], _UNIX_SOCKET_BIT_IDX);
        //__unset_bit(fd[0], _PROTO_UDP_BIT_IDX);
        if(nil != (e = os.socket_new(ip_addr, ip_port, &fd[1]))){
            __display_and_fatal(e);
        }
        SoLt(0, fd[1]);

        if(nil != (e = os.listen(fd[0]))){
            __display_and_fatal(e);
        }

        if(nil != (e = os.listen(fd[1]))){
            __display_and_fatal(e);
        }

        threadpool.addjob(accept_worker, fd);

        if(0 > waitpid(pid, nil, 0)){
            __fatal(strerror(errno));
        }
    }
}
    //error_t *(*sendmsg) (_i, struct iovec *, size_t);

    //void (*fd_trans_init) (struct fd_trans_env *);
    //error_t *(*send_fd) (struct fd_trans_env *, const _i, const _i);
    //error_t *(*recv_fd) (struct fd_trans_env *, const _i, _i *);

_i
main(void){
    rm_all();
    tcp_communication();
}
