#include "os.h"

#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>

#include <netdb.h>
#ifdef _OS_FREEBSD
#include <netinet/in.h>
#endif

#include <sys/stat.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <poll.h>

#define _UN_PATH_SIZ\
        sizeof(struct sockaddr_un)-((size_t) (& ((struct sockaddr_un*) 0)->sun_path))

#define _UNIX_SOCKET_BIT_IDX 1
#define _PROTO_UDP_BIT_IDX 2

#define __err_new_sys() __err_new(errno, strerror(errno), nil)

static void daemonize(const char *runpath);
static Error *remove_all(char *path) __prm_nonnull __mustuse;

static Error *set_nonblocking(_i fd) __mustuse;
static Error *set_blocking(_i fd) __mustuse;

static Error *socket_new(const char *addr, const char *port, _i *fd) __mustuse;
static Error *ip_socket_new(const char *addr, const char *port, _i *fd) __prm_nonnull __mustuse;
static Error *unix_socket_new(const char *path, _i *fd) __prm_nonnull __mustuse;

inline static Error *serv_listen(_i fd) __mustuse;

static Error *cli_connect(const char *addr, const char *port, _i *fd) __mustuse;

inline static Error *_send(_i fd, void *data, ssize_t data_siz) __prm_nonnull __mustuse;
inline static Error *connected_sendmsg(_i fd, struct iovec *vec, size_t vec_cnt) __prm_nonnull __mustuse;
inline static Error *_recv(_i fd, void *data, size_t data_siz) __prm_nonnull __mustuse;

static void fd_trans_init(struct FdTransEnv *env) __prm_nonnull;
static Error * send_fd(struct FdTransEnv *env, const _i unix_fd, const _i fd_to_send) __prm_nonnull __mustuse;
static Error * recv_fd(struct FdTransEnv *env, const _i unix_fd, _i *fd_to_recv) __prm_nonnull __mustuse;

struct OS os = {
    .daemonize = daemonize,
    .rm_all = remove_all,

    .set_nonblocking = set_nonblocking,
    .set_blocking = set_blocking,

    .socket_new = socket_new,
    .ip_socket_new = ip_socket_new,
    .unix_socket_new = unix_socket_new,

    .listen = serv_listen,

    .connect = cli_connect,

    .send = _send,
    .sendmsg = connected_sendmsg,
    .recv = _recv,

    .fd_trans_init = fd_trans_init,
    .send_fd = send_fd,
    .recv_fd = recv_fd,
};

/**
 * Daemonize
 */
static void
daemonize(const char *runpath){
    _i rv, fd;
    pid_t pid;

    __ignore_all_signal();

    umask(0);
    if(0 != (rv = chdir(nil == runpath? "/" : runpath))){
        __fatal(strerror(errno));
    }

    if(0 > (pid = fork())){
        __fatal(strerror(errno));
    } else if(0 < pid){
        _exit(0);
    }

    setsid();

    if(0 > (pid = fork())){
        __fatal(strerror(errno));
    } else if(0 < pid){
        _exit(0);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    fd = open("/dev/null", O_RDWR);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
}

/**
 * same as: rm -rf $path
 */
static _i
remove_all_ctw_cb(const char *path, const struct stat *stat __unuse,
        int file_type, struct FTW *zpF __unuse){
    _i rv = 0;

    if (FTW_F == file_type || FTW_SL == file_type || FTW_SLN == file_type){
        if (0 != unlink(path)){
            __info(path);
            rv = -1;
        }
    } else if (FTW_DP == file_type){
        if (0 != rmdir(path)){
            __info(path);
            rv = -1;
        }
    } else {
        //unknown file type
        __info(path);
    }

    return rv;
}

static Error *
remove_all(char *path){
    if(0 != nftw(path, remove_all_ctw_cb, 128, FTW_PHYS/*ignore symlink*/|FTW_DEPTH/*file first*/)){
        return __err_new_sys();
    }

    return nil;
}

/**
 * fd
 */
//**used on server and client side**
//@param fd[in]: socket fd
static Error *
set_nonblocking(_i fd){
    _i opt;
    if(0 > (opt = fcntl(fd, F_GETFL))){
        return __err_new_sys();
    }

    opt |= O_NONBLOCK;
    if(0 > fcntl(fd, F_SETFL, opt)){
        return __err_new_sys();
    }

    return nil;
}

//**used on server and client side**
//@param fd[in]: socket fd
static Error *
set_blocking(_i fd){
    _i opt;
    if(0 > (opt = fcntl(fd, F_GETFL))){
        return __err_new_sys();
    }

    opt &= ~O_NONBLOCK;
    if(0 > fcntl(fd, F_SETFL, opt)){
        return __err_new_sys();
    }

    return nil;
}

/**
 * Socket
 */
static void
addrinfo_drop(struct addrinfo **restrict ais){
    if(nil != ais && nil != *ais){
        freeaddrinfo(*ais);
    }
}

//@param fd[in]:
inline static Error *
serv_listen(_i fd){
    if(0 >listen(fd, 6)){
        return __err_new(-1, "socket listen failed", nil);
    }

    return nil;
}

//**used on server side**
//@param addr[in]: unix socket path, or serv ip, or url
//@param port[in]: serv port
//@param fd[in and out]: [in] used as bit mark, [out]: generated socket fd
static Error *
socket_new(const char *addr, const char *port, _i *fd){
    if(!(addr && fd)){
        return __err_new(-1, "param<addr, fd> can't be nil", nil);
    }

    Error *e = nil;

    if(__check_bit(*fd, _UNIX_SOCKET_BIT_IDX)){
        e = unix_socket_new(addr, fd);
    } else {
        e = ip_socket_new(addr, port, fd);
    }

    if(nil != e){
        e = __err_new(-1, "socket create failed", e);
    }

    return e;
}

//**used on server side**
//@param addr[in]: serv ip or url
//@param port[in]: serv port
//@param fd[in and out]: [in] UDP if second_bit setted, or TCP; [out] ==> generated socket fd
static Error *
ip_socket_new(const char *addr, const char *port, _i *fd){
    struct addrinfo hints_  = {
        .ai_flags = AI_PASSIVE|AI_NUMERICSERV,
        .ai_socktype = __check_bit(*fd, _PROTO_UDP_BIT_IDX) ? SOCK_DGRAM : SOCK_STREAM,
        .ai_protocol = __check_bit(*fd, _PROTO_UDP_BIT_IDX) ? IPPROTO_UDP: IPPROTO_TCP,
    };

    __drop(addrinfo_drop) struct addrinfo *ais = nil;
    struct addrinfo *ai;
    _i rv;

    if (0 != (rv = getaddrinfo(addr, port, &hints_, &ais))){
        return __err_new(rv, gai_strerror(rv), nil);
    }

    for (ai = ais; nil != ai; ai = ai->ai_next){
        if(0 < (*fd = socket(ai->ai_family, hints_.ai_socktype, hints_.ai_protocol))){
            break;
        }
    }

    if(0 > *fd){
        return __err_new(-1, "socket create failed", nil)
    }

    if(0 > setsockopt(*fd, SOL_SOCKET, SO_REUSEPORT, fd, sizeof(_i))){
        close(*fd);
        return __err_new_sys()
    }

    if(0 > bind(*fd, ais->ai_addr,
                (AF_INET6 == ai->ai_family) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))){
        close(*fd);
        return __err_new_sys()
    }

    return nil;
}

