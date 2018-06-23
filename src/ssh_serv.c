#include <utmp.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "libssh/callbacks.h"
#include "libssh/server.h"

#include "os_target.h"

#ifdef _OS_FREEBSD
#include <malloc_np.h>
#else
#include "jemalloc/jemalloc.h"
#endif

#ifdef _OS_DARWIN
#include <util.h>
#else
#include <pty.h>
#endif

#include "common.h"
#include "threadpool.h"

#define USER "myuser"
#define PASS "mypassword"
#define BUF_SIZE 1048576
#define SESSION_END (SSH_CLOSED | SSH_CLOSED_ERROR)

#define __ssherr_new(__hdr, __prev) __err_new(ssh_get_error_code(__hdr), ssh_get_error(__hdr), __prev)

static Error *
bind_config(ssh_bind sshbind) {
    int e = 0;
    char *bind_addr = getenv("ZJ_BIND_ADDR");
    char *bind_port= getenv("ZJ_BIND_PORT");
    char *rsa_kpath = getenv("ZJ_RSA_KEYPATH");
    char *ecdsa_kpath = getenv("ZJ_ECDSA_KEYPATH");

    if (NULL == bind_addr) { bind_addr = "0.0.0.0"; }
    if (NULL == bind_port) { bind_port= "54321"; }
    if (NULL == rsa_kpath) { rsa_kpath = "/usr/local/etc/ssh/ssh_host_rsa_key"; }
    if (NULL == ecdsa_kpath) { ecdsa_kpath = "/usr/local/etc/ssh/ssh_host_ecdsa_key"; }

    if (0 != (e = ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDADDR, bind_addr))) {
        return __ssherr_new(sshbind, nil);
    }

    if (0 != (e = ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, bind_port))) {
        return __ssherr_new(sshbind, nil);
    }

    if (0 != (e = ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY, rsa_kpath))) {
        return __ssherr_new(sshbind, nil);
    }

    if (0 != (e = ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_ECDSAKEY, ecdsa_kpath))) {
        return __ssherr_new(sshbind, nil);
    }

    return nil;
}

/* A userdata struct for channel. */
struct channel_data_struct {
    /* pid of the child process the channel will spawn. */
    pid_t pid;
    /* For PTY allocation */
    socket_t pty_master;
    socket_t pty_slave;
    /* For communication with the child process. */
    socket_t child_stdin;
    socket_t child_stdout;
    /* Only used for subsystem and exec requests. */
    socket_t child_stderr;
    /* Event which is used to poll the above descriptors. */
    ssh_event event;
    /* Terminal size struct. */
    struct winsize *winsize;
};

/* A userdata struct for session. */
struct session_data_struct {
    /* Pointer to the channel the session will allocate. */
    ssh_channel channel;
    int auth_attempts;
    int authenticated;
};

static int data_function(ssh_session session, ssh_channel channel, void *data,
                         uint32_t len, int is_stderr, void *userdata) {
    struct channel_data_struct *cdata = (struct channel_data_struct *) userdata;

    (void) session;
    (void) channel;
    (void) is_stderr;

    if (len == 0 || cdata->pid < 1 || kill(cdata->pid, 0) < 0) {
        return 0;
    }

    return write(cdata->child_stdin, (char *) data, len);
}

static int pty_request(ssh_session session, ssh_channel channel,
                       const char *term, int cols, int rows, int py, int px,
                       void *userdata) {
    struct channel_data_struct *cdata = (struct channel_data_struct *)userdata;

    (void) session;
    (void) channel;
    (void) term;

    cdata->winsize->ws_row = rows;
    cdata->winsize->ws_col = cols;
    cdata->winsize->ws_xpixel = px;
    cdata->winsize->ws_ypixel = py;

    if (openpty(&cdata->pty_master, &cdata->pty_slave, NULL, NULL,
                cdata->winsize) != 0) {
        fprintf(stderr, "Failed to open pty\n");
        return SSH_ERROR;
    }
    return SSH_OK;
}

