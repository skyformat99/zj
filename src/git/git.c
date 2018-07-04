#include "utils.h"
#include "git2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define __err_new_git() __err_new(-1, nil == giterr_last() ? "Error without message" : giterr_last()->message, nil)

static Error *
git_global_env_init(void){
    if(0 > git_libgit2_init()){
        return __err_new_git();
    }
    return nil;
}

inline static void
git_global_env_clean(void){
    git_libgit2_shutdown();
}

//@param addr[in]: path/.git OR url/xxx.git
//@param hdr[out]: git_repo handler
static Error *
git_repo_open(const char *addr, git_repository **hdr){
    if(0 > git_repository_open(hdr, addr)){
        *hdr = nil;
        return __err_new_git();
    }
    return nil;
}

static void
git_repo_close(git_repository *hdr){
    git_repository_free(hdr);
}

static Error *
git_sshcred_cb(git_cred **cred, const char * username,
        const char *url, unsigned int allowed_types, void * payload){
    (void)url;
    (void)allowed_types;
    (void)payload;

    if(!(cred && username)){
        return __err_new(-1, "param<cred, username> can't be nil", nil);
    }

    const char *home_path;
    if(nil == (home_path = getenv("HOME"))){
        return __err_new(-1, "$HOME invalid", nil);
    }

    char pubkey_path[strlen(home_path) + sizeof("/.ssh/id_rsa.pub")];
    char prvkey_path[strlen(home_path) + sizeof("/.ssh/id_rsa")];

    sprintf(pubkey_path, "%s%s", home_path, "/.ssh/id_rsa.pub");
    sprintf(prvkey_path, "%s%s", home_path, "/.ssh/id_rsa");

    if(0 > git_cred_ssh_key_new(cred, username, pubkey_path, prvkey_path, nil)){
        return __err_new_git();
    }

    return nil;
}

/*
 * 用于拉取远程仓库代码，效果相当于 git fetch origin，但不执行 git merge origin/xxxBranch：
 *     仅更新到 refs/remotes/origin/master，不合并到本地分支
 * zpRemoteRepoAddr 参数必须是 路径/.git 或 URL/仓库名.git 或 bare repo 的格式
 */
_i
zgit_remote_fetch(git_repository *zpRepo, char *zpRemoteRepoAddr, char **zppRefs, _i zRefsCnt, char *zpErrBufOUT/* size: 256 */) {
    /* get the remote */
    git_remote* zRemote = nil;
    //git_remote_lookup( &zRemote, zpRepo, "origin" );  // 使用已命名分支时，调用此函数
    if (0 != git_remote_create_anonymous(&zRemote, zpRepo, zpRemoteRepoAddr)) {  // 直接使用 URL 时调用此函数
        if (nil == zpErrBufOUT) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "[zgit_remote_fetch]:error without message" : giterr_last()->message);
        } else {
            strncpy(zpErrBufOUT, nil == giterr_last() ? "[zgit_remote_fetch]:error without message" : giterr_last()->message, 255);
            zpErrBufOUT[255] = '\0';
        }

        return -1;
    };

    /* connect to remote */
    git_remote_callbacks zConnOpts;  // = GIT_REMOTE_CALLBACKS_INIT;
    git_remote_init_callbacks(&zConnOpts, GIT_REMOTE_CALLBACKS_VERSION);
    zConnOpts.credentials = zgit_cred_acquire_cb;  // 指定身份认证所用的回调函数

    if (0 != git_remote_connect(zRemote, GIT_DIRECTION_FETCH, &zConnOpts, nil, nil)) {
        git_remote_free(zRemote);
        if (nil == zpErrBufOUT) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "[zgit_remote_fetch]:error without message" : giterr_last()->message);
        } else {
            strncpy(zpErrBufOUT, nil == giterr_last() ? "[zgit_remote_fetch]:error without message" : giterr_last()->message, 255);
            zpErrBufOUT[255] = '\0';
        }

        return -1;
    }

    /* add [a] fetch refspec[s] */
    git_strarray zGitRefsArray;
    zGitRefsArray.strings = zppRefs;
    zGitRefsArray.count = zRefsCnt;

    git_fetch_options zFetchOpts;  // = GIT_FETCH_OPTIONS_INIT;
    git_fetch_init_options(&zFetchOpts, GIT_FETCH_OPTIONS_VERSION);

    /* do the fetch */
    if (0 != git_remote_fetch(zRemote, &zGitRefsArray, &zFetchOpts, "fetch")) {
        git_remote_disconnect(zRemote);
        git_remote_free(zRemote);
        if (nil == zpErrBufOUT) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "[zgit_remote_fetch]:error without message" : giterr_last()->message);
        } else {
            strncpy(zpErrBufOUT, nil == giterr_last() ? "[zgit_remote_fetch]:error without message" : giterr_last()->message, 255);
            zpErrBufOUT[255] = '\0';
        }

        return -1;
    }

    git_remote_disconnect(zRemote);
    git_remote_free(zRemote);
    return 0;
}

