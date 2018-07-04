#include "utils.h"
#include "git2.h"

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define __err_new_git() __err_new(-1, nil == giterr_last() ? "error without message" : giterr_last()->message, nil)

/*
 * **************
 * Env Management
 * **************
 */

static Error *
global_env_init(void){
    if(0 > git_libgit2_init()){
        return __err_new_git();
    }
    return nil;
}

inline static void
global_env_clean(void){
    git_libgit2_shutdown();
}

//**can't be used to init bare repo**
//**if success, hdr has been opened**
//@param hdr[out]:
//@param path[in]:
static Error *
repo_init(git_repository **hdr, const char *path){
    _i rv;
    if(0 > (rv = git_repository_init(hdr, path, 0))){
        return __err_new_git();
    }
    return nil;
}

//like：git config user.name "_". eg. ...
//@param repo_path[in]:
//@param kv[in]: key/value
//@param kv_n[in]: cnt of kv
static Error *
info_config(char *repo_path, char *(*kv[2]), _i kv_n){
    char confpath[strlen(repo_path) + sizeof("/.git/config")];
    sprintf(confpath, "%s/.git/config", repo_path);

    git_config *conf;
    if(0 > git_config_open_ondisk(&conf, confpath)){
        return __err_new_git();
    }

    _i i;
    for(i = 0; i< kv_n; ++i){
        if(0 > git_config_set_string(conf, kv[i][0], kv[i][1])){
            git_config_free(conf);
            return __err_new_git();
        }
    }

    git_config_free(conf);
    return nil;
}

//@param hdr[out]: git_repo handler
//@param addr[in]: path/.git OR url/xxx.git
static Error *
repo_open(git_repository **hdr, const char *addr){
    if(0 > git_repository_open(hdr, addr)){
        *hdr = nil;
        return __err_new_git();
    }
    return nil;
}

//@param hdr[in]:
static void
repo_close(git_repository *hdr){
    git_repository_free(hdr);
}

/*
 * **************
 * Remote Management
 * **************
 */

//**libgit2: origin cb**
//@param cred[out]:
//@param username[in]:
//@param url[in]:
//@param allowed_types[in]:
//@param payload[in]:
static _i
cred_cb(git_cred **cred, const char * username,
        const char *url, unsigned int allowed_types, void * payload){
    (void)url;
    (void)allowed_types;
    (void)payload;

    if(!(cred && username)){
        __info("param<cred, username> can't be nil");
        return -1;
    }

    const char *home_path;
    if(nil == (home_path = getenv("HOME"))){
        __info("$HOME invalid");
        return -1;
    }

    char pubkey_path[strlen(home_path) + sizeof("/.ssh/id_rsa.pub")];
    char prvkey_path[strlen(home_path) + sizeof("/.ssh/id_rsa")];

    sprintf(pubkey_path, "%s%s", home_path, "/.ssh/id_rsa.pub");
    sprintf(prvkey_path, "%s%s", home_path, "/.ssh/id_rsa");

    if(0 > git_cred_ssh_key_new(cred, username, pubkey_path, prvkey_path, nil)){
        __info(nil == giterr_last() ? "error without message" : giterr_last()->message);
        return -1;
    }

    return 0;
}

//**libgit2: origin cb**
//@param hdr[out]:
//@param path[in]:
//@param isbare[in]:
//@param payload[in]:
static _i
clone_cb(git_repository **hdr, const char *path,
        _i isbare, void *payload){
    (void)isbare;
    (void)payload;

    Error *e;
    if(nil == (e = repo_init(hdr, path))){
        return 0;
    }else{
        __display_and_clean(e);
        return -1;
    }
}

//@param hdr[out]:
//@param repo_addr[in]: source repo url OR path
//@param local_path[in]:
static Error *
clone(git_repository **hdr, char *repo_addr, char *local_path){
    git_clone_options opt; // = GIT_CLONE_OPTIONS_INIT;
    git_clone_init_options(&opt, GIT_CLONE_OPTIONS_VERSION);

    opt.bare = 0;
    opt.fetch_opts.callbacks.credentials = cred_cb;
    opt.repository_cb = clone_cb;

    if(0 != git_clone(hdr, repo_addr, local_path, &opt)){
        return __err_new_git();
    }

    return nil;
}

