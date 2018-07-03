#ifndef _OS_H
#define _OS_H

#include "utils.h"
#include <sys/socket.h>

//Can be used to impl multi process_mode server
//MUST use helper macros to adapt variable os: CMSG_SPACE/CMSG_LEN/CMSG_DATA
struct fd_trans_env{
    struct msghdr msg;

    //CMSG_SPACE(_): possiable max data len
    char cmsgbuf[CMSG_SPACE(sizeof(_i))];
    struct cmsghdr *cmsg;
};

struct os{
    void (*daemonize) (const char *);
    error_t *(* rm_all) (char *);

    error_t *(*set_nonblocking) (_i);
    error_t *(*set_blocking) (_i);

    error_t *(*socket_new) (const char *, const char *, _i *);
    error_t *(*ip_socket_new) (const char *, const char *, _i *) __prm_nonnull;
    error_t *(*unix_socket_new) (const char *, _i *) __prm_nonnull;

    error_t * (*listen) (_i);

    error_t *(*connect) (const char *, const char *, _i *);

    error_t *(*send) (_i, void *, size_t);
    error_t *(*sendmsg) (_i, struct iovec *, size_t);
    error_t *(*recv) (_i, void *, size_t);
    error_t *(*recvall) (_i, void *, size_t);

    void (*fd_trans_init) (struct fd_trans_env *);
    error_t *(*send_fd) (struct fd_trans_env *, const _i, const _i);
    error_t *(*recv_fd) (struct fd_trans_env *, const _i, _i *);
};

struct os os;

#endif //_OS_H