/*
 * [ git push ]
 * zpRemoteRepoAddr 参数必须是 路径/.git 或 URL/仓库名.git 或 bare repo 的格式
 */
    /*
     * << 错误类型 >>
     * err1 bit[0]:服务端错误
     * err2 bit[1]:网络不通
     * err3 bit[2]:SSH 连接认证失败
     * err4 bit[3]:目标端磁盘容量不足
     * err5 bit[4]:目标端权限不足
     * err6 bit[5]:目标端文件冲突
     * err7 bit[6]:目标端布署后动作执行失败
     * err8 bit[7]:目标端收到重复布署指令(同一目标机的多个不同IP)
     */
static _i
zgit_remote_push(git_repository *zpRepo, char *zpRemoteRepoAddr, char **zppRefs, _i zRefsCnt, char *zpErrBufOUT/* size: 256 */) {
    _i zErrNo = 0;
    /* get the remote */
    git_remote* zRemote = nil;
    //git_remote_lookup( &zRemote, zRepo, "origin" );  // 使用已命名分支时，调用此函数
    if (0 != git_remote_create_anonymous(&zRemote, zpRepo, zpRemoteRepoAddr)) {  // 直接使用 URL 时调用此函数
        if (nil == zpErrBufOUT) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "[zgit_remote_push]:error without message" : giterr_last()->message);
        } else {
            strncpy(zpErrBufOUT, nil == giterr_last() ? "[zgit_remote_push]:error without message" : giterr_last()->message, 255);
            zpErrBufOUT[255] = '\0';
        }

        zErrNo = -1;
        goto zEndMark;
    };

    /* connect to remote */
    git_remote_callbacks zConnOpts;  // = GIT_REMOTE_CALLBACKS_INIT;
    git_remote_init_callbacks(&zConnOpts, GIT_REMOTE_CALLBACKS_VERSION);
    zConnOpts.credentials = zgit_cred_acquire_cb;  // 指定身份认证所用的回调函数

    if (0 != git_remote_connect(zRemote, GIT_DIRECTION_PUSH, &zConnOpts, nil, nil)) {
        git_remote_free(zRemote);
        if (nil == zpErrBufOUT) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "[zgit_remote_push]:error without message" : giterr_last()->message);
        } else {
            strncpy(zpErrBufOUT, nil == giterr_last() ? "[zgit_remote_push]:error without message" : giterr_last()->message, 255);
            zpErrBufOUT[255] = '\0';
        }

        zErrNo = -2;  /* 网络不通 */
        goto zEndMark;
    }

    /* add [a] push refspec[s] */
    git_strarray zGitRefsArray;
    zGitRefsArray.strings = zppRefs;
    zGitRefsArray.count = zRefsCnt;

    git_push_options zPushOpts;  // = GIT_PUSH_OPTIONS_INIT;
    git_push_init_options(&zPushOpts, GIT_PUSH_OPTIONS_VERSION);
    zPushOpts.pb_parallelism = 1;  // 限定单个 push 动作可以使用的线程数，若指定为 0，则将与本地的CPU数量相同

    /* do the push */
    if (0 != (zErrNo = git_remote_upload(zRemote, &zGitRefsArray, &zPushOpts))) {
        git_remote_disconnect(zRemote);
        git_remote_free(zRemote);
        if (nil == zpErrBufOUT) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "[zgit_remote_push]:error without message" : giterr_last()->message);
        } else {
            strncpy(zpErrBufOUT, nil == giterr_last() ? "[zgit_remote_push]:error without message" : giterr_last()->message, 255);
            zpErrBufOUT[255] = '\0';
        }

        /*
         * 更改了 libgit2 中 git_push_finish() 的源码
         * 远程代码无法展开的情况，通常代表目标机磁盘已满
         * 此时会返回定制的错误码 -40000
         */
        if (-40000 == zErrNo) {
            zErrNo = -4;
        } else {
            zErrNo = -1;
        }

        goto zEndMark;
    }

    /* 同步 TAGS 之类的信息 */
    // if (0 != git_remote_update_tips(zRemote, &zConnOpts, 0, 0, nil)) {
    //     git_remote_disconnect(zRemote);
    //     git_remote_free(zRemote);
    //    strncpy(zpErrBufOUT, nil == giterr_last() ? "[zgit_remote_push]:error without message" : giterr_last()->message, 255);
    //    zpErrBufOUT[255] = '\0';
    //     return -1;
    // }

    git_remote_disconnect(zRemote);
    git_remote_free(zRemote);