static void
git_remote_drop(git_remote **remote_hdr){
    if(nil != *remote_hdr){
        git_remote_disconnect(*remote_hdr);
        git_remote_free(*remote_hdr);
    }
}

//** like shell cmd: git fetch **
//@param repo_hdr[in]: repo hdr
//@param addr[in]: source repo addr, a remote url OR a local path
//@param refs[in]: branchs to fetch
//@param refscnt[in]: cnt of refs
static Error *
fetch(git_repository *repo_hdr, char *addr, char **refs, _i refscnt){
    __drop(git_remote_drop) git_remote* remote_hdr = nil;

    if(0 > git_remote_create_anonymous(&remote_hdr, repo_hdr, addr)){
    //if(0 > git_remote_lookup(&remote_hdr, repo_hdr, "origin")){ //call this func, when deal with named branch
        return __err_new_git();
    };

    /* connect to remote */
    git_remote_callbacks opt;  // = GIT_REMOTE_CALLBACKS_INIT;
    git_remote_init_callbacks(&opt, GIT_REMOTE_CALLBACKS_VERSION);
    opt.credentials = cred_cb;

    if(0 > git_remote_connect(remote_hdr, GIT_DIRECTION_FETCH, &opt, nil, nil)){
        return __err_new_git();
    }

    git_strarray refs_array;
    refs_array.strings = refs;
    refs_array.count = refscnt;

    git_fetch_options zFetchOpts;  // = GIT_FETCH_OPTIONS_INIT;
    git_fetch_init_options(&zFetchOpts, GIT_FETCH_OPTIONS_VERSION);

    /* do the fetch */
    if(0 != git_remote_fetch(remote_hdr, &refs_array, &zFetchOpts, "fetch")){
        return __err_new_git();
    }

    return nil;
}

//** like shell cmd: git push **
//@param repo_hdr[in]: repo hdr
//@param addr[in]: source repo addr, a remote url OR a local path
//@param refs[in]: branchs to fetch
//@param refscnt[in]: cnt of refs
static Error *
push(git_repository *repo_hdr, char *addr, char **refs, _i refscnt){
    __drop(git_remote_drop) git_remote* remote_hdr = nil;

    if(0 > git_remote_create_anonymous(&remote_hdr, repo_hdr, addr)){
    //if(0 > git_remote_lookup(&remote_hdr, repo_hdr, "origin")){ //call this func, when deal with named branch
        return __err_new_git();
    };

    /* connect to remote */
    git_remote_callbacks opt;  // = GIT_REMOTE_CALLBACKS_INIT;
    git_remote_init_callbacks(&opt, GIT_REMOTE_CALLBACKS_VERSION);
    opt.credentials = cred_cb;

    if(0 > git_remote_connect(remote_hdr, GIT_DIRECTION_PUSH, &opt, nil, nil)){
        __err_new_git();
    }

    git_strarray refs_array;
    refs_array.strings = refs;
    refs_array.count = refscnt;

    git_push_options zPushOpts;  // = GIT_PUSH_OPTIONS_INIT;
    git_push_init_options(&zPushOpts, GIT_PUSH_OPTIONS_VERSION);
    zPushOpts.pb_parallelism = 1; //max threads to use in one push_ops

    /* do the push */
    if(0 > git_remote_upload(remote_hdr, &refs_array, &zPushOpts)){
        return __err_new_git();
    }

    return nil;
}

/*
 * **************
 * Branch Management
 * **************
 */

//@param repo_hdr[in]:
//@param branch_name[in]: new branch name to create
//@param base_rev[int]: "HEAD" OR nil OR refs/heads/<refname> OR sha1_str<40bytes + '\0'>
static Error *
add_branch_local(git_repository *repo_hdr, char *branch_name, char *base_rev){
    git_reference *newbranch = nil;
    git_commit *commit = nil;
    git_oid baseoid;

    //get base_rev's oid
    if(nil == base_rev || 0 == strcmp("HEAD", base_rev)){
        if(0 > git_reference_name_to_id(&baseoid, repo_hdr, "HEAD")){
            return __err_new_git();
        }
    }else if(0 == strncmp("refs/", base_rev, 5)){
        if(0 > git_reference_name_to_id(&baseoid, repo_hdr, base_rev)){
            return __err_new_git();
        }
    }else{
        if(0 > git_oid_fromstr(&baseoid, base_rev)){
            return __err_new_git();
        }
    }

    //get base_rev's last commit
    if(0 > git_commit_lookup(&commit, repo_hdr, &baseoid)){
        return __err_new_git();
    }

    //create new branch
    if(0 > git_branch_create(&newbranch, repo_hdr, branch_name, commit, 0)){
        git_commit_free(commit);
        return __err_new_git();
    }

    git_commit_free(commit);
    git_reference_free(newbranch);

    return nil;
}


