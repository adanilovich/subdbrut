#include "helpers.h"
#include "string.h"

void print_http_status_code(char* buf)
{
        if (strstr(buf, "200") != NULL) {
                puts("200 OK");
                return;
        } else if (strstr(buf, "400") != NULL) {
                puts("400 not found");
                return;
        } else if (strstr(buf, "404") != NULL) {
                puts("404");
                return;
        }
        puts("status not detect");
}
