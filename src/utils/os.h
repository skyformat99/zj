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
    Error *(* rm_all) (char *);

    Error *(*set_nonblocking) (_i);
    Error *(*set_blocking) (_i);

    Error *(*socket_new) (const char *, const char *, _i *);
    Error *(*ip_socket_new) (const char *, const char *, _i *) __prm_nonnull;
    Error *(*unix_socket_new) (const char *, _i *) __prm_nonnull;

    Error * (*listen) (_i);

    Error *(*connect) (const char *, const char *, _i *);

    Error *(*send) (_i, void *, ssize_t);
    Error *(*sendmsg) (_i, struct iovec *, size_t);
    Error *(*recv) (_i, void *, size_t);

    void (*fd_trans_init) (struct FdTransEnv *);
    Error *(*send_fd) (struct FdTransEnv *, const _i, const _i);
    Error *(*recv_fd) (struct FdTransEnv *, const _i, _i *);
};

struct OS os;

#endif //_OS_H
