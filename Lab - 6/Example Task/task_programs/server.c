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

void notification_function(union sigval sv)
{
    mqd_t mq = *(mqd_t *)(sv.sival_ptr);
    char message[20]; // Adjust size as needed
    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = notification_function;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = sv.sival_ptr;
    if (mq_notify(mq, &sev) == -1)
    {
        perror("mq_notify asjdnaklda");
        exit(1);
    }

    if (mq_receive(mq, message, 8192, NULL) == -1)
    {
        perror("mq_receive");
        exit(1);
    }
    struct sum_message msg;
    pid_t pids = getpid();
    sscanf(message, "%d %d %d", &msg.a, &msg.b, &pids);
    char name_m[20];
    sprintf(name_m, "/%d", pids);
    printf("Received message: %s \n", name_m);
    mqd_t mq_m = mq_open(name_m, O_RDWR, 0666, NULL);
    int sum = msg.a + msg.b;
    char *sum_str = (char *)malloc(20);
    sprintf(sum_str, "%d", sum);
    if (mq_send(mq_m, sum_str, sizeof(msg) + 1, 0) == -1)
    {
        perror("mq_send asdasd asda");
        free(sum_str);
        exit(1);
    }
    printf("Sent message: %s\n", sum_str);

    // Re-register for the next message
    
}
void sumHandler(int sig, siginfo_t *info, void *ucontext)
{
    printf("Signal received\n");
    mqd_t mq = *(mqd_t *)info->si_value.sival_ptr;
    struct sum_message msg;
    char message[20];
    static struct sigevent not ;
    not .sigev_notify = SIGEV_THREAD;
    not .sigev_notify_function = notification_function;
    not .sigev_value.sival_ptr = info->si_value.sival_ptr;
    if (mq_notify(mq, &not ) < 0)
        ERR("mq_notify");

    ssize_t received = mq_receive(mq, message, 8192, NULL);
    if (received < 1)
    {
        if (errno != EAGAIN)
        {
            perror("mq_receive");
            exit(1);
        }
    }
    else
    {
        sscanf(message, "%d %d", &msg.a, &msg.b);
        int sum = msg.a + msg.b;
        char *sum_str = (char *)malloc(20);
        sprintf(sum_str, "%d", sum);
        if (mq_send(mq, sum_str, sizeof(msg), 0) == -1)
        {
            perror("mq_send");
            free(sum_str); // Free the allocated memory before exiting
            exit(1);
        }
        printf("Sent message: %s\n", sum_str);
        free(sum_str); // Free the allocated memory after it's no longer needed
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
    if (mq_s == (mqd_t) -1) {
        perror("mq_open name_s");
        exit(EXIT_FAILURE);
    }

    mqd_t mq_d = mq_open(name_d, O_RDWR | O_CREAT, 0666, NULL);
    if (mq_d == (mqd_t) -1) {
        perror("mq_open name_d");
        exit(EXIT_FAILURE);
    }

    mqd_t mq_m = mq_open(name_m, O_RDWR | O_CREAT, 0666, NULL);
    if (mq_m == (mqd_t) -1) {
        perror("mq_open name_m");
        exit(EXIT_FAILURE);
    }    
    printf("Queues created %s %s %s\n", name_s, name_d, name_m);



    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = notification_function;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = &mq_s;
    if (mq_notify(mq_s, &sev) == -1)
    {
        perror("mq_notify asdklamsdk");
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