#include "threads.h"
#include <sys/timerfd.h>
#include "socket.h"
#include "epoll_handler.h"
#include "conns_store.h"

#define SOCKET_TYPE 1
#define TIMER_TYPE 0

int connect_server(int fd, char* ip, int port, int timeout)
{
        struct linger l = { .l_onoff = 1, .l_linger = 0 };
        int n = setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
        if (n < 0) {
                g_error("connect_server().setsockopt(): %s, %d, %s:%d", strerror(errno), fd, ip, port);
                return -1;
        }

        set_nonblocking(fd);

        struct sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &addr.sin_addr);
        addr.sin_port = htons(port);
        n = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
        if (n < 0)
                g_error("try to make connection");
        return 0;
}

void set_nonblocking(int fd)
{
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
                g_error("set_nonblocking().fcntl(getfl): %s, %d", strerror(errno), fd);
                return;
        }

        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                g_error("set_nonblocking().fcntl(setfl): %s, %d", strerror(errno), fd);
        }
}

static int get_epollfd(int* list)
{
        _Atomic static int i;
        if (list[i] == 0) {
                i = ATOMIC_VAR_INIT(0);
        }
        return list[i++];
}

int reg_query(struct thread_data* thdata, uint8_t* packet, size_t packetlen, char* ip, int port, int timeout)
{
        int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sockfd < 0) {
                g_error("reg_query().socket(): %s, %s, %d", strerror(errno), ip, port);
                return -1;
        }
        //         g_debug("made new sockfd: %d", sockfd);

        int timerfd = create_timerfd(timeout);
        if (timerfd < 0) {
                g_error("reg_query().create_timerfd(): %s, %s, %d", strerror(errno), ip, port);
                return -1;
        }
        //         g_debug("made new timerfd: %d", timerfd);
        struct file_state* fs = new_file_state(sockfd, timerfd, packet, packetlen);

        if (connect_server(sockfd, ip, port, timeout) < 0) {
                g_error("reg_query().connect_server(): %s, %s, %d", strerror(errno), ip, port);
                return -1;
        }

        // free ip str
        // free port str
        int epollfd = get_epollfd(thdata->epollfd_list);
        struct container* timer_container = new_container(fs, NULL, TIMER_TYPE);
        struct container* socket_container = new_container(fs, timer_container, SOCKET_TYPE);
        timer_container->next = socket_container;

        // Attention: first timerfd, second sockfd!!!
        add_fd2epoll(epollfd, timerfd, timer_container, EPOLLIN | EPOLLET);
        add_fd2epoll(epollfd, sockfd, socket_container, EPOLLOUT | EPOLLONESHOT | EPOLLET);
        thdata->active_queries++;
        return sockfd;
}

int create_timerfd(int sec)
{
        struct itimerspec its;
        struct timespec now;
        //
        // get current time
        if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
                g_error("create_timerfd().clock_gettime(): %s", strerror(errno));
                return -1;
        }

        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        its.it_value.tv_sec = now.tv_sec + sec;
        its.it_value.tv_nsec = now.tv_nsec;
        int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
        if (fd == -1) {
                g_error("create_timerfd().timerfd_create(): %s", strerror(errno));
                return -1;
        }

        if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &its, NULL) == -1) {
                g_error("create_timerfd().timerfd_settime(): %s", strerror(errno));
                return -1;
        }
        return fd;
}