//**used on server side**
//@param path[in]: unix socket path
//@param fd[in and out]: [in] UDP if second_bit setted, or TCP; [out] ==> generated socket fd
static Error *
unix_socket_new(const char *path, _i *fd){
    struct sockaddr_un un = {
        .sun_family = AF_UNIX,
    };
    _i unlen = sizeof(un) - sizeof(un.sun_path);
    unlen += snprintf(un.sun_path, _UN_PATH_SIZ, "%s", path);
    unlen++; //'\0'

    if(0 > (*fd = socket(AF_UNIX, __check_bit(*fd, _PROTO_UDP_BIT_IDX) ? SOCK_DGRAM : SOCK_STREAM, 0))){
        *fd = -1;
        return __err_new_sys();
    }

    if(0 > setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, fd, sizeof(_i))){
        close(*fd);
        return __err_new_sys()
    }

    if(0 > bind(*fd, (struct sockaddr *) &un, unlen)){
        close(*fd);
        *fd = -1;
        return __err_new_sys();
    }

    return nil;
}

// timeout: 8000ms(8 secs)
// poll return: 0 for timeout, 1 for success, -1 for error
static Error *
do_connect(_i fd, struct sockaddr *sockaddr, size_t siz){
    Error *e = nil;
    if(nil == (e = set_nonblocking(fd))){
        if (0 == connect(fd, sockaddr, siz)){
            if(nil != (e = set_blocking(fd))){
                close(fd);
                e = __err_new(-1, "socket: set_blocking failed", e);
            }
        } else {
            if (EINPROGRESS == errno){
                struct pollfd ev = {
                    .fd = fd,
                    .events = POLLIN|POLLOUT,
                    .revents = -1,
                };

                if (0 < poll(&ev, 1, 8 * 1000)){
                    if(nil != (e = set_blocking(fd))){
                        close(fd);
                        e = __err_new(-1, "socket: set_blocking failed", e);
                    }
                } else {
                    e = __err_new_sys();
                }
            } else {
                e = __err_new_sys();
            }
        }
    } else {
        e = __err_new(-1, "socket: set_nonblocking failed", e);
    }

    return e;
}

