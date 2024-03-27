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

void sumHandler(int sig, siginfo_t *info, void *ucontext)
{
    printf("Signal received\n");
    mqd_t mq = *(mqd_t *)info->si_value.sival_ptr;
    struct sum_message msg;
    char message[20];
    if (mq_receive(mq, message, 8192, NULL) == -1)
    {
        perror("mq_receive");
        exit(1);
    }

    sscanf(message, "%d %d", &msg.a, &msg.b);
    int sum = msg.a + msg.b;
    char* sum_str = (char*)malloc(20);
    sprintf(sum_str, "%d", sum);
    if (mq_send(mq, sum_str, sizeof(msg), 0) == -1)
    {
        perror("mq_send");
        exit(1);
    }
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

    sethandler(sumHandler, SIGUSR1);

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

    pid_t child_pid = fork();
    if (child_pid == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (child_pid == 0)
    {
        char pid_str[20];
        sprintf(pid_str, "%d", pid);
        printf("%s\n", pid_str);
        execlp("./client", "./client", pid_str, NULL);
        perror("execlp");
        exit(1);
    }

    while (1)
    {
        ;
    }

    mq_close(mq_s);
    mq_close(mq_d);
    mq_close(mq_m);

    for (;;)
    {
        wait(NULL);
    }
    mq_unlink(name_s);
    mq_unlink(name_d);
    mq_unlink(name_m);
    return 0;
}