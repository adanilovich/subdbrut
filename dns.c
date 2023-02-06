#include "dns.h"
#include <inttypes.h>

char** load_dns_resolvers(char* file_path)
{
        FILE* fp;
        char line[1024];
        int fl = lines_count(file_path) + 1;
        if ((fp = fopen(file_path, "r")) == NULL) {
                g_error("Error occured while opening file", strerror(errno));
        }

        char** rows = calloc(sizeof(char*), fl);
        char** tmp_p = rows;
        while (fgets(line, 1024, fp) != NULL) {
                int l = strlen(line);
                line[l - 1] = '\0';
                *rows = malloc(l * sizeof(char));
                strcpy(*rows, line);
                rows++;
        }
        *rows = NULL;
        return tmp_p;
}

int lines_count(char* file_path)
{
        FILE* fp;
        int ch, prev_ch = 0, counter = 0;
        if ((fp = fopen(file_path, "r")) == NULL) {
                g_error("Error occured while opening file", strerror(errno));
        }

        while ((ch = fgetc(fp)) != EOF) {
                prev_ch = ch;
                if (ch == '\n')
                        counter++;
        }

        if (prev_ch == 0)
                return 0;
        if (prev_ch != '\n')
                counter++;

        return counter;
}

char* get_next_dns_resolver(char** dns_list)
{
        static int pos = 0;
        char* tmp;

        tmp = *(dns_list + pos);
        if (tmp == NULL) {
                pos = 1;
                return *dns_list;
        }
        pos++;
        return tmp;
}

uint8_t* compose_dns_query(char* hostname, size_t* packetlen)
{
        /* Set up the DNS header */
        dns_header_t header;
        memset(&header, 0, sizeof(dns_header_t));
        header.xid = htons(0x1234); /* Randomly chosen ID */
        header.flags = htons(0x0100); /* Q=0, RD=1 */
        header.qdcount = htons(1); /* Sending 1 question */

        /* Set up the DNS question */
        dns_question_t question;
        question.dnstype = htons(1); /* QTYPE 1=A */
        question.dnsclass = htons(1); /* QCLASS 1=IN */
        question.name = build_domain_qname(hostname);

        /* Copy all fields into a single, concatenated packet */
        *packetlen = sizeof(header) + strlen(hostname) + 2 + sizeof(question.dnstype) + sizeof(question.dnsclass);
        uint8_t* packet = calloc(*packetlen, sizeof(uint8_t));
        uint8_t* p = (uint8_t*)packet;

        /* Copy the header first */
        memcpy(p, &header, sizeof(header));
        p += sizeof(header);

        /* Copy the question name, QTYPE, and QCLASS fields */
        memcpy(p, question.name, strlen(hostname) + 2);
        free(question.name);
        p += strlen(hostname) + 2;
        memcpy(p, &question.dnstype, sizeof(question.dnstype));
        p += sizeof(question.dnstype);
        memcpy(p, &question.dnsclass, sizeof(question.dnsclass));

        return packet;
}

void print_dns_response(uint8_t* response, int buf_size)
{
        /* First, check the header for an error response code */
        dns_header_t* response_header = (dns_header_t*)response;
        if ((ntohs(response_header->flags) & 0xf) != 0) {
                //                 fprintf(stderr, "Failed to get response\n");
                return;
        }

        char domain_name[250];
        uint8_t* question = response + sizeof(dns_header_t) + 1;
        uint8_t* p = question;
        for (int i = 0; i < 255; i++) {
                //                 if (*p == 0x02 || *p == 0x03 || *p == 0x09 || *p == 0x04 || *p == 0x01 || *p == 0x06) {
                if (*p <= 0x20 && *p > 0x00) {
                        domain_name[i] = '.';
                        p++;
                        continue;
                }
                if (*p == 0x00) {
                        domain_name[i] = 0x00;
                        break;
                }
                domain_name[i] = *p;

                p++;
        }

        do {
                if (p - response > buf_size)
                        return;
                if (*p == 0xc0 && *(p + 1) == 0x0c && *(p + 2) == 0x00 && *(p + 3) == 0x01) {
                        //                 if (*p == 0xc0 && *(p + 1) == 0x0c && *(p + 2) == 0x00 && *(p + 3) == 0x01) {
                        break;
                }
        } while (p++);
        p = p + 12;

        char ip[5];
        ip[0] = *p++;
        ip[1] = *p++;
        ip[2] = *p++;
        ip[3] = *p++;
        ip[4] = 0x00;
        printf("%s:%d.%d.%d.%d\n", domain_name, (unsigned char)ip[0], (unsigned char)ip[1], (unsigned char)ip[2], (unsigned char)ip[3]);

        //         dns_record_a_t* records = (dns_record_a_t*)start_of_question;
        //         printf("%s:%s\n", questions[i].name, inet_ntoa(records[i].addr));
}

char* build_domain_qname(char* hostname)
{
        //         if (hostname != NULL)
        //                 return NULL;

        char* name = calloc(strlen(hostname) + 2, sizeof(uint8_t));

        /* Leave the first byte blank for the first field length */
        memcpy(name + 1, hostname, strlen(hostname));

        /* Example:
           +---+---+---+---+---+---+---+---+---+---+---+
           | a | b | c | . | d | e | . | c | o | m | \0|
           +---+---+---+---+---+---+---+---+---+---+---+

           becomes:
           +---+---+---+---+---+---+---+---+---+---+---+---+
           | 3 | a | b | c | 2 | d | e | 3 | c | o | m | 0 |
           +---+---+---+---+---+---+---+---+---+---+---+---+
         */

        uint8_t count = 0;
        uint8_t* prev = (uint8_t*)name;
        for (int i = 0; i < strlen(hostname); i++) {
                /* Look for the next ., then copy the length back to the
                   location of the previous . */
                if (hostname[i] == '.') {
                        *prev = count;
                        prev = (uint8_t*)name + i + 1;
                        count = 0;
                } else
                        count++;
        }
        *prev = count;

        return name;
}


