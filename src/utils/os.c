#include "os.h"

#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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

#define _SO_REUSE SO_REUSEADDR|SO_REUSEPORT

#define __err_new_sys() __err_new(errno, strerror(errno), nil)

static void daemonize(const char *runpath);
static error_t *remove_all(char *path);

static error_t *set_nonblocking(_i fd);
static error_t *set_blocking(_i fd);

static error_t *socket_new(const char *addr, const char *port, _i *fd);
static error_t *ip_socket_new(const char *addr, const char *port, _i *fd) __prm_nonnull;
static error_t *unix_socket_new(const char *path, _i *fd) __prm_nonnull;

inline static error_t *serv_listen(_i fd);

static error_t *cli_connect(const char *addr, const char *port, _i *fd);

inline static error_t *_send(_i fd, void *data, size_t data_siz);
inline static error_t *connected_sendmsg(_i fd, struct iovec *vec, size_t vec_cnt);
inline static error_t *_recv(_i fd, void *data, size_t data_siz);
inline static error_t *recvall(_i fd, void *data, size_t data_siz);

struct os os = {
    .daemonize = daemonize,
    .rm_all = remove_all,

    .set_nonblocking = set_nonblocking,
    .set_blocking = set_blocking,

    .socket_new = socket_new,
    .ip_socket_new = ip_socket_new,
    .unix_socket_new = unix_socket_new,

    .connect = cli_connect,

    .send = _send,
    .sendmsg = connected_sendmsg,
    .recv = _recv,
    .recvall = recvall,
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

static error_t *
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
static error_t *
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
static error_t *
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
inline static error_t *
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
static error_t *
socket_new(const char *addr, const char *port, _i *fd){
    if(!(addr && fd)){
        return __err_new(-1, "param<addr, fd> can't be nil", nil);
    }

    error_t *e;

    if(__check_bit(*fd, _UNIX_SOCKET_BIT_IDX)){
        e = unix_socket_new(addr, fd);
    } else {
        e = ip_socket_new(addr, port, fd);
    }

    if(nil != e){
        return __err_new(-1, "socket create failed", e);
    }

    return nil;
}

//**used on server side**
//@param addr[in]: serv ip or url
//@param port[in]: serv port
//@param fd[in and out]: [in] UDP if second_bit setted, or TCP; [out] ==> generated socket fd
static error_t *
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
        if(0 < (*fd = socket( ai->ai_family, hints_.ai_socktype, hints_.ai_protocol))){
            break;
        }
    }

    if(0 > *fd){
        return __err_new(-1, "socket create failed", nil)
    }

    if(0 > setsockopt(*fd, SOL_SOCKET, _SO_REUSE, fd, sizeof(_i))){
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
static error_t *
unix_socket_new(const char *path, _i *fd){
    struct sockaddr_un un = {
        .sun_family = AF_UNIX,
    };
    snprintf(un.sun_path, _UN_PATH_SIZ, "%s", path);

    if(0 > (*fd = socket(AF_UNIX, __check_bit(*fd, _PROTO_UDP_BIT_IDX) ? SOCK_DGRAM : SOCK_STREAM, 0))){
        *fd = -1;
        return __err_new_sys();
    }

    if(0 > setsockopt(*fd, SOL_SOCKET, _SO_REUSE, fd, sizeof(_i))){
        close(*fd);
        *fd = -1;
        return __err_new_sys();
    }

    if(0 > bind(*fd, (struct sockaddr *) &un, sizeof(struct sockaddr_un))){
        close(*fd);
        *fd = -1;
        return __err_new_sys();
    }

    return nil;
}

// timeout: 8000ms(8 secs)
// poll return: 0 for timeout, 1 for success, -1 for error
#define __do_connect(__sockaddr, __siz) ({\
    error_t *e = nil;\
    if(nil == (e = set_nonblocking(*fd))){\
        if (0 == connect(*fd, __sockaddr, __siz)){\
            if(nil != (e = set_blocking(*fd))){\
                close(*fd);\
                e = __err_new(-1, "socket: set_blocking failed", e);\
            }\
        } else {\
            if (EINPROGRESS == errno){\
                struct pollfd ev = {\
                    .fd = *fd,\
                    .events = POLLIN|POLLOUT,\
                    .revents = -1,\
                };\
\
                if (0 < poll(&ev, 1, 8 * 1000)){\
                    if(nil != (e = set_blocking(*fd))){\
                        close(*fd);\
                        e = __err_new(-1, "socket: set_blocking failed", e);\
                    }\
                } else {\
                    e = __err_new_sys();\
                }\
            } else {\
                e = __err_new_sys();\
            }\
        }\
    } else {\
        e = __err_new(-1, "socket: set_nonblocking failed", e);\
    }\
    e;\
})

//**used on client side**
//@param addr[in]: unix socket path, or serv ip, or url
//@param port[in]: serv port
//@param fd[in and out]: [in] used as bit mark, [out]: connected fd
static error_t *
cli_connect(const char *addr, const char *port, _i *fd){
    if(!(addr && fd)){
        return __err_new(-1, "param<addr, fd> can't be nil", nil);
    }

    _i rv;
    error_t *e = nil;

    if (__check_bit(*fd, _UNIX_SOCKET_BIT_IDX)){
        struct sockaddr_un un = {
            .sun_family = AF_UNIX,
        };
        snprintf(un.sun_path, _UN_PATH_SIZ, "%s", addr);

        if(nil != (e = __do_connect((struct sockaddr *)&un, sizeof(struct sockaddr_un)))){
            return e;
        }
    } else {
        __drop(addrinfo_drop) struct addrinfo *ais = nil;
        struct addrinfo *ai;

        if (0 > (rv = getaddrinfo(addr, port, nil, &ais))){
            return __err_new(rv, gai_strerror(rv), nil);
        }

        *fd = -1;
        for (ai = ais; nil != ai; ai = ai->ai_next){
            if(nil == (e = __do_connect(ai->ai_addr, (AF_INET6 == ai->ai_family) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)))){
                break;
            }
        }

        if(nil != e){
            return __err_new(-1, "connect failed", nil);
        }
    }

    return nil;
}