static int pty_resize(ssh_session session, ssh_channel channel, int cols,
                      int rows, int py, int px, void *userdata) {
    struct channel_data_struct *cdata = (struct channel_data_struct *)userdata;

    (void) session;
    (void) channel;

    cdata->winsize->ws_row = rows;
    cdata->winsize->ws_col = cols;
    cdata->winsize->ws_xpixel = px;
    cdata->winsize->ws_ypixel = py;

    if (cdata->pty_master != -1) {
        return ioctl(cdata->pty_master, TIOCSWINSZ, cdata->winsize);
    }

    return SSH_ERROR;
}

static int exec_pty(const char *mode, const char *command,
                    struct channel_data_struct *cdata) {
    switch(cdata->pid = fork()) {
        case -1:
            close(cdata->pty_master);
            close(cdata->pty_slave);
            fprintf(stderr, "Failed to fork\n");
            return SSH_ERROR;
        case 0:
            close(cdata->pty_master);
            if (login_tty(cdata->pty_slave) != 0) {
                exit(1);
            }
            execl("/bin/sh", "sh", mode, command, NULL);
            exit(0);
        default:
            close(cdata->pty_slave);
            /* pty fd is bi-directional */
            cdata->child_stdout = cdata->child_stdin = cdata->pty_master;
    }
    return SSH_OK;
}

static int exec_nopty(const char *command, struct channel_data_struct *cdata) {
    int in[2], out[2], err[2];

    /* Do the plumbing to be able to talk with the child process. */
    if (pipe(in) != 0) {
        goto stdin_failed;
    }
    if (pipe(out) != 0) {
        goto stdout_failed;
    }
    if (pipe(err) != 0) {
        goto stderr_failed;
    }

    switch(cdata->pid = fork()) {
        case -1:
            goto fork_failed;
        case 0:
            /* Finish the plumbing in the child process. */
            close(in[1]);
            close(out[0]);
            close(err[0]);
            dup2(in[0], STDIN_FILENO);
            dup2(out[1], STDOUT_FILENO);
            dup2(err[1], STDERR_FILENO);
            close(in[0]);
            close(out[1]);
            close(err[1]);
            /* exec the requested command. */
            execl("/bin/sh", "sh", "-c", command, NULL);
            exit(0);
    }

    close(in[0]);
    close(out[1]);
    close(err[1]);

    cdata->child_stdin = in[1];
    cdata->child_stdout = out[0];
    cdata->child_stderr = err[0];

    return SSH_OK;

fork_failed:
    close(err[0]);
    close(err[1]);
stderr_failed:
    close(out[0]);
    close(out[1]);
stdout_failed:
    close(in[0]);
    close(in[1]);
stdin_failed:
    return SSH_ERROR;
}

static int exec_request(ssh_session session, ssh_channel channel,
                        const char *command, void *userdata) {
    struct channel_data_struct *cdata = (struct channel_data_struct *) userdata;


    (void) session;
    (void) channel;

    if(cdata->pid > 0) {
        return SSH_ERROR;
    }

    if (cdata->pty_master != -1 && cdata->pty_slave != -1) {
        return exec_pty("-c", command, cdata);
    }
    return exec_nopty(command, cdata);
}

static int shell_request(ssh_session session, ssh_channel channel,
                         void *userdata) {
    struct channel_data_struct *cdata = (struct channel_data_struct *) userdata;

    (void) session;
    (void) channel;

    if(cdata->pid > 0) {
        return SSH_ERROR;
    }

    if (cdata->pty_master != -1 && cdata->pty_slave != -1) {
        return exec_pty("-l", NULL, cdata);
    }
    /* Client requested a shell without a pty, let's pretend we allow that */
    return SSH_OK;
}

static int auth_password(ssh_session session, const char *user,
                         const char *pass, void *userdata) {
    struct session_data_struct *sdata = (struct session_data_struct *) userdata;

    (void) session;

    if (strcmp(user, USER) == 0 && strcmp(pass, PASS) == 0) {
        sdata->authenticated = 1;
        return SSH_AUTH_SUCCESS;
    }

    sdata->auth_attempts++;
    return SSH_AUTH_DENIED;
}

