#include "client.h"
#include "dns.h"
#define TIME_CONTROL 100 // ms
#define RPS 1
#define REQ_TIMEOUT 3
#define LEN_LINE 1024

char* dns_resolver_file_path = "dns_resolvers.txt";
int main(int argc, char** argv)
{
        setbuf(stdout, NULL);
        int rps_limit = RPS, timeout = REQ_TIMEOUT, opt = 0;
        int count_threads = sysconf(_SC_NPROCESSORS_ONLN);
        while ((opt = getopt(argc, argv, "d:i:r:t:h")) != -1) {
                switch (opt) {
                case 'r':
                        rps_limit = atoi(optarg);
                        if (!rps_limit) {
                                g_error("you forgot argue");
                        }
                        break;
                case 't':
                        count_threads = atoi(optarg);
                        if (!count_threads) {
                                g_error("you forgot argue");
                        }
                        break;
                case 'd':
                        dns_resolver_file_path = optarg;
                        break;
                case 'i':
                        timeout = atoi(optarg);
                        if (!timeout) {
                                g_error("you forgot argue");
                        }
                        break;
                case 'h':
                        usage(argv[0], rps_limit, timeout, dns_resolver_file_path, timeout);
                        exit(1);
                }
        }
        sigaction(SIGPIPE, &(struct sigaction) { SIG_IGN }, NULL);

        char** resolvers = load_dns_resolvers(dns_resolver_file_path);
        int* epollfd_list = init_epollfd_array(count_threads);
        char* line[LEN_LINE];

        // create pool threads of socket and timer event process
        struct thread_data* thdata = init_threads_pool(
            epollfd_list,
            count_threads);
        pthread_t* threads_pool = run_threads_pool(thdata, count_threads);

        char *domain, *resolver;
        int port = 0, rps_count = 0;
        size_t packetlen;
        uint8_t* packet;
        int prev_time = (int)time(NULL);
        while (fgets(line, LEN_LINE, stdin) != NULL) {
                extract_fields(line, &domain);
                packetlen = 0;
                resolver = get_next_dns_resolver(resolvers);
                packet = compose_dns_query(domain, &packetlen);
                reg_query(thdata, packet, packetlen, resolver, 53, timeout);

                rps_count++;
                wait_slot(thdata, &rps_count, rps_limit, &prev_time);
        }

        wait_complete_work(thdata);

        // clean store of timers and sockets
        free_epollfd(epollfd_list);
        thread_pool_release(threads_pool, count_threads);
        free(thdata);
        return 0;
}

void usage(char* app_name, int rps_limit, int timeout, char* dns_resolver_file_path, int count_threads)
{
        printf("[ %s ] Aggressive subdomain brutforcer \n", app_name);
        printf("\nOptions:\n");
        printf("  -r        Requests per second. Default = %d\n"
               "  -i        Timeout is how long will be your connection. Default = %d\n"
               "  -d        File contained dns resolvers list. Default = %s\n"
               "  -t        Amount of threads, that equals cores number. Default = %d\n",
            rps_limit, timeout, dns_resolver_file_path, count_threads);
        printf("\nExample:\n");
        printf("cat subdomains_list.txt | ./subdbrut -r 1000 -i 3 -d dns_resolvers.txt\n");
}

void extract_fields(char* line, char** domain)
{
        line[strlen(line) - 1] = '\0';
        *domain = line;
}

void free_epollfd(int* list)
{
        int* p = list;
        do {
                close(*p++);
        } while (*p != 0);
        free(list);
}

void wait_slot(struct thread_data* thdata, int* rps_count, int rps_limit, int* time_prev)
{
        int time_now = (int)time(NULL);
        if (time_now > *time_prev) {
                *rps_count = 0;
        }
        while (thdata->active_queries >= rps_limit || *rps_count >= rps_limit) {
                usleep(TIME_CONTROL);

                time_now = (int)time(NULL);
                if (time_now > *time_prev) {
                        *rps_count = 0;
                }
        }

        *time_prev = time_now;
}

void wait_complete_work(struct thread_data* thdata)
{
        while (thdata->active_queries != 0) {
                //                 printf("active queries:%d\n", thdata->active_queries);
                sleep(1);
        }
}