inline static error_t *
_send(_i fd, void *data, size_t data_siz){
    if(0 > send(fd, data, data_siz, 0)){
        return __err_new_sys();
    }

    return nil;
}

inline static error_t *
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

inline static error_t *
_recv(_i fd, void *data, size_t data_siz){
    if(0 > recv(fd, data, data_siz, 0)){
        return __err_new_sys();
    }

    return nil;
}

inline static error_t *
recvall(_i fd, void *data, size_t data_siz){
    if(0 > recv(fd, data, data_siz, MSG_WAITALL)){
        return __err_new_sys();
    }

    return nil;
}

/*
 * 进程间传递文件描述符
 * 用于实现多进程模型服务器
 * 每次只传送一个 fd
 * @param: zUN 是 UNIX 域套接字，用作传输的工具
 * @param: zFd 是需要被传递的目标 fd
 * 返回 0 表示成功，否则表示失败
 * 若使用 UDP 通信，则必须事先完成了 connect
 */
static _i
zsend_fd(const _i zUN, const _i zFd, void *zpPeerAddr, _i zAddrSiz){
    /*
     * 法1:可以只发送一个字节的常规数据，与连接连开区分
     * 用于判断 sendmsg 的执行结果
     * 法2:也可以发送空内容
     */
    //struct iovec zVec_ = {
    //    .iov_base = "",
    //    .iov_len = zBYTES(1),
    //};

    /*
     * 存放将要被发送的 fd 的所有必要信息的空间
     * 为适用不同的硬件平台，需要使用宏取值
     */
    char zCmsgBuf[CMSG_SPACE(sizeof(_i))];

    /*
     * sendmsg 直接使用的最外层结构体
     */
    struct msghdr zMsg_ = {
        .msg_name = zpPeerAddr,
        .msg_namelen = zAddrSiz,

        //.msg_iov = &zVec_,
        //.msg_iovlen = 1,
        .msg_iov = nil,
        .msg_iovlen = 0,

        .msg_control = zCmsgBuf,
        .msg_controllen = CMSG_SPACE(sizeof(_i)),

        .msg_flags = 0,
    };

    /*
     * 以下为控制数据赋值
     */
    struct cmsghdr *zpCmsg = zMsg_.msg_control;

    /*
     * 声明传递的数据层级：SOL_SOCKET
     * 与 setsockopt 中的含义一致
     */
    zpCmsg->cmsg_level = SOL_SOCKET;

    /*
     * 声明传递的数据类型是：
     * socket controling management rights/权限
     */
    zpCmsg->cmsg_type = SCM_RIGHTS;

    /*
     * CMSG_SPACE(data_to_send) 永远 >= CMSG_LEN(data_to_send)
     * 因为前者对实际要发送的数据对象，也做了对齐填充计算
     * 而后者是 cmsghdr 结构体对齐填充后的大小，与实际数据的原始长度之和
     */
    zpCmsg->cmsg_len = CMSG_LEN(sizeof(_i));

    /*
     * cmsghdr 结构体的最后一个成员是 C99 风格的 data[] 形式
     * 使用宏将目标 fd 写入此位置，
     */
    * (_i *) CMSG_DATA(zpCmsg) = zFd;

    /*
     * 成功发送了 1/0 个字节的数据，即说明执行成功
     */
    //if (1 == sendmsg(zUN, &zMsg_, MSG_NOSIGNAL)){
    if (0 == sendmsg(zUN, &zMsg_, MSG_NOSIGNAL)){
        return 0;
    } else {
        return -1;
    }
}


/*
 * 接收其它进程传递过来的 fd
 * 成功返回 fd（正整数），失败返回 -1
 * @param: 传递所用的 UNIX 域套接字
 * 若使用 UDP 通信，则必须事先完成了 connect
 */
static _i
zrecv_fd(const _i zFd){
    char _;
    struct iovec zVec_ = {
        .iov_base = &_,
        .iov_len = 1,
    };

    char zCmsgBuf[CMSG_SPACE(sizeof(_i))];

    struct msghdr zMsg_ = {
        .msg_name = nil,
        .msg_namelen = 0,

        .msg_iov = &zVec_,
        .msg_iovlen = 1,

        .msg_control = zCmsgBuf,
        .msg_controllen = CMSG_SPACE(sizeof(_i)),

        .msg_flags = 0,
    };

    /* 目标 fd 空间预置为 -1 */
    * (_i *) CMSG_DATA(CMSG_FIRSTHDR(&zMsg_)) = -1;

    /*
     * zsend_fd() 只发送了一个字节的常规数据
     */
    if (1 == recvmsg(zFd, &zMsg_, 0)){
        if (nil == CMSG_FIRSTHDR(&zMsg_)){
            return -1;
        } else {
            /*
             * 只发送了一个 cmsghdr 结构体 + fd
             * 其最后的 data[] 存放的即是接收到的 fd
             */
            return * (_i *) CMSG_DATA(CMSG_FIRSTHDR(&zMsg_));
        }
    } else {
        return -1;
    }
}

#undef _UN_PATH_SIZ
#undef __err_new_sys