static ssh_channel channel_open(ssh_session session, void *userdata) {
    struct session_data_struct *sdata = (struct session_data_struct *) userdata;

    sdata->channel = ssh_channel_new(session);
    return sdata->channel;
}

static int process_stdout(socket_t fd, int revents, void *userdata) {
    char buf[BUF_SIZE];
    int n = -1;
    ssh_channel channel = (ssh_channel) userdata;

    if (channel != NULL && (revents & POLLIN) != 0) {
        n = read(fd, buf, BUF_SIZE);
        if (n > 0) {
            ssh_channel_write(channel, buf, n);
        }
    }

    return n;
}

static int process_stderr(socket_t fd, int revents, void *userdata) {
    char buf[BUF_SIZE];
    int n = -1;
    ssh_channel channel = (ssh_channel) userdata;

    if (channel != NULL && (revents & POLLIN) != 0) {
        n = read(fd, buf, BUF_SIZE);
        if (n > 0) {
            ssh_channel_write_stderr(channel, buf, n);
        }
    }

    return n;
}

static void handle_session(ssh_event event, ssh_session session) {
    int n, rc;

    /* Structure for storing the pty size. */
    struct winsize wsize = {
        .ws_row = 0,
        .ws_col = 0,
        .ws_xpixel = 0,
        .ws_ypixel = 0
    };

    /* Our struct holding information about the channel. */
    struct channel_data_struct cdata = {
        .pid = 0,
        .pty_master = -1,
        .pty_slave = -1,
        .child_stdin = -1,
        .child_stdout = -1,
        .child_stderr = -1,
        .event = NULL,
        .winsize = &wsize
    };

    /* Our struct holding information about the session. */
    struct session_data_struct sdata = {
        .channel = NULL,
        .auth_attempts = 0,
        .authenticated = 0
    };

    struct ssh_channel_callbacks_struct channel_cb = {
        .userdata = &cdata,
        .channel_pty_request_function = pty_request,
        .channel_pty_window_change_function = pty_resize,
        .channel_shell_request_function = shell_request,
        .channel_exec_request_function = exec_request,
        .channel_data_function = data_function,
    };

    struct ssh_server_callbacks_struct server_cb = {
        .userdata = &sdata,
        .auth_password_function = auth_password,
        .channel_open_request_session_function = channel_open,
    };

    ssh_callbacks_init(&server_cb);
    ssh_callbacks_init(&channel_cb);

    ssh_set_server_callbacks(session, &server_cb);

    if (ssh_handle_key_exchange(session) != SSH_OK) {
        fprintf(stderr, "%s\n", ssh_get_error(session));
        return;
    }

    ssh_set_auth_methods(session, SSH_AUTH_METHOD_PASSWORD);
    ssh_event_add_session(event, session);

    n = 0;
    while (sdata.authenticated == 0 || sdata.channel == NULL) {
        /* If the user has used up all attempts, or if he hasn't been able to
         * authenticate in 10 seconds (n * 100ms), disconnect. */
        if (sdata.auth_attempts >= 3 || n >= 100) {
            return;
        }

        if (ssh_event_dopoll(event, 100) == SSH_ERROR) {
            fprintf(stderr, "%s\n", ssh_get_error(session));
            return;
        }
        n++;
    }

    ssh_set_channel_callbacks(sdata.channel, &channel_cb);

    do {
        /* Poll the main event which takes care of the session, the channel and
         * even our child process's stdout/stderr (once it's started). */
        if (ssh_event_dopoll(event, -1) == SSH_ERROR) {
          ssh_channel_close(sdata.channel);
        }

        /* If child process's stdout/stderr has been registered with the event,
         * or the child process hasn't started yet, continue. */
        if (cdata.event != NULL || cdata.pid == 0) {
            continue;
        }
        /* Executed only once, once the child process starts. */
        cdata.event = event;
        /* If stdout valid, add stdout to be monitored by the poll event. */
        if (cdata.child_stdout != -1) {
            if (ssh_event_add_fd(event, cdata.child_stdout, POLLIN, process_stdout,
                                 sdata.channel) != SSH_OK) {
                fprintf(stderr, "Failed to register stdout to poll context\n");
                ssh_channel_close(sdata.channel);
            }
        }

        /* If stderr valid, add stderr to be monitored by the poll event. */
        if (cdata.child_stderr != -1){
            if (ssh_event_add_fd(event, cdata.child_stderr, POLLIN, process_stderr,
                                 sdata.channel) != SSH_OK) {
                fprintf(stderr, "Failed to register stderr to poll context\n");
                ssh_channel_close(sdata.channel);
            }
        }
    } while(ssh_channel_is_open(sdata.channel) &&
            (cdata.pid == 0 || waitpid(cdata.pid, &rc, WNOHANG) == 0));

    close(cdata.pty_master);
    close(cdata.child_stdin);
    close(cdata.child_stdout);
    close(cdata.child_stderr);

    /* Remove the descriptors from the polling context, since they are now
     * closed, they will always trigger during the poll calls. */
    ssh_event_remove_fd(event, cdata.child_stdout);
    ssh_event_remove_fd(event, cdata.child_stderr);

    /* If the child process exited. */
    if (kill(cdata.pid, 0) < 0 && WIFEXITED(rc)) {
        rc = WEXITSTATUS(rc);
        ssh_channel_request_send_exit_status(sdata.channel, rc);
    /* If client terminated the channel or the process did not exit nicely,
     * but only if something has been forked. */
    } else if (cdata.pid > 0) {
        kill(cdata.pid, SIGKILL);
    }

    ssh_channel_send_eof(sdata.channel);
    ssh_channel_close(sdata.channel);

    /* Wait up to 5 seconds for the client to terminate the session. */
    for (n = 0; n < 50 && (ssh_get_status(session) & SESSION_END) == 0; n++) {
        ssh_event_dopoll(event, 100);
    }
}

