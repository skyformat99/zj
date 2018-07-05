/*
 * libgit2 库特点：
 *   - 其错误信息是混乱不可靠的，出现问题查看源码
 *   - 其接口入参，都会检查空值，调用时可不必重复检查
 *   - 本地推送至远端 HEAD 所在分支，会静默失败
 *   - 拉取远端内容至本地 HEAD 所在分支，会强制覆盖
 *   - **所有的分支管理函数，其 branchname 参数均不带 'refs/heads' 前缀
 */
#include "git.h"

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define __err_new_git() __err_new(-1, nil == giterr_last() ? "error without message" : giterr_last()->message, nil)

static Error *global_env_init(void) __mustuse;
static void global_env_clean(void);

static Error *repo_init(git_repository **hdr, const char *path) __mustuse;

static Error *info_config(char *(*kv[2]), _i kv_n) __mustuse;
static Error *get_cfg(const char *key, char **value);

static Error *repo_open(git_repository **hdr, const char *addr) __mustuse;
static void repo_close(git_repository *hdr);

static Error *clone(git_repository **hdr, const char *repo_addr, const char *local_path) __mustuse;
static Error *fetch(git_repository *hdr, const char *addr, char **refs, _i refscnt) __mustuse;
static Error *push(git_repository *hdr, const char *addr, char **refs, _i refscnt) __mustuse;

static Error *create_branch_local(git_repository *hdr, const char *branchname, const char *base_ref) __mustuse;
static Error *del_branch_local(git_repository *hdr, const char *branchname) __mustuse;
static Error *rename_branch_local(git_repository *hdr, const char *oldname, const char *newname) __mustuse;
static Error *switch_branch_local(git_repository *hdr, const char *branchname) __mustuse;

static Error *branch_exists(git_repository *hdr, const char *branchname, _i *res) __mustuse;
static Error *branch_is_head(git_repository *hdr, const char *branchname, _i *res) __mustuse;

static Error *gen_walker(git_revwalk **walker, git_object **obj, git_repository *hdr, const char *ref, _i sort_mode) __mustuse;
static void revwalker_free(git_revwalk *walker, git_object *obj);

static Error *get_commit(git_commit **commit, const char **revsig, git_repository *hdr, git_revwalk *walker) __mustuse;
static void commit_free(git_commit *commit);

static time_t get_commit_ts(git_commit *commit);
static const char *get_commit_msg(git_commit *commit) __mustuse;

static Error *do_commit(git_repository *hdr, const char *branchname, const char *msg) __mustuse;

struct Git git = {
    .env_init = global_env_init,
    .env_clean = global_env_clean,

    .repo_init = repo_init,

    .config = info_config,
    .get_cfg = get_cfg,

    .repo_open = repo_open,
    .repo_close = repo_close,

    .clone = clone,
    .fetch = fetch,
    .push = push,

    .create_branch = create_branch_local,
    .del_branch = del_branch_local,
    .rename_branch = rename_branch_local,
    .switch_branch = switch_branch_local,

    .branch_exists = branch_exists,
    .branch_is_head = branch_is_head,

    .gen_walker = gen_walker,
    .walker_free = revwalker_free,

    .get_commit = get_commit,
    .commit_free = commit_free,

    .get_commit_ts = get_commit_ts,
    .get_commit_msg = get_commit_msg,

    .do_commit = do_commit,
};

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

static void
global_env_clean(void){
    git_libgit2_shutdown();
}

