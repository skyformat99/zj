#include <stdio.h>
#include <stdlib.h>
#include "libssh/callbacks.h"

#include "os_target.h"

#ifdef _OS_FREEBSD_ZJ
#include <malloc_np.h>
#else
#include "jemalloc/jemalloc.h"
#endif

#include "common.h"

#define __ssherr_new(__hdr) __err_new(ssh_get_error_code(__hdr), ssh_get_error(__hdr), nil)

__init static void
multi_threads_env_init(void) {
    //link with -lssh_threads
    //must be called before ssh_init()
	//always return SSH_OK, need not check it's ret
    ssh_threads_set_callbacks(ssh_threads_get_pthread());

	if(0 != ssh_init()){
		__info("libssh multi_threads_env_init failed");
		exit(1);
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
chan_drop(ssh_channel *chan){
	if(nil == *chan){
		return;
	}

	if(!ssh_channel_is_closed(*chan)){
		ssh_channel_close(*chan);
	}

	ssh_channel_free(*chan);
	*chan = nil;
}

//**exec once, and disconnect**
//@param host[in]: 
//@param port[in]: 
//@param username[in]: 
//@param timeout_secs[in]: 
//@param exit_status[out]: the exit_status of `cmd`
//@param cmd_info[out]: the stdout and stderr output of `cmd`
//@param cmd_info_siz[out]: 1 + strlen(cmd_info)
static Error *
exec_once(char *cmd, _i *exit_status, char **cmd_info, size_t *cmd_info_siz,
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

	__drop(chan_drop) ssh_channel chan = ssh_channel_new(session);
	if(nil == chan){
		return __ssherr_new(session);
	}

	if(SSH_OK != ssh_channel_open_session(chan)){
		return __ssherr_new(session);
	}

	if(SSH_OK != ssh_channel_request_exec(chan, cmd)){
		return __ssherr_new(session);
	}

	if(nil != exit_status){
		*exit_status = ssh_channel_get_exit_status(chan);
	}

	if(nil != cmd_info && nil != cmd_info_siz){
#define __max_recv_siz 8 * 1024
		*cmd_info_siz = 0;
		*cmd_info = __malloc(__max_recv_siz);
		*cmd_info_siz += sprintf(*cmd_info + *cmd_info_siz, "\n\x1b[31;01m"
				"================\n"
				"==== stderr ====\n"
				"================\n"
				"\x1b[00m");
		size_t rcsiz = ssh_channel_read(chan, *cmd_info + *cmd_info_siz, __max_recv_siz - *cmd_info_siz, 1);
		while(0 < rcsiz){
			*cmd_info_siz += rcsiz;
			rcsiz = ssh_channel_read(chan, *cmd_info + *cmd_info_siz, __max_recv_siz - *cmd_info_siz, 1);
		}

		*cmd_info_siz += sprintf(*cmd_info + *cmd_info_siz, "\n\x1b[31;01m"
				"================\n"
				"==== stdout ====\n"
				"================\n"
				"\x1b[00m");
		rcsiz = ssh_channel_read(chan, *cmd_info + *cmd_info_siz, __max_recv_siz - *cmd_info_siz, 0);
		while(0 < rcsiz){
			*cmd_info_siz += rcsiz;
			rcsiz = ssh_channel_read(chan, *cmd_info + *cmd_info_siz, __max_recv_siz - *cmd_info_siz, 0);
		}

		++(*cmd_info_siz); //sizeof('\0')
#undef __max_recv_siz
	}

	ssh_channel_send_eof(chan);
	return nil;
}

#ifdef _UNIT_TEST
#include "convey.c"
Main({
    Test("ssh cli tests", {
        Error *e = nil;
		_i exit_status = 0;
		char *recv_buf = nil;
		size_t recv_siz = 0;

        Convey("no output", {
            e = exec_once("ls", &exit_status, nil, &recv_siz, "localhost", 22, __UNIT_TEST_USERNAME, 3);
            if(nil != e) {
                __display_errchain(e);
            }

            So(nil == e);
			So(0 == exit_status);
			So(nil == recv_buf);
			So(0 == recv_siz);
        });

        Convey("no output, must fail", {
            e = exec_once("ls /root/_", &exit_status, &recv_buf, nil, "localhost", 22, __UNIT_TEST_USERNAME, 3);
            if(nil != e) {
                __display_errchain(e);
            }

            So(nil == e);
			So(0 != exit_status);
			So(nil == recv_buf);
			So(0 == recv_siz);
        });

        Convey("have output", {
            e = exec_once("ls", &exit_status, &recv_buf, &recv_siz, "localhost", 22, "fh", 3);
            if(nil != e) {
                __display_errchain(e);
            }

            So(nil == e);
			So(0 == exit_status);
			So(nil != recv_buf);
			//__info(recv_buf);
			//printf("%zd, %zd\n", 1 + strlen(recv_buf), recv_siz);
			So((1 + strlen(recv_buf)) == recv_siz);
        });

        Convey("have output, must fail", {
            e = exec_once("ls /root/_", &exit_status, &recv_buf, &recv_siz, "localhost", 22, "fh", 3);
            if(nil != e) {
                __display_errchain(e);
            }

            So(nil == e);
			So(0 != exit_status);
			So(nil != recv_buf);
			//__info(recv_buf);
			//printf("%zd, %zd\n", 1 + strlen(recv_buf), recv_siz);
			So((1 + strlen(recv_buf)) == recv_siz);
        });
    });
})
#endif