/* SIGCHLD handler for cleaning up dead children. */
static void sigchld_handler(int signo __unuse) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

static void *
session_worker(void *session) {
    ssh_event event = ssh_event_new();
    if(nil == event){
		__info("create polling context failed");
    } else {
        handle_session(event, session);
        ssh_event_free(event);
    }

    ssh_disconnect(session);
    ssh_free(session);

	return nil;
}

Error *
run(void) {
    Error *e = nil;
    ssh_bind sshbind;
    struct sigaction sa;

    /* Set up SIGCHLD handler. */
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (0 != sigaction(SIGCHLD, &sa, NULL)) {
        return __err_new(errno, strerror(errno), nil);
    }

    //link with -lssh_threads
    //must be called before ssh_init()
    ssh_threads_set_callbacks(ssh_threads_get_pthread());
    ssh_init();

    sshbind = ssh_bind_new();
    e = bind_config(sshbind);
    if(nil != e){
        return __ssherr_new(sshbind, e);
    }

    if(0 > ssh_bind_listen(sshbind)){
        return __ssherr_new(sshbind, nil);
    }

	threadpool.init(8, 512); // if any_error occur, will exit(1)

    while(1){
        ssh_session session = ssh_new();
        if(nil == session){
            __info("allocate session failed");
			continue;
        }

        /* Blocks until there is a new incoming connection. */
        if(SSH_ERROR != ssh_bind_accept(sshbind, session)) {
            switch(threadpool.add(session_worker, session)) {
                case 0:
					break;
                default:
					__info("threadpool: task_new failed");
            }
        } else {
			__info(ssh_get_error(sshbind));
        }
    }

    ssh_bind_free(sshbind);
    ssh_finalize();

	return nil;
}

#ifdef _UNIT_TEST
#include "convey.c"
Main({
    Test("ssh server tests", {
        Convey("sshbind test", {
    		ssh_init();
    		Error *e = bind_config(ssh_bind_new());
        	if(nil != e) {
        	    __display_errchain(e);
        	}

            So(nil == e);
        });
        Convey("sshd test", {
    		Error *e = run();
        	if(nil != e) {
        	    __display_errchain(e);
        	}

            So(nil == e);
        });
    });
})
#endif
