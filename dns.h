#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <glib.h>
#include <string.h>

/* Structure of the bytes for a DNS header */
typedef struct {
        uint16_t xid;
        uint16_t flags;
        uint16_t qdcount;
        uint16_t ancount;
        uint16_t nscount;
        uint16_t arcount;
} dns_header_t;

/* Structure of the bytes for a DNS question */
typedef struct
{
        char* name;
        uint16_t dnstype;
        uint16_t dnsclass;
} dns_question_t;

/* Structure of the bytes for an IPv4 answer */
typedef struct {
        uint16_t compression;
        uint16_t type;
        uint16_t class;
        uint32_t ttl;
        uint16_t length;
        struct in_addr addr;
} __attribute__((packed)) dns_record_a_t;
char** load_dns_resolvers(char* file_path);
int lines_count(char* file_path);
char* get_next_dns_resolver(char** dns_list);
uint8_t* compose_dns_query(char* hostname, size_t* packetlen);
char* build_domain_qname(char* hostname);
void print_dns_response(uint8_t* response, int buf_size);
