/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef INCLUDE_transports_http_h__
#define INCLUDE_transports_http_h__

#include "buffer.h"

GIT_INLINE(int) git_http__user_agent(git_buf *buf)
{
    const char *ua = git_libgit2__user_agent();

    if (!ua)
        ua = "libgit2 " LIBGIT2_VERSION;

    return git_buf_printf(buf, "git/2.0 (%s)", ua);
}

#endif
