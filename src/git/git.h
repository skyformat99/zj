#ifndef _GIT_H
#define _GIT_H

#include "utils.h"
#include "git2.h"

struct Git{
    Error *(*env_init) (void) __mustuse;
    void (*env_clean) (void);
    
    Error *(*repo_init) (git_repository **hdr, const char *path) __prm_nonnull __mustuse;
    void (*repo_free) (git_repository *hdr) __prm_nonnull;
    
    Error *(*config) (char *repo_path, char *(*kv[2]), _i kv_n) __prm_nonnull __mustuse;
    
    Error *(*repo_open) (git_repository **hdr, const char *addr) __prm_nonnull __mustuse;
    void (*repo_close) (git_repository *hdr) __prm_nonnull;
    
    Error *(*clone) (git_repository **hdr, char *repo_addr, char *local_path) __prm_nonnull __mustuse;
    Error *(*fetch) (git_repository *repo_hdr, char *addr, char **refs, _i refscnt) __prm_nonnull __mustuse;
    Error *(*push) (git_repository *repo_hdr, char *addr, char **refs, _i refscnt) __prm_nonnull __mustuse;
    
    Error *(*create_branch) (git_repository *repo_hdr, char *branch_name, char *base_rev) __prm_nonnull __mustuse;
    Error *(*del_branch) (git_repository *repo_hdr, char *branch_name) __prm_nonnull __mustuse;
    Error *(*rename_branch) (git_repository *repo_hdr, char *oldname, char *newname) __prm_nonnull __mustuse;
    Error *(*switch_branch) (git_repository *repo_hdr, char *branch_name) __prm_nonnull __mustuse;
    
    Error *(*branch_exists) (git_repository *repo_hdr, const char *branch_name, _i *res) __prm_nonnull __mustuse;
    Error *(*branch_is_head) (git_repository *repo_hdr, const char *branch_name, _i *res) __prm_nonnull __mustuse;
    
    Error *(*gen_walker) (git_revwalk **walker, git_object **obj, git_repository *repo_hdr, char *ref, _i sort_mode) __prm_nonnull __mustuse;
    void (*walker_free) (git_revwalk *walker, git_object *obj) __prm_nonnull;
    
    Error *(*get_commit) (git_commit **commit, char **revsig, git_repository *repo_hdr, git_revwalk *walker) __prm_nonnull __mustuse;
    void (*commit_free) (git_commit *commit) __prm_nonnull;
    
    time_t (*get_commit_ts) (git_commit *commit) __prm_nonnull;
    const char *(*get_commit_msg) (git_commit *commit) __prm_nonnull __mustuse;
    
    Error *(*do_commit) (git_repository *repo_hdr, char *branch_name, char *path, char *msg) __prm_nonnull __mustuse;
};

struct Git git;

#endif
