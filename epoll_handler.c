#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include "epoll_handler.h"
#include "conns_store.h"

int add_event(int epollfd, struct container* container, int event);
// epollin will be turn on if socket is closed
void epollin(struct thread_data* thdata, int epollfd, struct container* socket_container)
{
        uint8_t buf[DEFAULT_BUF_SIZE];
        struct file_state* cur_state = socket_container->data;
        int sockfd = cur_state->sockfd;
        size_t n;

        if ((n = read(sockfd, buf, sizeof(buf))) < 0) {
                if (errno == EAGAIN) {
                        puts("eagain");
                        return;
                }
                g_warning("epollin().read(): %s", strerror(errno));
        }
        print_dns_response(buf, n);

        //         print_http_status_code(buf);
        goto clear;
clear:
        free_container(socket_container);
        --thdata->active_queries;
}

// ready to write in the tube
void epollout(struct thread_data* thdata, int epollfd, struct container* socket_container)
{
        struct file_state* cur_state = socket_container->data;
        int sockfd = cur_state->sockfd;
        ssize_t nbytes = write(sockfd, cur_state->packet, cur_state->packetlen);
        if (nbytes < 0) {
                g_warning("epollout().write(): %s", strerror(errno));
                goto clear;
        }
        add_event(epollfd, socket_container, EPOLLIN | EPOLLONESHOT | EPOLLET);

        return;
clear:
        free_container(socket_container);
        --thdata->active_queries;
}

void process_event(struct thread_data* thdata, int epollfd, struct epoll_event* event)
{
        struct container* container = event->data.ptr;
        int cur_event = event->events;
        if (cur_event & EPOLLIN) {
                if (container->type) {
                        epollin(thdata, epollfd, container);
                        return;
                }
                free_container(container);
                //                 printf("timer done: %d\n", epollfd);
                --thdata->active_queries;
        } else if (cur_event & EPOLLOUT) {
                epollout(thdata, epollfd, container);
        }
}

int add_fd2epoll(int epollfd, int fd, struct container* container, uint32_t events)
{
        struct epoll_event ev = { 0 };

        ev.events = events;
        ev.data.ptr = container;

        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) != 0) {
                g_error("add_fd2epoll().epoll_ctl(): %s, %d->%d", strerror(errno), epollfd, fd);
                return -1;
        }
        return 0;
}

int add_event(int epollfd, struct container* container, int event)
{
        struct epoll_event ev = { 0 };
        ev.events = event;
        ev.data.ptr = container;
        int sockfd = container->data->sockfd;
        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &ev) != 0) {
                g_error("add_event().epoll_ctl(): %s, %d, %d", strerror(errno), sockfd, epollfd);
                return -1;
        }
        return 0;
}


