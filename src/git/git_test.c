#include "git.c"
#include "utils.c"
#include "os.c"
#include <fcntl.h>
#include <sys/wait.h>

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
    char *kv0[2] = {"user.name", getenv("USER")};
    char *kv1[2] = {"user.email", "@163.com"};
    kv[0] = kv0;
    kv[1] = kv1;

    Error *e;
    __check_fatal(e, git.config(kv, 2));
    free(kv);

    git_config *cfg;
    if(0 > git_config_open_default(&cfg)){
        e = __err_new_git();
        __display_and_fatal(e);
    }

    git_config_entry *v;
    if(0 > git_config_get_entry(&v, cfg, "user.name")){
        e = __err_new_git();
        __display_and_fatal(e);
    }
    So(0, strcmp("x", v->value));
    v->free(v);
    if(0 > git_config_get_entry(&v, cfg, "user.email")){
        e = __err_new_git();
        __display_and_fatal(e);
    }
    So(0, strcmp("@163.com", v->value));
    v->free(v);
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

_i
main(void){
    (void)os.rm_all(repo_path);
    (void)os.rm_all(repo_path2);

    pid_t pid = fork();
    if(0 > pid){
        __fatal_sys();
    }else if(0 < pid){
        _env_init();

        sleep(1);
        _clone();

        _repo_open(&repo_hdr2, repo_path2);

        _mkdir_and_create_file(repo_path2, "fileX");
        _do_commit(repo_hdr2);
        _push();

        _mkdir_and_create_file(repo_path2, "fileY");
        _do_commit(repo_hdr2);
        _push();

        _fetch();

        git.repo_close(repo_hdr2);
        git.env_clean();

        waitpid(pid, nil, 0);
    }else{
        _env_init();

        _repo_init(&repo_hdr, repo_path);
        _config_name_and_email();
        _repo_open(&repo_hdr, repo_path);

        _mkdir_and_create_file(repo_path, "file");
        _do_commit(repo_hdr);

        sleep(1);

        _mkdir_and_create_file(repo_path, "file1");
        _mkdir_and_create_file(repo_path, "file2");
        _mkdir_and_create_file(repo_path, "file3");
        _do_commit(repo_hdr);

        git.repo_close(repo_hdr);
        git.env_clean();
    }
}
