#pragma once
#define __STDC_WANT_LIB_EXT2__ 1
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

struct timer_state {
        int timerfd;
        int sockfd;
};
void print_http_status_code(char* buf);
int reg_query(struct thread_data* thdata, uint8_t* packet, size_t packetlen, char* ip, int port, int timeout);
void set_nonblocking(int fd);
void set_timeout(int sockfd, int timeout);
int connect_server(int sockfd, char* ip, int port, int timeout);
int reg_socket(int epollfd, int sockfd);

int create_timerfd(int sec);
