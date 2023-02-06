#pragma once
#include <errno.h>
#include <string.h>
#include "threads.h"
#include "conns_store.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <glib.h>
#define DEFAULT_BUF_SIZE 10240

void epollin(struct thread_data* thdata, int epollfd, struct container* container);
void epollout(struct thread_data* thdata, int epollfd, struct container* container);
void process_event(struct thread_data* thdata, int epollfd, struct epoll_event* event);

int add_fd2epoll(int epollfd, int fd, struct container* container, uint32_t events);
int del_event(int epollfd, int sockfd);
int upd_event(int epollfd, int fd, struct file_state* fs, int events);

void print_dns_response(uint8_t* response, int buf_size);

