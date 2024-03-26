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



void ServerWork()
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
    queuenames args = {name_s, name_d, name_m};
    mq_close(mq_s);
    mq_close(mq_d);
    mq_close(mq_m);

    mq_unlink(name_s);
    mq_unlink(name_d);
    mq_unlink(name_m);

    return (NULL);
}

void ClientWork(queuenames q_names)
{
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
    printf("I've recieved %s %s %s\n", q_names.name_s, q_names.name_d, q_names.name_m);
    sleep(1);
    mq_close(mq);
    mq_unlink(name_mq);
}

int main(int argc, char **argv)
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
    queuenames args = {name_s, name_d, name_m};

    switch(fork())
    {
        case -1:
            ERR("fork");
        case 0:
            ClientWork(args);
            exit(0);
        default:
            break;
    }
    mq_close(mq_s);
    mq_close(mq_d);
    mq_close(mq_m);

    mq_unlink(name_s);
    mq_unlink(name_d);
    mq_unlink(name_m);

    
    return 0;
}