zEndMark:
    return zErrNo;
}

/*
 * [ git log --format=%H ] and [ git log --format=%ct ]
 * success return zpRevWalker, fail return nil
 */
static zGitRevWalk__ *
zgit_generate_revwalker(git_repository *zpRepo, char *zpRef, _i zSortMode) {
    git_object *zpObj;
    git_revwalk *zpRevWalker = nil;

    if (0 != git_revwalk_new(&zpRevWalker, zpRepo)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return nil;
    }

    /* zSortType 显示順序：[0] git 默认排序，新记录在上、[1]逆序，旧记录在上 */
    if (0 == zSortMode) {
        zSortMode = GIT_SORT_TIME;
    } else {
        zSortMode = GIT_SORT_TIME|GIT_SORT_REVERSE;
    }

    git_revwalk_sorting(zpRevWalker, zSortMode);

    if ((0 != git_revparse_single(&zpObj, zpRepo, zpRef))
            || (0 != git_revwalk_push(zpRevWalker, git_object_id(zpObj)))) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        zgit_destroy_revwalker(zpRevWalker);
        return nil;
    }

    return zpRevWalker;
}

static void
zgit_destroy_revwalker(git_revwalk *zpRevWalker) {
    if (nil != zpRevWalker) {
        git_revwalk_free(zpRevWalker);
    }
}

/*
 * 结果返回：正常返回时间戳，若返回 0 表示已取完所有记录，返回 -1 表示出错
 * 参数返回：git log --format="%H" 格式的单条数据
 * 用途：每调用一次取出一条数据
 */
static _i
zgit_get_one_commitsig_and_timestamp(char *zpRevSigOUT, git_repository *zpRepo, git_revwalk *zpRevWalker) {
    git_oid zOid;
    git_commit *zpCommit = nil;
    _i zErrNo = 0;
    time_t zTimeStamp = 0;

    zErrNo = git_revwalk_next(&zOid, zpRevWalker);

    if (0 == zErrNo) {
        if (0 != git_commit_lookup(&zpCommit, zpRepo, &zOid)) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
            return -1;
        }

        /* 取完整的 40 位 SHA1 sig，第二个参数指定调用者传入的缓冲区大小 */
        if ('\0' == git_oid_tostr(zpRevSigOUT, 41, &zOid)[0]) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
            git_commit_free(zpCommit);
            return -1;
        }

        zTimeStamp = git_commit_time(zpCommit);
        git_commit_free(zpCommit);
        return zTimeStamp;
    } else if (GIT_ITEROVER == zErrNo) {
        return 0;
    } else {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }
}


/*
 * [ TEST: PASS ]
 * 创建新分支，若 zForceMark 指定为 true，则将覆盖已存在的同名分支
 * @zpBranchName 将要创建的目标分支名称
 * @zpBaseRev 可以指定为 “HEAD”、nil、refs/heads/xxx  或者具体的 40 位 SHA1 commitSig
 */
static _i
zgit_branch_add_local(git_repository *zpRepo, char *zpBranchName, char *zpBaseRev, zbool_t zForceMark) {
    git_reference *zpNewBranch = nil;
    git_commit *zpCommit = nil;
    git_oid zBaseRefOid;

    /* 获取基准版本 BaseRev 的 Oid */
    if (nil == zpBaseRev || 0 == strcmp("HEAD", zpBaseRev)) {
        if (0 != git_reference_name_to_id(&zBaseRefOid, zpRepo, "HEAD")) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
            return -1;
        }
    } else if (0 == strncmp("refs/", zpBaseRev, 5)) {
        if (0 != git_reference_name_to_id(&zBaseRefOid, zpRepo, zpBaseRev)) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
            return -1;
        }
    } else {
        if (0 != git_oid_fromstr(& zBaseRefOid, zpBaseRev)) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
            return -1;
        }
    }

    /*
     * 获取 HEAD 的最后一次 commit
     * 或者指定的具体的 commit
     */
    if (0 != git_commit_lookup(&zpCommit, zpRepo, &zBaseRefOid)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    /* 基于 HEAD 的最后一次 commit 创建新分支 */
    if (0 != git_branch_create(&zpNewBranch, zpRepo, zpBranchName, zpCommit, zForceMark)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);

        git_commit_free(zpCommit);
        return -1;
    }

    git_commit_free(zpCommit);
    git_reference_free(zpNewBranch);

    return 0;
}


