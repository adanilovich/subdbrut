#include "conns_store.h"
#include <pthread.h>

void free_container(struct container* container)
{
        struct file_state* data = container->data;
        int sockfd = data->sockfd;
        int timerfd = data->timerfd;
        free(data->packet);
        free(data);
        free(container->next);
        free(container);
        close(timerfd);
        close(sockfd);
}

// 0 - timer
// 1 - socket
struct container* new_container(struct file_state* fs, struct container* next, int type)
{
        struct container* c = malloc(sizeof(struct container));
        c->next = next;
        c->type = type;
        c->data = fs;
        return c;
}

struct file_state* new_file_state(int sockfd, int timerfd, uint8_t* packet, size_t packetlen)
{
        struct file_state* fs = malloc(sizeof(struct file_state));
        if (fs == NULL)
                g_error("new_file_state().malloc");
        fs->sockfd = sockfd;
        fs->timerfd = timerfd;
        fs->packet = packet;
        fs->packetlen = packetlen;
        return fs;
}


