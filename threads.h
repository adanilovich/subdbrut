#pragma once
#include <pthread.h>
#include <string.h>
#include <glib.h>
#include <stdatomic.h>

struct thread_data {
	int *epollfd_list;
        unsigned _Atomic active_queries;
};

void* event_worker(void* arg_epollfd);
pthread_t* run_threads_pool(struct thread_data* thdata, int cpu_cores);
struct thread_data* init_threads_pool(int* epollfd_list, int events_size);
void thread_pool_release(pthread_t* thread_pool, int cpus);
int* init_epollfd_array(int cores);


