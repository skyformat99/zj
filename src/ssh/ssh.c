#include "ssh.h"
#include <stdio.h>

#define __ssherr_new(__hdr) __err_new(ssh_get_error_code(__hdr), ssh_get_error(__hdr), nil)

static Error *
exec_once(char *cmd, _i *exit_status, Source *cmdout, char *host, _i port, char *username, time_t conn_timeout_secs);

static Error *
exec_once_default(char *cmd, char *host, _i port, char *username);

struct SSH ssh = {
    .exec = exec_once,
    .exec_default = exec_once_default,
};

__init static void
multi_threads_env_init(void) {
    //link with -lssh_threads
    //must be called before ssh_init()
    //always return SSH_OK, need not check it's ret
    ssh_threads_set_callbacks(ssh_threads_get_pthread());

    if(0 != ssh_init()){
        __fatal("libssh multi_threads_env_init failed");
    }
}

static void
session_drop(ssh_session *session){
    if(nil == *session){
        return;
    }

    if(ssh_is_connected(*session)){
        ssh_disconnect(*session);
    }

    ssh_free(*session);
    *session = nil;
}

static void
channel_drop(ssh_channel *channel){
    if(nil == *channel){
        return;
    }

    if(!ssh_channel_is_closed(*channel)){
        ssh_channel_close(*channel);
    }

    ssh_channel_free(*channel);
    *channel = nil;
}

//simple wrapper of exec_once()
static Error *
exec_once_default(char *cmd, char *host, _i port, char *username) {
    return exec_once(cmd, nil, nil, host, port, username, 10);
}

//**exec once, and disconnect**
//@param host[in]:
//@param port[in]:
//@param username[in]:
//@param timeout_secs[in]:
//@param exit_status[out]: the exit_status of `cmd`
//@param cmdout[out]: the stdout and stderr output of `cmd`
static Error *
exec_once(char *cmd, _i *exit_status, Source *cmdout,
        char *host, _i port, char *username, time_t conn_timeout_secs) {
    __drop(session_drop) ssh_session session = ssh_new();
    if(nil == session){
        return __err_new(-1, "create ssh_session failed", nil);
    }

    if(0 != ssh_options_set(session, SSH_OPTIONS_HOST, host)){
        return __ssherr_new(session);
    }

    if(0 != ssh_options_set(session, SSH_OPTIONS_PORT, &port)){
        return __ssherr_new(session);
    }

    if(0 != ssh_options_set(session, SSH_OPTIONS_USER, username)){
        return __ssherr_new(session);
    }

    if(0 != ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &conn_timeout_secs)){
        return __ssherr_new(session);
    }

    _i log_level = SSH_LOG_NOLOG;
    if(0 != ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &log_level)){
        return __ssherr_new(session);
    }

    if(SSH_OK != ssh_connect(session)){
        return __ssherr_new(session);
    }

    if(SSH_AUTH_SUCCESS != ssh_userauth_publickey_auto(session, nil, nil)){
        return __ssherr_new(session);
    }

    __drop(channel_drop) ssh_channel channel = ssh_channel_new(session);
    if(nil == channel){
        return __ssherr_new(session);
    }

    if(SSH_OK != ssh_channel_open_session(channel)){
        return __ssherr_new(session);
    }

    if(SSH_OK != ssh_channel_request_exec(channel, cmd)){
        return __ssherr_new(session);
    }

    if(nil != exit_status){
        *exit_status = ssh_channel_get_exit_status(channel);
    }

    if(nil != cmdout){
#define __max_recv_siz 2 * 1024
        cmdout->dsiz = 0;
        cmdout->data = __alloc(1 + __max_recv_siz);
        cmdout->drop = utils.sys_drop;

        cmdout->dsiz += sprintf(cmdout->data + cmdout->dsiz, "\n\x1b[31;01m"
                "==== stderr ====\n"
                "\x1b[00m");
        size_t rcsiz = ssh_channel_read(channel, cmdout->data + cmdout->dsiz, __max_recv_siz - cmdout->dsiz, 1);
        while(0 < rcsiz){
            cmdout->dsiz += rcsiz;
            rcsiz = ssh_channel_read(channel, cmdout->data + cmdout->dsiz, __max_recv_siz - cmdout->dsiz, 1);
        }

        cmdout->dsiz += sprintf(cmdout->data + cmdout->dsiz, "\x1b[31;01m"
                "==== stdout ====\n"
                "\x1b[00m");
        rcsiz = ssh_channel_read(channel, cmdout->data + cmdout->dsiz, __max_recv_siz - cmdout->dsiz, 0);
        while(0 < rcsiz){
            cmdout->dsiz += rcsiz;
            rcsiz = ssh_channel_read(channel, cmdout->data + cmdout->dsiz, __max_recv_siz - cmdout->dsiz, 0);
        }
#undef __max_recv_siz

        ((char *)cmdout->data)[cmdout->dsiz] = '\0';
        ++cmdout->dsiz;
        cmdout->data = __ralloc(cmdout->data, cmdout->dsiz);
    }

    ssh_channel_send_eof(channel);
    return nil;
}