/*
 * [ TEST: PASS ]
 * 删除分支
 */
static _i
zgit_branch_del_local(git_repository *zpRepo, char *zpBranchName) {
    git_reference *zpBranch = nil;

    if (0 != git_branch_lookup(&zpBranch, zpRepo, zpBranchName, GIT_BRANCH_LOCAL)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    if (nil == zpBranch) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_reference_free(zpBranch);
        return -1;
    }

    // if (git_branch_is_head(zpBranch)) {
    //     zPRINT_ERR_EASY("Delete HEAD will failed!");
    //     git_reference_free(zpBranch);
    //     return -1;
    // }

    if (0 != git_branch_delete(zpBranch)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_reference_free(zpBranch);
        return -1;
    }

    git_reference_free(zpBranch);

    return 0;
}


/*
 * 分支改名
 */
static _i
zgit_branch_rename_local(git_repository *zpRepo, char *zpOldName, char *zpNewName, zbool_t zForceMark) {
    git_reference *zpOld = nil;
    git_reference *zpNew = nil;

    if (0 != git_branch_lookup(&zpOld, zpRepo, zpOldName, GIT_BRANCH_LOCAL)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    if (nil == zpOld) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_reference_free(zpOld);
        return -1;
    }

    if (0 != git_branch_move(&zpNew, zpOld, zpNewName, zForceMark)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_reference_free(zpOld);
        return -1;
    }

    git_reference_free(zpOld);
    git_reference_free(zpNew);

    return 0;
}


/*
 * 切换分支
 */
static _i
zgit_branch_switch_local(git_repository *zpRepo, char *zpBranchName) {
    git_reference *zpBranch = nil;

    if (0 != git_branch_lookup(&zpBranch, zpRepo, zpBranchName, GIT_BRANCH_LOCAL)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    if (nil == zpBranch) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_reference_free(zpBranch);
        return -1;
    }

    if (0 != git_repository_set_head(zpRepo, git_reference_name(zpBranch))) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_reference_free(zpBranch);
        return -1;
    }

    git_reference_free(zpBranch);

    return 0;
}


/*
 * 将所有本地分支名称写出到传入的 zpResBufOUT 中
 * zBufLen 指定可写出的缓冲区总大小
 * 分支总数写入到 zpResItemCnt 所指向的整数
 * 若缓冲区大小不足，写出的内容将会不完整，但分支数量将是完整的
 */
static _i
zgit_branch_list_local(git_repository *zpRepo, char *zpResBufOUT, _i zBufLen, _i *zpResItemCnt) {
    git_branch_iterator *zpBranchIter = nil;
    git_reference *zpTmpBranch = nil;
    git_branch_t zBranchTypeOUT;
    const char *zpBranchName = nil;

    if (0 != git_branch_iterator_new(&zpBranchIter, zpRepo, GIT_BRANCH_LOCAL)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    _i zOffSet = 0;
    while (GIT_ITEROVER != git_branch_next(&zpTmpBranch, &zBranchTypeOUT, zpBranchIter)
            && zOffSet < zBufLen) {
        (* zpResItemCnt)++;

        git_branch_name(&zpBranchName, zpTmpBranch);  /* 迭代出的分支信息不会出错，此处不必检查返回值 */
        zOffSet += snprintf(zpResBufOUT + zOffSet + 1, zBufLen - zOffSet - 1, "%s", zpBranchName);
    }

    git_branch_iterator_free(zpBranchIter);

    return 0;
}


/*
 * [ TEST: PASS ]
 * git init .
 * zIsBare 置为 1/zTrue，则会初始化成 bare 库；否则就是普通库
 */
static _i
zgit_init(git_repository **zppRepoOUT, const char *zpPath, zbool_t zIsBare) {
    _i zErrNo = 0;

    /**
     * 在进行任何其它操作之前，必须首先调用 git_libgit2_init()
     * 此处要使用 0 > ... 作为条件
     */
    if (0 > (zErrNo = git_libgit2_init())) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
    } else {
        zErrNo = 0;
        if (0 != (zErrNo = git_repository_init(zppRepoOUT, zpPath, zIsBare))) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
            git_libgit2_shutdown();
        }
    }

    return zErrNo;
}