//**can't be used to init bare repo**
//**if success, hdr has been opened**
//@param hdr[out]:
//@param path[in]:
static Error *
repo_init(git_repository **hdr, const char *path){
    __check_nil(hdr&&path);
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
info_config(char *(*kv[2]), _i kv_n){
    __check_nil(kv);
    git_config *conf;
    if(0 > git_config_open_default(&conf)){
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

//@param key[in]: key to query
//@param value[out]: value of key, MUST free(_)
static Error *
get_cfg(const char *key, char **value){
    __check_nil(key&&value);
    git_config *cfg;
    git_config_entry *v;

    if(0 > git_config_open_default(&cfg)){
        return __err_new_git();
    }
    if(0 > git_config_get_entry(&v, cfg, key)){
        return __err_new_git();
    }

    *value = strdup(v->value);

    v->free(v);
    git_config_free(cfg);
    return nil;
}

//@param hdr[out]: git_repo handler
//@param path[in]: repo path
static Error *
repo_open(git_repository **hdr, const char *path){
    __check_nil(hdr&&path);
    if(0 > git_repository_open(hdr, path)){
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
cred_cb(git_cred **cred, const char *url, const char *username, unsigned int allowed_types, void * payload){
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
clone_cb(git_repository **hdr, const char *path, _i isbare, void *payload){
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
clone(git_repository **hdr, const char *repo_addr, const char *local_path){
    __check_nil(hdr&&repo_addr&&local_path);
    git_clone_options opt; // = GIT_CLONE_OPTIONS_INIT;
    git_clone_init_options(&opt, GIT_CLONE_OPTIONS_VERSION);

    opt.bare = 0;
    opt.fetch_opts.callbacks.credentials = cred_cb;
    opt.repository_cb = clone_cb;

    if(0 > git_clone(hdr, repo_addr, local_path, &opt)){
        return __err_new_git();
    }

    return nil;
}

static void
git_remote_drop(git_remote **remote){
    git_remote_free(*remote);
}

//** like shell cmd: git fetch **
//@param hdr[in]: repo hdr
//@param url[in]: source repo addr, a remote url OR a local path
//@param refs[in]: branchs to fetch
//    !!!! form MUST be: "+refs/heads/master:refs/heads/test"
//@param refscnt[in]: cnt of refs
static Error *
fetch(git_repository *hdr, const char *url, char **refs, _i refscnt){
    __check_nil(hdr&&url&&refs);
    __drop(git_remote_drop) git_remote* remote = nil;

    if(0 > git_remote_create_anonymous(&remote, hdr, url)){
    //if(0 > git_remote_lookup(&remote, hdr, "origin")){ //call this func, when deal with named branch
        return __err_new_git();
    };

    git_strarray refs_array;
    refs_array.strings = refs;
    refs_array.count = refscnt;

    git_remote_callbacks remote_opt = GIT_REMOTE_CALLBACKS_INIT;
    remote_opt.credentials = cred_cb;

    git_fetch_options fetch_opt = GIT_FETCH_OPTIONS_INIT;
    fetch_opt.callbacks = remote_opt;

    //do the fetch
    if(0 > git_remote_fetch(remote, &refs_array, &fetch_opt, nil)){
        return __err_new_git();
    }

    return nil;
}

//** like shell cmd: git push **
//@param hdr[in]: repo hdr
//@param url[in]: source repo addr, a remote url OR a local path
//@param refs[in]: branchs to fetch
//    !!!! form MUST be: "+refs/heads/master:refs/heads/test"
//@param refscnt[in]: cnt of refs
static Error *
push(git_repository *hdr, const char *url, char **refs, _i refscnt){
    __check_nil(hdr&&url&&refs);
    __drop(git_remote_drop) git_remote* remote = nil;

    if(0 > git_remote_create_anonymous(&remote, hdr, url)){
    //if(0 > git_remote_lookup(&remote, hdr, "origin")){ //call this func, when deal with named branch
        return __err_new_git();
    };

    git_strarray refs_array;
    refs_array.strings = refs;
    refs_array.count = refscnt;

    git_remote_callbacks remote_opt = GIT_REMOTE_CALLBACKS_INIT;
    remote_opt.credentials = cred_cb;

    git_push_options push_opt = GIT_PUSH_OPTIONS_INIT;
    push_opt.pb_parallelism = 1; //max threads to use in one push_ops
    push_opt.callbacks = remote_opt;

    //do the push
    if(0 > git_remote_push(remote, &refs_array, &push_opt)){
        return __err_new_git();
    }

    return nil;
}

/*
 * **************
 * Branch Management
 * **************
 */

//@param hdr[in]:
//@param branchname[in]: branch name to check
//@param res[out]: 0 for false, 1 for true
static Error *
branch_exists(git_repository *hdr, const char *branchname, _bool *res){
    __check_nil(hdr&&branchname&&res);
    git_reference *branch;
    if(0 > (*res = git_branch_lookup(&branch, hdr, branchname, GIT_BRANCH_LOCAL))){
        if(GIT_ENOTFOUND == *res){
            *res = 0;
            return nil;
        }else{
            return __err_new_git();
        }
    }

    *res = 1;
    return nil;
}

//@param hdr[in]:
//@param branchname[in]: branch name to check
//@param res[out]: 0 for false, 1 for true
static Error *
branch_is_head(git_repository *hdr, const char *branchname, _bool *res){
    __check_nil(hdr&&branchname&&res);
    git_reference *branch;
    if(0 > (*res = git_branch_lookup(&branch, hdr, branchname, GIT_BRANCH_LOCAL))){
        return __err_new_git();
    }

    //return 0 if is NOT head, 1 if is head, or a value < 0
    if(0 > (*res = git_branch_is_head(branch))){
        return __err_new_git();
    }

    return nil;
}

//@param hdr[in]:
//@param branchname[in]: new branch name to create
//@param base_ref[int]: "HEAD" OR refs/heads/<refname> OR sha1_str<40bytes + '\0'>
static Error *
create_branch_local(git_repository *hdr, const char *branchname, const char *base_ref){
    __check_nil(hdr&&branchname&&base_ref);
    git_reference *newbranch;
    git_commit *commit;
    git_oid baseoid;

    //get base_ref's oid
    if(0 == strcmp("HEAD", base_ref)){
        if(0 > git_reference_name_to_id(&baseoid, hdr, "HEAD")){
            return __err_new_git();
        }
    }else if(0 == strncmp("refs/", base_ref, 5)){
        if(0 > git_reference_name_to_id(&baseoid, hdr, base_ref)){
            return __err_new_git();
        }
    }else{
        if(0 > git_oid_fromstr(&baseoid, base_ref)){
            return __err_new_git();
        }
    }

    //get base_ref's last commit
    if(0 > git_commit_lookup(&commit, hdr, &baseoid)){
        return __err_new_git();
    }

    //create new branch
    if(0 > git_branch_create(&newbranch, hdr, branchname, commit, 0)){
        git_commit_free(commit);
        return __err_new_git();
    }

    git_commit_free(commit);
    git_reference_free(newbranch);

    return nil;
}

//@param hdr[in]:
//@param branchname[in]: branch to delete
static Error *
del_branch_local(git_repository *hdr, const char *branchname){
    __check_nil(hdr&&branchname);
    git_reference *branch;
    _i rv;

    if(0 > (rv = git_branch_lookup(&branch, hdr, branchname, GIT_BRANCH_LOCAL))){
        if(GIT_ENOTFOUND == rv){
            return nil;
        }else{
            return __err_new_git();
        }
    }

    if(nil == branch){
        return __err_new_git();
    }

    if(0 > git_branch_delete(branch)){
        git_reference_free(branch);
        return __err_new_git();
    }

    git_reference_free(branch);
    return nil;
}

//@param hdr[in]:
//@param oldname[in]: current branch name, **without prefix: 'refs/heads'**
//@param newname[in]: new name for it
static Error *
rename_branch_local(git_repository *hdr, const char *oldname, const char *newname){
    __check_nil(hdr&&oldname&&newname);
    git_reference *oldref;
    git_reference *newref;

    if(0 > git_branch_lookup(&oldref, hdr, oldname, GIT_BRANCH_LOCAL)){
        return __err_new_git();
    }

    if(nil == oldref){
        return __err_new_git();
    }

    if(0 > git_branch_move(&newref, oldref, newname, 0)){
        git_reference_free(oldref);
        return __err_new_git();
    }

    git_reference_free(oldref);
    git_reference_free(newref);
    return nil;
}

//@param hdr[in]:
//@param branchname[in]: branch to switch to
static Error *
switch_branch_local(git_repository *hdr, const char *branchname){
    __check_nil(hdr&&branchname);

    git_reference *branch;
    if(0 > git_branch_lookup(&branch, hdr, branchname, GIT_BRANCH_LOCAL)){
        return __err_new_git();
    }
    if(nil == branch){
        return __err_new_git();
    }

    if(0 > git_repository_set_head(hdr, git_reference_name(branch))){
        git_reference_free(branch);
        return __err_new_git();
    }

    git_reference_free(branch);
    return 0;
}

/*
 * **************
 * Commit Management
 * **************
 */

static void
revwalker_free(git_revwalk *revwalker, git_object *revobj){
    git_object_free(revobj);
    git_revwalk_free(revwalker);
}

//@param walker[out]:
//@param obj[out]: walker associated obj, need git_object_free(_)
//@param hdr[in]:
//@param ref[in]: usually a branch name
//@param sort_mode[in]:
static Error *
gen_walker(git_revwalk **revwalker, git_object **revobj, git_repository *hdr, const char *branchname, _i sort_mode){
    __check_nil(revwalker&&revobj&&hdr&&branchname);

    if(0 > git_revwalk_new(revwalker, hdr)){
        return __err_new_git();
    }

    //sort_mode: 0 for git's default, other value for reverse sorting
    sort_mode = (0 == sort_mode) ? GIT_SORT_TIME : GIT_SORT_TIME|GIT_SORT_REVERSE;
    git_revwalk_sorting(*revwalker, sort_mode);

    if(0 > git_revparse_single(revobj, hdr, branchname)){
        revwalker_free(*revwalker, *revobj);
        return __err_new_git();
    }
    if(0 > git_revwalk_push(*revwalker, git_object_id(*revobj))){
        revwalker_free(*revwalker, *revobj);
        return __err_new_git();
    }

    return nil;
}

//**USAGE: keep call this func in a loop until nil == *revsig**
//@param commit[out]: commit hdr to get info from
//@param revsig[out]: sha1 40bytes revsig
//@param hdr[in]:
//@param revwalker[in]:
static Error *
get_commit(git_commit **commit, const char **revsig, git_repository *hdr, git_revwalk *revwalker){
    __check_nil(commit&&revsig&&hdr&&revwalker);
    git_oid oid;
    _i rv;

    *revsig = nil;
    if(0 == (rv = git_revwalk_next(&oid, revwalker))){
        if(0 > git_commit_lookup(commit, hdr, &oid)){
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
get_commit_ts(git_commit *commit){
    return git_commit_time(commit);
}

static const char *
get_commit_msg(git_commit *commit){
    return git_commit_message_raw(commit);
}

static void
commit_free(git_commit *commit){
    git_commit_free(commit);
}

/*
 * **used to add a path OR a single file**
 * act as shell cmds：
 *     git config user.name _
 *     && git config user.email _
 *     && git add $path
 *     && git commit --allow-empty -m "_"
 * @param hdr[in]:
 * @param ref[in]: refs/heads/xxx
 * @param commit_msg[in]: user's commit msg
 */
static Error *
do_commit(git_repository *hdr, const char *ref, const char *commit_msg){
    __check_nil(hdr&&ref&&commit_msg);
    _i rv;

    git_index* index;
    git_commit *parent_commit;

    git_tree *tree;
    git_oid tree_oid;
    git_oid parent_oid;

    _i parent_cnt;
    const git_commit *parent_commits[1];

    git_signature *me;
    git_oid commit_oid;

    //get index
    if(0 > git_repository_index(&index, hdr)){
        return __err_new_git();
    }

    //git add file
    static char *path[1] = {"."};
    git_strarray paths;
    paths.strings = path;
    paths.count = 1;

    rv = git_index_add_all(index, &paths, GIT_INDEX_ADD_FORCE, nil, nil);

    if(0 > rv){
        git_index_free(index);
        return __err_new_git();
    }

    //Write the in-memory index to disk
    if(0 > git_index_write(index)){
        git_index_free(index);
        return __err_new_git();
    }

    //get parent's commit_oid, OR create a new branch
    if(0 == (rv = git_reference_name_to_id(&parent_oid, hdr, ref))){
        if(0 > git_commit_lookup(&parent_commit, hdr, &parent_oid)){
            git_index_free(index);
            return __err_new_git();
        }

        parent_cnt = 1;
        parent_commits[0] = parent_commit;
    }else if(GIT_ENOTFOUND == rv){
        //branch not exist(will be auto created)
        parent_commit = nil;
        parent_cnt = 0;
    }else{
        git_index_free(index);
        return __err_new_git();
    }

    //commit to work_area, and create a subtree, and write tree_oid out
    if(0 > git_index_write_tree(&tree_oid, index)){
        if(nil != parent_commit){
            git_commit_free(parent_commit);
        }
        git_index_free(index);
        return __err_new_git();
    }

    //look up tree by tree_oid
    if(0 > git_tree_lookup(&tree, hdr, &tree_oid)){
        if(nil != parent_commit){
            git_commit_free(parent_commit);
        }
        git_index_free(index);
        return __err_new_git();
    }

    //config temp info
    if(0 > git_signature_now(&me, "_", "_")){
        git_tree_free(tree);
        if(nil != parent_commit){
            git_commit_free(parent_commit);
        }
        git_index_free(index);
        return __err_new_git();
    }

    //associate newly created tree and parent_commits, write to disk
    if(0 > git_commit_create(&commit_oid, hdr, ref, me, me, "UTF-8", commit_msg, tree, parent_cnt, parent_commits)){
        git_signature_free(me);
        git_tree_free(tree);
        if(nil != parent_commit){
            git_commit_free(parent_commit);
        }
        git_index_free(index);
        return __err_new_git();
    }

    //final: clean...
    git_index_free(index);
    if(nil != parent_commit){
        git_commit_free(parent_commit);
    }
    git_tree_free(tree);
    git_signature_free(me);

    return nil;
}

#undef __err_new_git
