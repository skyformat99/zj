#include <stdio.h>
#include <stdlib.h>

#include "zj_survey.h"

static Error * leader_new(const char *id, nng_socket *sock); 
static Error * voter_new(const char *id, nng_socket *sock);
static Error * send(nng_socket sock, void *data, size_t data_len);
static Error * recv(nng_socket sock, void **data, size_t *data_len);

struct zj_nng zjnng = {
	.leader_new = leader_new,
	.voter_new = voter_new,
	.send = send,
	.recv = recv,
};

//@param id[in]: handler name
//@param sock[out]: created handler
static Error *
leader_new(const char *id, nng_socket *sock){
	_i e = nng_surveyor0_open(sock);
	if (0 != e) {
		return __err_new(e, nng_strerror(e), nil);
	}

	e = nng_listen(*sock, id, NULL, 0);
	if (0 != e) {
		return __err_new(e, nng_strerror(e), nil);
	}

	return nil;
}

//@param id[in]: handler name
//@param sock[out]: created handler
static Error *
voter_new(const char *id, nng_socket *sock){
	_i e = nng_respondent0_open(sock);
	if (0 != e) {
		return __err_new(e, nng_strerror(e), nil);
	}
	
	e = nng_dial(*sock, id, NULL, 0);
	if (0 != e) {
		return __err_new(e, nng_strerror(e), nil);
	}

	return nil;
}

//@param sock[in]: handler
//@param data[in]: data to send
//@param data_len[in]: len of data
static Error *
send(nng_socket sock, void *data, size_t data_len){
	_i e = nng_send(sock, data, data_len, 0);
	if (0 != e) {
		return __err_new(e, nng_strerror(e), nil);
	}
	return nil;
}

//@param sock[in]: handler
//@param data[out]: must call nng_free except given 'nil'
//@param data_len[out]: actual len of recved data
static Error *
recv(nng_socket sock, void **data, size_t *data_len){
	_i e;
	if(nil == data || nil == data_len){
		//only for info purpose!
		char buf[1];
		size_t len = 1;
    	e = nng_recv(sock, &buf, &len, 0);
		if (0 != e) {
			return __err_new(e, nng_strerror(e), nil);
		}
	} else {
		//must call: nng_free()
    	e = nng_recv(sock, data, data_len, NNG_FLAG_ALLOC);
		if (0 != e) {
			return __err_new(e, nng_strerror(e), nil);
		}
	}

	return nil;
}


//#define _ZJ_UNIT_TEST
//#define _XOPEN_SOURCE 700
#ifdef _ZJ_UNIT_TEST
#include <string.h>

#include "zj_threadpool.c"
#include "convey.c"

#define __node_total 20
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t br;

#define __atomic_log(__e) do{\
	pthread_mutex_lock(&mlock);\
	__display_and_clean(__e);\
	pthread_mutex_unlock(&mlock);\
}while(0)

void *
trd_worker(void *url){
	nng_socket sock;
	Error *e = voter_new(url, &sock);
	if(nil != e){
		__atomic_log(e);
		exit(1);
	}

	pthread_barrier_wait(&br);

	e = send(sock, "", sizeof(""));
	if(nil != e){
		__atomic_log(e);
		exit(1);
	}

	nng_close(sock);
	return nil;
}

Main({
    Test("nng survey tests", {
        Convey("thread_barrier mode", {
			_i eno = pthread_barrier_init(&br, nil, __node_total);
			if(0 != eno){
				__info(strerror(eno));
				exit(1);
			}

			Error *e = threadpool.init(__node_total - 1, __node_total);
			if(nil != e){
				__atomic_log(e);
				exit(1);
			}

			static char *url = "tcp://[::1]:9000";
			nng_socket sock;
			e = leader_new(url, &sock);
			if(nil != e){
				__atomic_log(e);
				exit(1);
			}

			_i i = 1; //leave one to current thread
			while(i < __node_total){
				threadpool.add(trd_worker, url);
				++i;
			}

			pthread_barrier_wait(&br);

			i = 0;
			while(i < __node_total){
				e = recv(sock, nil, nil);
				if(nil != e){
					__atomic_log(e);
					exit(1);
				}
				++i;
			}

			nng_close(sock);
			pthread_barrier_destroy(&br);

			So(__node_total == i);
        });
    });
})
#endif
