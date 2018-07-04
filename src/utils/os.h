#ifndef _OS_H
#define _OS_H

#include "utils.h"
#include <sys/socket.h>

//Can be used to impl multi process_mode server
//MUST use helper macros to adapt variable os: CMSG_SPACE/CMSG_LEN/CMSG_DATA
struct FdTransEnv{
    struct msghdr msg;

    //CMSG_SPACE(_): possiable max data len
    char cmsgbuf[CMSG_SPACE(sizeof(_i))];
    struct cmsghdr *cmsg;
};

struct OS{
    void (*daemonize) (const char *);
    Error *(* rm_all) (char *) __mustuse;

    Error *(*set_nonblocking) (_i) __mustuse;
    Error *(*set_blocking) (_i) __mustuse;

    Error *(*socket_new) (const char *, const char *, _i *) __mustuse;
    Error *(*ip_socket_new) (const char *, const char *, _i *) __prm_nonnull __mustuse;
    Error *(*unix_socket_new) (const char *, _i *) __prm_nonnull __mustuse;

    Error * (*listen) (_i) __mustuse;

    Error *(*connect) (const char *, const char *, _i *) __mustuse;

    Error *(*send) (_i, void *, ssize_t) __mustuse;
    Error *(*sendmsg) (_i, struct iovec *, size_t) __mustuse;
    Error *(*recv) (_i, void *, size_t) __mustuse;

    void (*fd_trans_init) (struct FdTransEnv *);
    Error *(*send_fd) (struct FdTransEnv *, const _i, const _i) __mustuse;
    Error *(*recv_fd) (struct FdTransEnv *, const _i, _i *) __mustuse;
};

struct OS os;

#endif //_OS_H
