#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include "threads.h"
#include "epoll_handler.h"
#include <sys/epoll.h>
#include "conns_store.h"

#define MAXEVENTS 100
static int get_epollfd(int* list);
unsigned _Atomic active_queries;

struct events_slice {
        struct epoll_event** data;
        int scope;
        int len;
} * events_stack;

int* init_epollfd_array(int cores)
{
        int* list = calloc(sizeof(int), cores + 1);
        for (int i = 0; i < cores; i++) {
                int fd = epoll_create1(EPOLL_CLOEXEC);
                if (fd < 0) {
                        g_error("epoll_create: %s", strerror(errno));
                        return NULL;
                }
                list[i] = fd;
        }
        list[cores] = 0;
        return list;
}

struct events_slice* new_events_slice(int scope)
{
        struct events_slice* m = NULL;
        events_stack = malloc(sizeof(*m));
        events_stack->data = calloc(scope, sizeof(struct epoll_event*));
        events_stack->scope = scope;
        events_stack->len = 0;
        return events_stack;
}

void release_events_slice()
{
        for (int i = 0; i < events_stack->scope; i++) {
                free(events_stack->data[i]);
        }
        free(events_stack->data);
        free(events_stack);
}

void add_to_events_slice(struct epoll_event* ee)
{
        events_stack->data[events_stack->len] = ee;
        events_stack->len++;
}

int count = 0;

static int get_epollfd(int* list)
{
        _Atomic static int i;
        if (list[i] == 0) {
                i = ATOMIC_VAR_INIT(0);
        }
        return list[i++];
}

void* event_socket_worker(void* arg)
{
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0x00);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0x00);
        struct thread_data* thdata = (struct thread_data*)arg;
        int sockfd = 0;
        int epollfd = get_epollfd(thdata->epollfd_list);

        struct epoll_event* events = calloc(MAXEVENTS, sizeof(struct epoll_event));
        add_to_events_slice(events);
        while (1) {
                int nevents = epoll_wait(epollfd, events, MAXEVENTS, -1);
                if (nevents == -1) {
                        g_error("event_socket_worker().epoll_wait(): %s, %d, %d",
                            strerror(errno),
                            epollfd,
                            sockfd);
                        return 0;
                }

                for (int i = 0; i < nevents; i++) {
                        process_event(thdata, epollfd, &(events[i]));
                }
        }
}

struct thread_data* init_threads_pool(int* epollfd_list, int events_size)
{
        struct thread_data* thdata = malloc(sizeof(struct thread_data));
        thdata->epollfd_list = epollfd_list;
        thdata->active_queries = ATOMIC_VAR_INIT(0);

        // in order to kill threads in feature
        events_stack = new_events_slice(events_size);
        return thdata;
}

pthread_t* run_threads_pool(struct thread_data* thdata, int cpus)
{
        pthread_t* threads = calloc(cpus, sizeof(pthread_t));

        for (int i = 0; i < cpus; i++) {
                int ret = pthread_create(&threads[i], NULL, event_socket_worker, thdata);
                if (ret < 0) {
                        g_error("run_threads_pool().pthread_create(): %s, %d", strerror(errno), i);
                        exit(1);
                }
        }
        return threads;
}

void thread_pool_release(pthread_t* thread_pool, int cpus)
{
        for (int i = 0; i < cpus; i++) {
                int status = pthread_kill(thread_pool[i], 0);
                if (status != ESRCH) {
                        pthread_cancel(thread_pool[i]);
                        pthread_join(thread_pool[i], 0x00);
                }
        }
        free(thread_pool);
        release_events_slice();
}