/*
 * [ TEST: PASS ]
 * 删除分支
 */
static _i
zgit_branch_del_local(git_repository *repo_hdr, char *branch_name){
    git_reference *zpBranch = nil;

    if(0 != git_branch_lookup(&zpBranch, repo_hdr, branch_name, GIT_BRANCH_LOCAL)){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    if(nil == zpBranch){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_reference_free(zpBranch);
        return -1;
    }

    // if(git_branch_is_head(zpBranch)){
    //     zPRINT_ERR_EASY("Delete HEAD will failed!");
    //     git_reference_free(zpBranch);
    //     return -1;
    // }

    if(0 != git_branch_delete(zpBranch)){
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
zgit_branch_rename_local(git_repository *repo_hdr, char *zpOldName, char *zpNewName, zbool_t zForceMark){
    git_reference *zpOld = nil;
    git_reference *zpNew = nil;

    if(0 != git_branch_lookup(&zpOld, repo_hdr, zpOldName, GIT_BRANCH_LOCAL)){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    if(nil == zpOld){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_reference_free(zpOld);
        return -1;
    }

    if(0 != git_branch_move(&zpNew, zpOld, zpNewName, zForceMark)){
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
zgit_branch_switch_local(git_repository *repo_hdr, char *branch_name){
    git_reference *zpBranch = nil;

    if(0 != git_branch_lookup(&zpBranch, repo_hdr, branch_name, GIT_BRANCH_LOCAL)){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    if(nil == zpBranch){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_reference_free(zpBranch);
        return -1;
    }

    if(0 != git_repository_set_head(repo_hdr, git_reference_name(zpBranch))){
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
zgit_branch_list_local(git_repository *repo_hdr, char *zpResBufOUT, _i zBufLen, _i *zpResItemCnt){
    git_branch_iterator *zpBranchIter = nil;
    git_reference *zpTmpBranch = nil;
    git_branch_t zBranchTypeOUT;
    const char *branch_name = nil;

    if(0 != git_branch_iterator_new(&zpBranchIter, repo_hdr, GIT_BRANCH_LOCAL)){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    _i zOffSet = 0;
    while (GIT_ITEROVER != git_branch_next(&zpTmpBranch, &zBranchTypeOUT, zpBranchIter)
            && zOffSet < zBufLen){
        (* zpResItemCnt)++;

        git_branch_name(&branch_name, zpTmpBranch);  /* 迭代出的分支信息不会出错，此处不必检查返回值 */
        zOffSet += snprintf(zpResBufOUT + zOffSet + 1, zBufLen - zOffSet - 1, "%s", branch_name);
    }

    git_branch_iterator_free(zpBranchIter);

    return 0;
}
/*
 * **************
 * Commit Management
 * **************
 */

inline static void
free_walker(git_revwalk *walker, git_object *obj){
    git_object_free(obj);
    git_revwalk_free(walker);
}

//@param walker[out]:
//@param obj[out]: walker associated obj, need git_object_free(_)
//@param repo_hdr[in]:
//@param ref[in]: usually a branch name
//@param sort_mode[in]:
static Error *
get_walker(git_revwalk **walker, git_object **obj,
        git_repository *repo_hdr, char *ref, _i sort_mode){
    if(0 > git_revwalk_new(walker, repo_hdr)){
        return __err_new_git();
    }

    //sort_mode: 0 for git's default, other value for reverse sorting
    sort_mode = (0 == sort_mode) ? GIT_SORT_TIME : GIT_SORT_TIME|GIT_SORT_REVERSE;
    git_revwalk_sorting(*walker, sort_mode);

    if(0 > git_revparse_single(obj, repo_hdr, ref)){
        free_walker(*walker, *obj);
        return __err_new_git();
    }
    if(0 > git_revwalk_push(*walker, git_object_id(*obj))){
        free_walker(*walker, *obj);
        return __err_new_git();
    }

    return nil;
}

//**USAGE: keep call this func in a loop until nil == *revsig**
//@param commit[out]: commit hdr to get info from
//@param revsig[out]: sha1 40bytes revsig
//@param repo_hdr[in]:
//@param walker[in]:
static Error *
get_one_commit(git_commit **commit, char **revsig,
        git_repository *repo_hdr, git_revwalk *walker){
    git_oid oid;
    _i rv;

    *revsig = nil;
    if(0 == (rv = git_revwalk_next(&oid, walker))){
        if(0 > git_commit_lookup(commit, repo_hdr, &oid)){
            return __err_new_git();
        }else{
            //static memory space, per thread(when enable libgit2's THREADSAFE)
            *revsig = git_oid_tostr_s(&oid);
        }
    }else if(GIT_ITEROVER == rv){
        return nil;
    }else{
        return __err_new_git();
    }

    return nil;
}

static time_t
get_one_commit_ts(git_commit *commit){
    return git_commit_time(commit);
}

static const char *
get_one_commit_msg(git_commit *commit){
    return git_commit_message_raw(commit);
}

inline static void
free_commit(git_commit *commit){
    git_commit_free(commit);
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
zgit_add_and_commit(git_repository *repo_hdr,
        char *zpRefName,
        char *zpPath,
        char *zpMsg){

    _i rv = 0;
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
    if(0 != git_repository_index(&zpIndex, repo_hdr)){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        return -1;
    }

    /* git add file */
    if(S_ISDIR(zS_.st_mode)){
        git_strarray zPaths;
        zPaths.strings = &zpPath;
        zPaths.count = 1;

        rv = git_index_add_all(zpIndex, &zPaths, GIT_INDEX_ADD_FORCE, nil, nil);
    }else{
        rv = git_index_add_bypath(zpIndex, zpPath);
    }

    if(0 != rv){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_index_free(zpIndex);
        return -1;
    }

    /* Write the in-memory index to disk */
    if(0 != git_index_write(zpIndex)){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_index_free(zpIndex);
        return -1;
    }

    /*
     * 获取父节点的 CommitOid
     * 无父节点将以指定的 RefName 创建新分支
     */
    if(0 == (rv = git_reference_name_to_id(&zParentOid, repo_hdr, zpRefName))){
        if(0 != git_commit_lookup(&zpParentCommit, repo_hdr, &zParentOid)){
            zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
            git_index_free(zpIndex);
            return -1;
        }

        zParentCnt = 1;
        zppParentCommit[0] = zpParentCommit;
    }else if(GIT_ENOTFOUND == rv){
        /* branch not exist(will be created), do nothing... */
    }else{
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_index_free(zpIndex);
        return -1;
    }

    /* 以提交到工作区的内容，创建一棵子树，并写出 TreeOid */
    if(0 != git_index_write_tree(&zTreeOid, zpIndex)){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        if(nil != zpParentCommit){
            git_commit_free(zpParentCommit);
        }
        git_index_free(zpIndex);
        return -1;
    }

    /* 以生成的 TreeOid 找出对应的树对象 */
    if(0 != git_tree_lookup(&zpTree, repo_hdr, &zTreeOid)){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        if(nil != zpParentCommit){
            git_commit_free(zpParentCommit);
        }
        git_index_free(zpIndex);
        return -1;
    }

    /* 生成临时签名 */
    if(0 != git_signature_now(&zpMe, "_", "_@_")){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_tree_free(zpTree);
        if(nil != zpParentCommit){
            git_commit_free(zpParentCommit);
        }
        git_index_free(zpIndex);
        return -1;
    }

    /* 将新生成的树对象、父节点 CommitID 联系起来，写到库中 */
    if(0 != git_commit_create(&zCommitOid, repo_hdr, zpRefName, zpMe, zpMe, "UTF-8", zpMsg, zpTree, zParentCnt, zppParentCommit)){
        zPRINT_ERR_EASY(nil == giterr_last() ? "Error without message" : giterr_last()->message);
        git_signature_free(zpMe);
        git_tree_free(zpTree);
        if(nil != zpParentCommit){
            git_commit_free(zpParentCommit);
        }
        git_index_free(zpIndex);
        return -1;
    }

    /* clean... */
    git_index_free(zpIndex);
    if(nil != zpParentCommit){
        git_commit_free(zpParentCommit);
    }
    git_tree_free(zpTree);
    git_signature_free(zpMe);

    return 0;
}
