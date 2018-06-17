#include <stdlib.h>
#include "libssh/callbacks.h"

#include "zj_common.h"

Error *
ssh_exec_once(char *cmd, _i *exit_status, char **cmd_info, size_t *cmd_info_siz, char *host, _i port, char *username, time_t conn_timeout_secs) __mustuse;

Error *
ssh_exec_once_default(char *cmd, char *host, _i port, char *username) __mustuse;