//**used on client side**
//@param addr[in]: unix socket path, or serv ip, or url
//@param port[in]: serv port
//@param fd[in and out]: [in] used as bit mark, [out]: connected fd
static Error *
cli_connect(const char *addr, const char *port, _i *fd){
    if(!(addr && fd)){
        return __err_new(-1, "param<addr, fd> can't be nil", nil);
    }

    _i rv;
    _i socksiz;
    _i socktype;
    Error *e = nil;

    if(__check_bit(*fd, _PROTO_UDP_BIT_IDX)){
        socktype =  SOCK_DGRAM;
    } else {
        socktype =  SOCK_STREAM;
    }

    if(__check_bit(*fd, _UNIX_SOCKET_BIT_IDX)){
        struct sockaddr_un un = {
            .sun_family = AF_UNIX,
        };
        socksiz = sizeof(un) - sizeof(un.sun_path);
        socksiz += snprintf(un.sun_path, _UN_PATH_SIZ, "%s", addr);
        socksiz++; //'\0'

        if(0 > (*fd = socket(AF_UNIX, socktype, 0))){
            e = __err_new_sys();
        } else {
            e = do_connect(*fd, (struct sockaddr *)(&un), socksiz);
        }
    } else {
        __drop(addrinfo_drop) struct addrinfo *ais = nil;
        struct addrinfo *ai;

        if (0 > (rv = getaddrinfo(addr, port, nil, &ais))){
            return __err_new(rv, gai_strerror(rv), nil);
        }

        *fd = -1;
        for(ai = ais; nil != ai; ai = ai->ai_next){
            if(AF_INET6 == ai->ai_family){
                socksiz = sizeof(struct sockaddr_in6);
            } else {
                socksiz = sizeof(struct sockaddr_in);
            }
            if(0 > (*fd = socket(ai->ai_family, socktype, 0))){
                e = __err_new_sys();
                break;
            }
            if(nil == (e = do_connect(*fd, ai->ai_addr, socksiz))){
                break;
            } else {
                e = nil;
                close(*fd);
            }
        }

        if(nil != e){
            e = __err_new(-1, "connect failed", e);
        }
    }

    return e;
}

static Error *
_recv(_i fd, void *data, size_t data_siz){
    if(0 > recv(fd, data, data_siz, 0)){
        return __err_new_sys();
    }

    return nil;
}

static Error *
_send(_i fd, void *data, ssize_t data_siz){
    if(data_siz > send(fd, data, data_siz, 0)){
        return __err_new_sys();
    }

    return nil;
}

static Error *
connected_sendmsg(_i fd, struct iovec *vec, size_t vec_cnt){
    struct msghdr msg = {
        .msg_name = nil,
        .msg_namelen = 0,
        .msg_iov = vec,
        .msg_iovlen = vec_cnt,
        .msg_control = nil,
        .msg_controllen = 0,
        .msg_flags = 0
    };

    if(0 > sendmsg(fd, &msg, 0)){
        return __err_new_sys();
    }

    return nil;
}

//@param env[in, inner write out]
static void
fd_trans_init(struct FdTransEnv *env){
    //if using UDP, MUST have connected
    env->msg.msg_name = nil;
    env->msg.msg_namelen = 0;

    //keep 1 byte regular data to send
    static struct iovec vdata[1];
    static char buf[1] = "";
    vdata[0].iov_base = buf,
    vdata[0].iov_len = 1,

    env->msg.msg_iov = vdata;
    env->msg.msg_iovlen = 1;

    env->msg.msg_control = env->cmsgbuf;
    env->msg.msg_controllen = CMSG_SPACE(sizeof(_i)),

    //no trans flags
    env->msg.msg_flags = 0;

    //easy to use
    env->cmsg = env->msg.msg_control;

    env->cmsg->cmsg_level = SOL_SOCKET;
    env->cmsg->cmsg_type = SCM_RIGHTS; //socket controling management rights

    //CMSG_LEN(_): actual data len
    env->cmsg->cmsg_len = CMSG_LEN(sizeof(_i));
}

//@param env[in]:
//@param unix_fd[in]:
//@param fd_to_send[in]:
//USAGE:
//    - define a FdTransEnv struct,
//    - and init it with fd_trans_int(_) ,
//    - then pass it to send_fd,
//    - following send_fd(_) can reuse it
static Error *
send_fd(struct FdTransEnv *env, const _i unix_fd, const _i fd_to_send){
    //write data
    *(_i *)CMSG_DATA(env->cmsg) = fd_to_send;

    if (0 > sendmsg(unix_fd, &env->msg, MSG_NOSIGNAL)){
        return __err_new_sys();
    }

    return nil;
}

//@param env[in]:
//@param unix_fd[in]:
//@param fd_to_recv[out]:
//USAGE:
//    - define a FdTransEnv struct,
//    - and init it with fd_trans_int(_) ,
//    - then pass it to recv_fd,
//    - following recv_fd(_) can reuse it
static Error *
recv_fd(struct FdTransEnv *env, const _i unix_fd, _i *fd_to_recv){
    //*(_i *)CMSG_DATA(CMSG_FIRSTHDR(&env->msg)) = -1;

    //at least 1 byte data
    if (1 > recvmsg(unix_fd, &env->msg, 0)){
        return __err_new_sys();
    } else {
        //a success recvmsg will update env->cmsg
        if (nil == CMSG_FIRSTHDR(&env->msg)){
            return __err_new(-1, "recv_fd failed", nil);
        } else {
            *fd_to_recv = *(_i *)CMSG_DATA(CMSG_FIRSTHDR(&env->msg));
        }
    }

    return nil;
}

#undef _UN_PATH_SIZ
#undef __err_new_sys