/*
 * [ TEST: PASS ]
 * git clone URL/.git
 * @zpBranchName: clone 后使用的默认分支，置为 nil 表示使用远程库的默认分支
 * @zIsBare 为 1/zTrue 表示要生成 bare 库，否则为普通带工作区的库
 */
static _i
zgit_clone_cb_repo_init(git_repository **zppRepoOUT,
        const char *zpPath, _i zIsBare, void *zpPayLoad __attribute__ ((__unused__))) {
    return zgit_init(zppRepoOUT, zpPath, zIsBare);
}

static _i
zgit_clone(git_repository **zppRepoOUT, char *zpRepoAddr, char *zpPath, char *zpBranchName, zbool_t zIsBare) {
    _i zErrNo = 0;

    git_clone_options zOpt;  // = GIT_CLONE_OPTIONS_INIT;
    git_clone_init_options(&zOpt, GIT_CLONE_OPTIONS_VERSION);

    zOpt.fetch_opts.callbacks.credentials = zgit_cred_acquire_cb;
    zOpt.bare = (zFalse == zIsBare) ? 0 : 1;
    zOpt.repository_cb = zgit_clone_cb_repo_init;

    if (nil != zpBranchName) {
        zOpt.checkout_branch = zpBranchName;
    }

    if (0 != (zErrNo = git_clone(zppRepoOUT, zpRepoAddr, zpPath, &zOpt))) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
    }

    return zErrNo;
}


/*
 * [ TEST: PASS ]
 * 提交文件至指定分支
 * 若分支不存在，将基于当前 HEAD 自动创建一个新分支
 * [@] 路径名称必须是相对于 git 库根路径的
 * @param zpRefName 除非使用 “HEAD”，否则其名称必须完整，如：refs/heads/xxx
 *
 * 功能等同于如下命令的组合：
 *     git config user.name _
 *     && git config user.email _
 *     && git add --all /PATH
 *     && git commit --allow-empty -m "_"
 */
static _i
zgit_add_and_commit(git_repository *zpRepo,
        char *zpRefName,
        char *zpPath,
        char *zpMsg) {

    _i zErrNo = 0;
    struct stat zS_;
    zCHECK_NEGATIVE_RETURN(stat(zpPath, &zS_), -1);

    git_index* zpIndex = nil;

    git_commit *zpParentCommit = nil;

    git_oid zTreeOid;
    git_tree *zpTree = nil;

    git_oid zParentOid;
    const git_commit *zppParentCommit[1] = { nil };
    _i zParentCnt = 0;

    git_signature *zpMe = nil;

    git_oid zCommitOid;

    /* get index */
    if (0 != git_repository_index(&zpIndex, zpRepo)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    /* git add file */
    if (S_ISDIR(zS_.st_mode)) {
        git_strarray zPaths;
        zPaths.strings = &zpPath;
        zPaths.count = 1;

        zErrNo = git_index_add_all(zpIndex, &zPaths, GIT_INDEX_ADD_FORCE, nil, nil);
    } else {
        zErrNo = git_index_add_bypath(zpIndex, zpPath);
    }

    if (0 != zErrNo) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_index_free(zpIndex);
        return -1;
    }

    /* Write the in-memory index to disk */
    if (0 != git_index_write(zpIndex)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_index_free(zpIndex);
        return -1;
    }

    /*
     * 获取父节点的 CommitOid
     * 无父节点将以指定的 RefName 创建新分支
     */
    if (0 == (zErrNo = git_reference_name_to_id(&zParentOid, zpRepo, zpRefName))) {
        if (0 != git_commit_lookup(&zpParentCommit, zpRepo, &zParentOid)) {
            zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
            git_index_free(zpIndex);
            return -1;
        }

        zParentCnt = 1;
        zppParentCommit[0] = zpParentCommit;
    } else if (GIT_ENOTFOUND == zErrNo) {
        /* branch not exist(will be created), do nothing... */
    } else {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_index_free(zpIndex);
        return -1;
    }

    /* 以提交到工作区的内容，创建一棵子树，并写出 TreeOid */
    if (0 != git_index_write_tree(&zTreeOid, zpIndex)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        if (nil != zpParentCommit) {
            git_commit_free(zpParentCommit);
        }
        git_index_free(zpIndex);
        return -1;
    }

    /* 以生成的 TreeOid 找出对应的树对象 */
    if (0 != git_tree_lookup(&zpTree, zpRepo, &zTreeOid)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        if (nil != zpParentCommit) {
            git_commit_free(zpParentCommit);
        }
        git_index_free(zpIndex);
        return -1;
    }

    /* 生成临时签名 */
    if (0 != git_signature_now(&zpMe, "_", "_@_")) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_tree_free(zpTree);
        if (nil != zpParentCommit) {
            git_commit_free(zpParentCommit);
        }
        git_index_free(zpIndex);
        return -1;
    }

    /* 将新生成的树对象、父节点 CommitID 联系起来，写到库中 */
    if (0 != git_commit_create(&zCommitOid, zpRepo, zpRefName, zpMe, zpMe, "UTF-8", zpMsg, zpTree, zParentCnt, zppParentCommit)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_signature_free(zpMe);
        git_tree_free(zpTree);
        if (nil != zpParentCommit) {
            git_commit_free(zpParentCommit);
        }
        git_index_free(zpIndex);
        return -1;
    }

    /* clean... */
    git_index_free(zpIndex);
    if (nil != zpParentCommit) {
        git_commit_free(zpParentCommit);
    }
    git_tree_free(zpTree);
    git_signature_free(zpMe);

    return 0;
}


