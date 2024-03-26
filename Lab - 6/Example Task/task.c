#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

typedef struct queuenames
{
    char name_s[20];
    char name_d[20];
    char name_m[20];
} queuenames;

void *ServerWork(void* arg)
{

    int pid = getpid();
    char name_s[20], name_d[20], name_m[20];
    sprintf(name_s, "/%d_s", pid);
    sprintf(name_d, "/%d_d", pid);
    sprintf(name_m, "/%d_m", pid);

    mqd_t mq_s = mq_open(name_s, O_RDWR | O_CREAT, 0666, NULL);
    mqd_t mq_d = mq_open(name_d, O_RDWR | O_CREAT, 0666, NULL);
    mqd_t mq_m = mq_open(name_m, O_RDWR | O_CREAT, 0666, NULL);
    if (mq_s == -1 || mq_d == -1 || mq_m == -1)
    {
        perror("Queue creation failed");
        exit(1);
    }
    printf("Queues created %s %s %s\n", name_s, name_d, name_m);

    sleep(1);
    mq_close(mq_s);
    mq_close(mq_d);
    mq_close(mq_m);

    mq_unlink(name_s);
    mq_unlink(name_d);
    mq_unlink(name_m);
    return (NULL);
}

void *ClientWork(void *arg)
{
    queuenames *args = (queuenames *)arg;
    int pid = getpid();
    char name_mq[20];
    sprintf(name_mq, "/%d", pid);

    mqd_t mq = mq_open(name_mq, O_RDWR | O_CREAT, 0666, NULL);
    if (mq == -1)
    {
        perror("Queue creation failed");
        exit(1);
    }
    printf("Queue created %s\n", name_mq);
    sleep(1);
    mq_close(mq);
    mq_unlink(name_mq);
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t thread_server;
    if (pthread_create(&thread_server, NULL, ServerWork, NULL) != 0)
    {
        perror("Thread creation failed");
        exit(1);
    }
    pthread_t thread_client;
    if (pthread_create(&thread_client, NULL, ClientWork, NULL) != 0)
    {
        perror("Thread creation failed");
        exit(1);
    }

    pthread_join(thread_server, NULL);
    pthread_join(thread_client, NULL);
    return 0;
}