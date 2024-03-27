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

struct sum_message
{
    int a;
    int b;
};

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void handler(int sig, siginfo_t *info, void *ucontext)
{
    mqd_t mq = *(mqd_t *)info->si_value.sival_ptr;
    char msg[20]; // Adjust size as needed
    if (mq_receive(mq, msg, 8192, NULL) == -1)
    {
        perror("mq_receive");
        exit(1);
    }
    printf("Received message: %s\n", msg);
    
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

    mqd_t mq_s = mq_open(q_names.name_s, O_RDWR);
    if (mq_s == -1)
    {
        perror("Queue open failed");
        exit(1);
    }

    sethandler(handler, SIGUSR1);

    while (1)
    {
        struct sum_message msg;
        printf("Enter two numbers (or 'q' to quit): ");
        fflush(stdout);

        if (scanf("%d %d", &msg.a, &msg.b) != 2)
        {
        }

        char message[5];
        sprintf(message, "%d %d", msg.a, msg.b);

        if (mq_send(mq_s, message, sizeof(message) + 1, 0) == -1)
        {
            perror("mq_send");
            exit(1);
        }
        struct sigevent sev;
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGUSR1;
        sev.sigev_notify_attributes = NULL;
        sev.sigev_value.sival_ptr = &mq_s;
        if (mq_notify(mq_s, &sev) == -1)
        {
            perror("mq_notify");
            exit(1);
        }
    }
}

int main(int argc, char **argv)
{

    printf("I recived %s\n", argv[1]);

    queuenames args;

    sprintf(args.name_s, "/%s_s", argv[1]);
    sprintf(args.name_d, "/%s_d", argv[1]);
    sprintf(args.name_m, "/%s_m", argv[1]);
    printf("Queuesdfasas created %s %s %s\n", args.name_s, args.name_d, args.name_m);
    ClientWork(args);
    return 0;
}