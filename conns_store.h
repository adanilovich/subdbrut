#pragma once
#define _GNU_SOURCE
#include <glib.h>
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

struct file_state {
        int sockfd;
        int timerfd;
        uint8_t* packet;
        size_t packetlen;
};

struct container {
        int type;
        struct container* next;
        struct file_state* data;
};

struct container* new_container(struct file_state* fs, struct container* next, int type);
struct file_state* new_file_state(int sockfd, int timerfd, uint8_t* packet, size_t packetlen);
void free_container(struct container* container);


