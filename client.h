#pragma once
#define _GNU_SOURCE
#include <glib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include "epoll_handler.h"
#include "threads.h"
#include "socket.h"
#include "conns_store.h"

#define MAXEVENTS 100
#define SIZE_BUF 10240
#define pd(m, v) printf("%s %ld\n", m, v)
#define ps(m, v) printf("%s %s\n", m, v)

void wait_slot(struct thread_data* thdata, int* rps_count, int rps_limit, int* time_prev);
void wait_complete_work(struct thread_data* thdata);
void free_epollfd(int* epollfd_list);
void extract_fields(char* line, char** domain);
void usage(char* app_name, int rps_limit, int timeout, char *dns_resolver_file_path, int count_threads);