/*
 * [ TEST: PASS ]
 * @param zpRepoPath 源库路径
 * 功能等同于：git config user.name = "_" && git config user.email = "_@_"
 */
static _i
zgit_config_name_and_email(char *zpRepoPath) {
    char zConfPathBuf[strlen(zpRepoPath) + sizeof("/.git/config")];
    sprintf(zConfPathBuf, "%s/.git/config", zpRepoPath);

    git_config *zpConf = nil;
    if (0 != git_config_open_ondisk(&zpConf, zConfPathBuf)) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    if (0 != git_config_set_string(zpConf, "user.name", "_")) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_config_free(zpConf);
        return -1;
    }

    if (0 != git_config_set_string(zpConf, "user.email", "_@_")) {
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_config_free(zpConf);
        return -1;
    }

    git_config_free(zpConf);
    return 0;
}


// /*
//  * TO DO...
//  */
// void
// zgit_diff_path_list(git_repository *zpRepo, char *zpRef0, char *zpRef1) {
//     git_object *zpObj;
//     git_tree *zpTree[2];
//     git_diff *zpDiff;
//     git_diff_options zDiffOpts;
//     git_diff_find_options zDiffFindOpts;
//     git_diff_format_t zDiffFormat = GIT_DIFF_FORMAT_NAME_ONLY;
//
//     git_diff_init_options(&zDiffOpts, GIT_DIFF_OPTIONS_VERSION);
//     git_diff_find_init_options(&zDiffFindOpts, GIT_DIFF_FIND_OPTIONS_VERSION);
//     zDiffFindOpts.flags |= GIT_DIFF_FIND_ALL;
//
//     git_revparse_single(&zpObj, zpRepo, zpRef0);
//     git_object_peel((git_object **) zpTree, zpObj, GIT_OBJ_TREE);
//     git_object_free(zpObj);
//
//     git_revparse_single(&zpObj, zpRepo, zpRef1);
//     git_object_peel((git_object **) zpTree, zpObj, GIT_OBJ_TREE);
//     git_object_free(zpObj);
//
//     git_diff_tree_to_tree(&zpDiff, zpRepo, zpTree[0], zpTree[1], &zDiffOpts);
//     git_diff_find_similar(zpDiff, &zDiffFindOpts);
//
//     git_diff_print(zpDiff, zDiffFormat, nil, nil);  // the last two nil param：diff res ops and it's inner param ptr
//
//     git_diff_free(zpDiff);
//     git_tree_free(zpTree[0]);
//     git_tree_free(zpTree[1]);
//
//     // 参见 log.c diff.c 实现 git log --format=%H、git diff --name-only、git diff -- filepath_0 filepath_N
//     // 可以反向显示提交记录
//     // 优化生成缓存的相关的函数实现
// }
//
// /*
//  * TO DO...
//  */
// void
// zgit_diff_onefile(char *zpFilePath) {
//     git_diff_options zDiffOpts;
//     git_diff_init_options(&zDiffOpts, GIT_DIFF_OPTIONS_VERSION);
//     zDiffOpts.pathspec.strings = &zpFilePath;
//     zDiffOpts.pathspec.count = 1;
//
//     //git_pathspec_new(nil, nil);
// }
