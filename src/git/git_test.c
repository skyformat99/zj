#include "git.c"
#include "os.c"
#include "utils.c"

#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>

#define __err_new_git() __err_new(-1, nil == giterr_last() ? "error without message" : giterr_last()->message, nil)

git_repository *repo_hdr;
char *repo_path = "/tmp/__gitdir";

git_repository *repo_hdr2;
char *repo_path2 = "/tmp/__gitdir2";

char *username;
char *repo_url;

__init void
hahahaha(void){
    if(nil == (username = getenv("USER"))){
        __fatal_sys();
    }

    repo_url = __alloc(strlen(username) + sizeof("ssh://@localhost/tmp/__gitdir/.git"));
    sprintf(repo_url, "ssh://%s@localhost/tmp/__gitdir/.git", username);
}

void
_mkdir_and_create_file(const char *dir, const char *file){
    _i fd;
    if(0 > mkdir(dir, 0700)){
        if(EEXIST != errno){
            __fatal_sys();
        }
    }
    if(0 > chdir(dir)){ __fatal_sys(); }
    if(0 > (fd = open(file, O_CREAT|O_WRONLY, 0600))){
        __fatal_sys();
    }

    char randbuf[16];
    sprintf(randbuf, "%d\n", utils.urand());
    if(16 != write(fd, randbuf, 16)){ __fatal_sys(); }
    close(fd);
}

void
_env_init(void){
    Error *e;
    __check_fatal(e, git.env_init());
    git.env_clean();
    __check_fatal(e, git.env_init());
}

void
_repo_init(git_repository **hdr, const char *path){
    Error *e;
    __check_fatal(e, git.repo_init(hdr, path));
    git.repo_close(repo_hdr);
}

void
_config_name_and_email(){
    char ***kv = __alloc(2 * sizeof(char **));
    char *kv0[2] = {"user.name", username};
    char *kv1[2] = {"user.email", "@163.com"};
    kv[0] = kv0;
    kv[1] = kv1;

    Error *e;
    char *value;

    __check_fatal(e, git.config(kv, 2));
    free(kv);

    __check_fatal(e, git.get_cfg("user.name", &value));
    So(0, strcmp(username, value));
    free(value);

    __check_fatal(e, git.get_cfg("user.email", &value));
    So(0, strcmp("@163.com", value));
    free(value);
}

void
_repo_open(git_repository **hdr, const char *path){
    Error *e;
    __check_fatal(e, git.repo_open(hdr, path));
}

void
_clone(void){
    Error *e;
    __check_fatal(e, git.clone(&repo_hdr2, repo_url, repo_path2));
    git.repo_close(repo_hdr2);
}

void
_do_commit(git_repository *hdr){
    Error *e;
    __check_fatal(e, git.do_commit(hdr, "refs/heads/master", "some commit msg"));
}

void
_push(void){
    Error *e;
    char *rmbranch[1] = {"+refs/heads/master:refs/heads/test"};
    __check_fatal(e, git.push(repo_hdr2, repo_url, rmbranch, 1));
}

void
_fetch(void){
    Error *e;
    char *rmbranch[1] = {"+refs/heads/master:refs/heads/test"};
    __check_fatal(e, git.fetch(repo_hdr2, repo_url, rmbranch, 1));
}

void
log_walker(void){
    Error *e;
    git_revwalk *walker;
    git_object *obj;

    git_commit *commit;
    const char *revsig;

    __check_fatal(e, git.gen_walker(&walker, &obj, repo_hdr2, "refs/heads/master", 0));

    __check_fatal(e, git.get_commit(&commit, &revsig, repo_hdr2, walker));
    SoLt(0, printf("revsig: %s, ts: %ld, msg: %s\n", revsig, git.get_commit_ts(commit), git.get_commit_msg(commit)));
    git.commit_free(commit);

    __check_fatal(e, git.get_commit(&commit, &revsig, repo_hdr2, walker));
    SoLt(0, printf("revsig: %s, ts: %ld, msg: %s\n", revsig, git.get_commit_ts(commit), git.get_commit_msg(commit)));
    git.commit_free(commit);

    __check_fatal(e, git.get_commit(&commit, &revsig, repo_hdr2, walker));
    SoLt(0, printf("revsig: %s, ts: %ld, msg: %s\n", revsig, git.get_commit_ts(commit), git.get_commit_msg(commit)));
    git.commit_free(commit);

    revwalker_free(walker, obj);
}

void
branch_mgmt(void){
    Error *e;
    _bool rv;

    __check_fatal(e, git.create_branch(repo_hdr2, "new01", "refs/heads/master"));
    __check_fatal(e, git.rename_branch(repo_hdr2, "new01", "new02"));
    __check_fatal(e, git.switch_branch(repo_hdr2, "new02"));
    __check_fatal(e, git.del_branch(repo_hdr2, "master"));

    __check_fatal(e, git.branch_exists(repo_hdr2, "master", &rv));
    So(0, rv);
    __check_fatal(e, git.branch_exists(repo_hdr2, "new02", &rv));
    So(1, rv);
    __check_fatal(e, git.branch_is_head(repo_hdr2, "new02", &rv));
    So(1, rv);
}

_i
main(void){
    (void)os.rm_all(repo_path);
    (void)os.rm_all(repo_path2);

    sem_t *sem[2];
    if(SEM_FAILED == (sem[0] = sem_open("/__gittstsem", O_CREAT|O_RDWR, 0600))){
        __fatal_sys();
    }
    if(SEM_FAILED == (sem[1] = sem_open("/__gittstsem2", O_CREAT|O_RDWR, 0600))){
        __fatal_sys();
    }

    pid_t pid = fork();
    if(0 > pid){
        __fatal_sys();
    }else if(0 < pid){
        _env_init();

        sem_wait(sem[0]);
        _clone();
        sem_post(sem[1]);

        _repo_open(&repo_hdr2, repo_path2);

        _mkdir_and_create_file(repo_path2, "fileX");
        _do_commit(repo_hdr2);
        _push();

        _mkdir_and_create_file(repo_path2, "fileY");
        _do_commit(repo_hdr2);
        _push();

        sem_wait(sem[0]);
        _fetch();

        log_walker();
        branch_mgmt();

        git.repo_close(repo_hdr2);
        git.env_clean();

        sem_close(sem[0]);
        sem_close(sem[1]);

        waitpid(pid, nil, 0);
    }else{
        _env_init();

        _repo_init(&repo_hdr, repo_path);
        _config_name_and_email();
        _repo_open(&repo_hdr, repo_path);

        _mkdir_and_create_file(repo_path, "file");
        _do_commit(repo_hdr);

        sem_post(sem[0]);
        sem_wait(sem[1]);

        _mkdir_and_create_file(repo_path, "file1");
        _mkdir_and_create_file(repo_path, "file2");
        _mkdir_and_create_file(repo_path, "file3");
        _do_commit(repo_hdr);

        sem_post(sem[0]);

        git.repo_close(repo_hdr);
        git.env_clean();
    }
}
