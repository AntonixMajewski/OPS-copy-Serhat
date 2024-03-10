#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L //Added bc macos doestnt have it yey

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef TEMP_FAILURE_RETRY //Added bc macos doestnt have it yey
#define TEMP_FAILURE_RETRY(expr) \
  ({ long int _res; \
     do _res = (long int) (expr); \
     while (_res == -1L && errno == EINTR); \
     _res; })
#endif

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{

    return 0;
}