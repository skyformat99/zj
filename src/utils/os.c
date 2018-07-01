#include "os.h"

#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <poll.h>

#define UN_PATH_SIZ\
        sizeof(struct sockaddr_un)-((size_t) (& ((struct sockaddr_un*) 0)->sun_path))

static void daemonize(const char *runpath);
static error_t *remove_all(char *path);

struct os os = {
    .daemonize = daemonize,
    .rm_all = remove_all,
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

    if (FTW_F == file_type || FTW_SL == file_type || FTW_SLN == file_type) {
        if (0 != unlink(path)) {
            __info(path);
            rv = -1;
        }
    } else if (FTW_DP == file_type) {
        if (0 != rmdir(path)) {
            __info(path);
            rv = -1;
        }
    } else {
        __info("unknown file type");
    }

    return rv;
}

static error_t *
remove_all(char *path) {
    if(0 != nftw(path, remove_all_ctw_cb, 128, FTW_PHYS/*ignore symlink*/|FTW_DEPTH/*file first*/)){
        return __err_new(errno, strerror(errno), nil);
    }

    return nil;
}

///**
// * Socket
// */
//
//// Start server: TCP or UDP,
//// Option zServType: 1 for TCP, 0 for UDP.
//static _i
//zgenerate_serv_SD(char *zpHost, char *zpPort, char *zpUNPath, znet_proto_t zProtoType) {
//    _i zSd = -1,
//       zErrNo = -1;
//
//    if (NULL == zpUNPath) {
//        struct addrinfo *zpRes_ = NULL,
//                        *zpAddrInfo_ = NULL;
//        struct addrinfo zHints_  = {
//            .ai_flags = AI_PASSIVE|AI_NUMERICHOST|AI_NUMERICSERV
//        };
//
//        zHints_.ai_socktype = (zProtoUDP == zProtoType) ? SOCK_DGRAM : SOCK_STREAM;
//        zHints_.ai_protocol = (zProtoUDP == zProtoType) ? IPPROTO_UDP:IPPROTO_TCP;
//
//        if (0 != (zErrNo = getaddrinfo(zpHost, zpPort, &zHints_, &zpRes_))) {
//            zPRINT_ERR_EASY(gai_strerror(zErrNo));
//            exit(1);
//        }
//
//        for (zpAddrInfo_ = zpRes_; NULL != zpAddrInfo_; zpAddrInfo_ = zpAddrInfo_->ai_next) {
//            if(0 < (zSd = socket( zpAddrInfo_->ai_family, zHints_.ai_socktype, zHints_.ai_protocol))) {
//                break;
//            }
//        }
//
//        zCHECK_NEGATIVE_EXIT(zSd);
//
//        /* 不等待，直接重用地址与端口 */
//        zCHECK_NEGATIVE_EXIT(
//                setsockopt(zSd, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &zSd, sizeof(_i))
//                );
//
//        zCHECK_NEGATIVE_EXIT(
//                bind(zSd, zpRes_->ai_addr,
//                    (AF_INET6 == zpAddrInfo_->ai_family) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))
//                );
//
//        freeaddrinfo(zpRes_);
//    } else {
//        struct sockaddr_un zUN;
//
//        zCHECK_NEGATIVE_EXIT(
//                zSd = socket(PF_UNIX,
//                    (zProtoUDP == zProtoType) ? SOCK_DGRAM : SOCK_STREAM,
//                    0)
//                );
//
//        zUN.sun_family = PF_UNIX;
//        snprintf(zUN.sun_path, UN_PATH_SIZ,  /* 防止越界 */
//                "%s",
//                zpUNPath);
//
//        /* 尝试清除可能存在的旧文件 */
//        unlink(zpUNPath);
//
//        /* 不等待，直接重用地址与端口 */
//        zCHECK_NEGATIVE_EXIT(
//                setsockopt(zSd, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &zSd, sizeof(_i))
//                );
//
//        zCHECK_NEGATIVE_EXIT(
//                bind(zSd, (struct sockaddr *) &zUN, SUN_LEN(&zUN))
//                );
//    }
//
//    if(zProtoTCP == zProtoType){
//        zCHECK_NEGATIVE_EXIT( listen(zSd, 6) );
//    }
//
//    return zSd;
//}
//
//
///*
// *  将指定的套接字属性设置为非阻塞
// */
//static void
//zset_nonblocking(_i zSd) {
//    _i zOpts;
//    zCHECK_NEGATIVE_EXIT( zOpts = fcntl(zSd, F_GETFL) );
//    zOpts |= O_NONBLOCK;
//    zCHECK_NEGATIVE_EXIT( fcntl(zSd, F_SETFL, zOpts) );
//}
//
//
///*
// *  将指定的套接字属性设置为阻塞
// */
//static void
//zset_blocking(_i zSd) {
//    _i zOpts;
//    zCHECK_NEGATIVE_EXIT( zOpts = fcntl(zSd, F_GETFL) );
//    zOpts &= ~O_NONBLOCK;
//    zCHECK_NEGATIVE_EXIT( fcntl(zSd, F_SETFL, zOpts) );
//}
//
//
///* Used by client */
//static _i
//ztry_connect(struct sockaddr *zpAddr_, size_t zSiz, _i zIpFamily, _i zSockType, _i zProto) {
//    _i zSd = socket(zIpFamily, zSockType, zProto);
//    zCHECK_NEGATIVE_RETURN(zSd, -1);
//
//    zset_nonblocking(zSd);
//
//    errno = 0;
//    if (0 == connect(zSd, zpAddr_, zSiz)) {
//        zset_blocking(zSd);  /* 连接成功后，属性恢复为阻塞 */
//        return zSd;
//    } else if (EINPROGRESS == errno) {
//        struct pollfd zWd_ = {zSd, POLLIN|POLLOUT, -1};
//        /*
//         * poll 出错返回 -1，超时返回 0，
//         * 在超时之前成功建立连接，则返回可用连接数量
//         * connect 8 秒超时
//         */
//        if (0 < poll(&zWd_, 1, 8 * 1000)) {
//            zset_blocking(zSd);  /* 连接成功后，属性恢复为阻塞 */
//            return zSd;
//        }
//    } else {
//        zPRINT_ERR_EASY_SYS();
//    }
//
//    /* 已超时或出错 */
//    close(zSd);
//    return -1;
//}
//
//
///*
// * Used by client
// * @return success: socket_fd, failed: -1
// */
//static _i
//zconnect(char *zpHost, char *zpPort, char *zpUNPath, znet_proto_t zProto) {
//     _i zErrNo = -1;
//
//     _i zSockType, zProtoType;
//
//     if (zProtoUDP == zProto) {
//         zSockType = SOCK_DGRAM;
//         //zProtoType = IPPROTO_UDP;
//         zProtoType = 0;
//     } else {
//         zSockType = SOCK_STREAM;
//         //zProtoType = IPPROTO_TCP;
//         zProtoType = 0;
//     }
//
//    if (NULL == zpUNPath) {
//        struct addrinfo *zpRes_ = NULL,
//                        *zpAddrInfo_ = NULL,
//                        zHints_ = {
//                            .ai_socktype = zSockType,
//                            .ai_protocol = zProtoType,
//                            //.ai_flags = AI_NUMERICHOST|AI_NUMERICSERV,
//                        };
//
//        if (0 != (zErrNo = getaddrinfo(zpHost, zpPort, &zHints_, &zpRes_))) {
//            zPRINT_ERR_EASY(gai_strerror(zErrNo));
//            zErrNo = -1;
//            goto zEndMark;
//        }
//
//        for (zpAddrInfo_ = zpRes_; NULL != zpAddrInfo_; zpAddrInfo_ = zpAddrInfo_->ai_next) {
//            if(0 < (zErrNo = ztry_connect(zpAddrInfo_->ai_addr,
//                            (AF_INET6 == zpAddrInfo_->ai_family) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in),
//                            zpAddrInfo_->ai_family, zSockType, zProtoType))) {
//
//                freeaddrinfo(zpRes_);
//                goto zEndMark;
//            }
//        }
//
//        freeaddrinfo(zpRes_);
//        zErrNo = -1;
//        goto zEndMark;
//    } else {
//        struct sockaddr_un zUN;
//        zUN.sun_family = PF_UNIX;
//        snprintf(zUN.sun_path, UN_PATH_SIZ,  /* 防止越界 */
//                "%s",
//                zpUNPath);
//
//        if (0 > (zErrNo = ztry_connect((struct sockaddr *) &zUN,
//                        SUN_LEN(&zUN),
//                        PF_UNIX, zSockType, zProtoType))) {
//            zErrNo = -1;
//            goto zEndMark;
//        }
//    }
//
//zEndMark:
//    return zErrNo;
//}
//
//
//static _i
//zsendto(_i zSd, void *zpBuf, size_t zLen,
//        struct sockaddr *zpAddr_, socklen_t zAddrSiz) {
//    return sendto(zSd, zpBuf, zLen,
//            MSG_NOSIGNAL,
//            zpAddr_, zAddrSiz);
//}
//
//
//static _i
//zsend(_i zSd, void *zpBuf, size_t zLen) {
//    return send(zSd, zpBuf, zLen, MSG_NOSIGNAL);
//}
//
//
//static _i
//zsendmsg(_i zSd, struct iovec *zpVec_, size_t zVecSiz,
//        struct sockaddr *zpAddr_, socklen_t zAddrSiz) {
//    struct msghdr zMsg_ = {
//        .msg_name = zpAddr_,
//        .msg_namelen = zAddrSiz,
//        .msg_iov = zpVec_,
//        .msg_iovlen = zVecSiz,
//        .msg_control = NULL,
//        .msg_controllen = 0,
//        .msg_flags = 0
//    };
//
//    return sendmsg(zSd, &zMsg_, MSG_NOSIGNAL);
//}
//
//
//static _i
//zrecv_all(_i zSd, void *zpBuf, size_t zLen, struct sockaddr *zpAddr_, socklen_t *zpAddrSiz) {
//    return recvfrom(zSd, zpBuf, zLen, MSG_WAITALL|MSG_NOSIGNAL, zpAddr_, zpAddrSiz);
//}
//
//
//// static _i
//// zrecv_nohang(_i zSd, void *zpBuf, size_t zLen, struct sockaddr *zpAddr_, socklen_t *zpAddrSiz) {
////     return recvfrom(zSd, zpBuf, zLen, MSG_DONTWAIT|MSG_NOSIGNAL, zpAddr_, zpAddrSiz);
//// }
//
//
///*
// * 将文本格式的ip地址转换成二进制无符号整型数组(按网络字节序，即大端字节序)，以及反向转换
// * inet_pton: 返回 1 表示成功，返回 0 表示指定的地址无效，返回 -1 表示指定的ip类型错误
// */
//static _i
//zconvert_ip_str_to_bin(const char *zpStrAddr, zip_t zIpType, _ull *zpResOUT/* _ull[2] */) {
//    _i zErrNo = -1;
//
//    if (zIPTypeV6 == zIpType) {
//        struct in6_addr zIp6Addr_ = {{{0}}};
//
//        if (1 == inet_pton(AF_INET6, zpStrAddr, &zIp6Addr_)) {
//            zpResOUT[0] = * ((_ull *) (zIp6Addr_.__in6_u.__u6_addr8) + 1);
//            zpResOUT[1] = * ((_ull *) (zIp6Addr_.__in6_u.__u6_addr8) + 0);
//
//            zErrNo = 0;
//        }
//    } else {
//        struct in_addr zIpAddr_ = {0};
//
//        if (1 == inet_pton(AF_INET, zpStrAddr, &zIpAddr_)) {
//            zpResOUT[0] = zIpAddr_.s_addr;
//            zpResOUT[1] = 0xff;  /* 置为 "FF00::"，IPV6 组播地址*/
//
//            zErrNo = 0;
//        }
//    }
//
//    return zErrNo;
//}
//
//
//static _i
//zconvert_ip_bin_to_str(_ull *zpIpNumeric/* _ull[2] */, zip_t zIpType, char *zpResOUT/* char[INET6_ADDRSTRLEN] */) {
//    _i zErrNo = -1;
//
//    if (zIpType == zIPTypeV6) {
//        struct in6_addr zIpAddr_ = {{{0}}};
//        _uc *zp = (_uc *) zpIpNumeric;
//
//        zIpAddr_.__in6_u.__u6_addr8[0] = * (zp + 8);
//        zIpAddr_.__in6_u.__u6_addr8[1] = * (zp + 9);
//        zIpAddr_.__in6_u.__u6_addr8[2] = * (zp + 10);
//        zIpAddr_.__in6_u.__u6_addr8[3] = * (zp + 11);
//        zIpAddr_.__in6_u.__u6_addr8[4] = * (zp + 12);
//        zIpAddr_.__in6_u.__u6_addr8[5] = * (zp + 13);
//        zIpAddr_.__in6_u.__u6_addr8[6] = * (zp + 14);
//        zIpAddr_.__in6_u.__u6_addr8[7] = * (zp + 15);
//
//        zIpAddr_.__in6_u.__u6_addr8[8] = * (zp + 0);
//        zIpAddr_.__in6_u.__u6_addr8[9] = * (zp + 1);
//        zIpAddr_.__in6_u.__u6_addr8[10] = * (zp + 2);
//        zIpAddr_.__in6_u.__u6_addr8[11] = * (zp + 3);
//        zIpAddr_.__in6_u.__u6_addr8[12] = * (zp + 4);
//        zIpAddr_.__in6_u.__u6_addr8[13] = * (zp + 5);
//        zIpAddr_.__in6_u.__u6_addr8[14] = * (zp + 6);
//        zIpAddr_.__in6_u.__u6_addr8[15] = * (zp + 7);
//
//        if (NULL != inet_ntop(AF_INET6, &zIpAddr_, zpResOUT, INET6_ADDRSTRLEN)) {
//            zErrNo = 0;  /* 转换成功 */
//        }
//    } else {
//        struct in_addr zIpAddr_ = { .s_addr =  zpIpNumeric[0] };
//        if (NULL != inet_ntop(AF_INET, &zIpAddr_, zpResOUT, INET_ADDRSTRLEN)) {
//            zErrNo = 0;  /* 转换成功 */
//        }
//    }
//
//    return zErrNo;
//}
//
//
///*
// * 进程间传递文件描述符
// * 用于实现多进程模型服务器
// * 每次只传送一个 fd
// * @param: zUN 是 UNIX 域套接字，用作传输的工具
// * @param: zFd 是需要被传递的目标 fd
// * 返回 0 表示成功，否则表示失败
// * 若使用 UDP 通信，则必须事先完成了 connect
// */
//static _i
//zsend_fd(const _i zUN, const _i zFd, void *zpPeerAddr, _i zAddrSiz) {
//    /*
//     * 法1:可以只发送一个字节的常规数据，与连接连开区分
//     * 用于判断 sendmsg 的执行结果
//     * 法2:也可以发送空内容
//     */
//    //struct iovec zVec_ = {
//    //    .iov_base = "",
//    //    .iov_len = zBYTES(1),
//    //};
//
//    /*
//     * 存放将要被发送的 fd 的所有必要信息的空间
//     * 为适用不同的硬件平台，需要使用宏取值
//     */
//    char zCmsgBuf[CMSG_SPACE(sizeof(_i))];
//
//    /*
//     * sendmsg 直接使用的最外层结构体
//     */
//    struct msghdr zMsg_ = {
//        .msg_name = zpPeerAddr,
//        .msg_namelen = zAddrSiz,
//
//        //.msg_iov = &zVec_,
//        //.msg_iovlen = 1,
//        .msg_iov = NULL,
//        .msg_iovlen = 0,
//
//        .msg_control = zCmsgBuf,
//        .msg_controllen = CMSG_SPACE(sizeof(_i)),
//
//        .msg_flags = 0,
//    };
//
//    /*
//     * 以下为控制数据赋值
//     */
//    struct cmsghdr *zpCmsg = zMsg_.msg_control;
//
//    /*
//     * 声明传递的数据层级：SOL_SOCKET
//     * 与 setsockopt 中的含义一致
//     */
//    zpCmsg->cmsg_level = SOL_SOCKET;
//
//    /*
//     * 声明传递的数据类型是：
//     * socket controling management rights/权限
//     */
//    zpCmsg->cmsg_type = SCM_RIGHTS;
//
//    /*
//     * CMSG_SPACE(data_to_send) 永远 >= CMSG_LEN(data_to_send)
//     * 因为前者对实际要发送的数据对象，也做了对齐填充计算
//     * 而后者是 cmsghdr 结构体对齐填充后的大小，与实际数据的原始长度之和
//     */
//    zpCmsg->cmsg_len = CMSG_LEN(sizeof(_i));
//
//    /*
//     * cmsghdr 结构体的最后一个成员是 C99 风格的 data[] 形式
//     * 使用宏将目标 fd 写入此位置，
//     */
//    * (_i *) CMSG_DATA(zpCmsg) = zFd;
//
//    /*
//     * 成功发送了 1/0 个字节的数据，即说明执行成功
//     */
//    //if (1 == sendmsg(zUN, &zMsg_, MSG_NOSIGNAL)) {
//    if (0 == sendmsg(zUN, &zMsg_, MSG_NOSIGNAL)) {
//        return 0;
//    } else {
//        return -1;
//    }
//}
//
//
///*
// * 接收其它进程传递过来的 fd
// * 成功返回 fd（正整数），失败返回 -1
// * @param: 传递所用的 UNIX 域套接字
// * 若使用 UDP 通信，则必须事先完成了 connect
// */
//static _i
//zrecv_fd(const _i zFd) {
//    char _;
//    struct iovec zVec_ = {
//        .iov_base = &_,
//        .iov_len = zBYTES(1),
//    };
//
//    char zCmsgBuf[CMSG_SPACE(sizeof(_i))];
//
//    struct msghdr zMsg_ = {
//        .msg_name = NULL,
//        .msg_namelen = 0,
//
//        .msg_iov = &zVec_,
//        .msg_iovlen = 1,
//
//        .msg_control = zCmsgBuf,
//        .msg_controllen = CMSG_SPACE(sizeof(_i)),
//
//        .msg_flags = 0,
//    };
//
//    /* 目标 fd 空间预置为 -1 */
//    * (_i *) CMSG_DATA(CMSG_FIRSTHDR(&zMsg_)) = -1;
//
//    /*
//     * zsend_fd() 只发送了一个字节的常规数据
//     */
//    if (1 == recvmsg(zFd, &zMsg_, 0)) {
//        if (NULL == CMSG_FIRSTHDR(&zMsg_)) {
//            return -1;
//        } else {
//            /*
//             * 只发送了一个 cmsghdr 结构体 + fd
//             * 其最后的 data[] 存放的即是接收到的 fd
//             */
//            return * (_i *) CMSG_DATA(CMSG_FIRSTHDR(&zMsg_));
//        }
//    } else {
//        return -1;
//    }
//}

#undef UN_PATH_SIZ
