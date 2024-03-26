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

void sumHandler(int sig, siginfo_t *info, void *ucontext)
{
    printf("Signal received\n");
    mqd_t mq = *(mqd_t *)info->si_value.sival_ptr;
    struct sum_message msg;
    if (mq_receive(mq, (char *)&msg, sizeof(msg), NULL) == -1)
    {
        perror("mq_receive");
        exit(1);
    }

    int sum = msg.a + msg.b;
    printf("Sum: %d\n", sum);

    if (mq_send(mq, (char *)&sum, sizeof(sum), 0) == -1)
    {
        perror("mq_send");
        exit(1);
    }
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

    struct sum_message msg;
    printf("Enter two numbers: ");
    fflush(stdout);

    scanf("%d %d", &msg.a, &msg.b);

    printf("Qname %s\n", q_names.name_s);

    char message[5];
    sprintf(message, "%d %d", msg.a, msg.b);
    
    mqd_t mq_s = mq_open(q_names.name_s, O_RDWR);
    if (mq_s == -1)
    {
        perror("Queue open failed");
        exit(1);
    }

    if (mq_send(mq_s, message, sizeof(message)+1, 0) == -1)
    {
        perror("mq_send");
        exit(1);
    }



    printf("Message sent this is the message: %s with size %d\n", message, sizeof(message));

    int result;
    if (mq_receive(mq_s, (char *)&result, 8192, NULL) == -1)
    {
        perror("mq_receive");
        exit(1);
    }

    printf("Received sum: %d\n", result);

    mq_close(mq);
    mq_close(mq_s);
    mq_unlink(name_mq);
}
void serverThread(union sigval sv)
{
    mqd_t mq = *((mqd_t *) sv.sival_ptr);
    char message[20];
    while (1)
    {
        if (mq_receive(mq, message, 8192, NULL) == -1)
        {
            if(errno == EAGAIN)
                break; // No more messages
            perror("mq_receive");
            exit(1);
        }
        
        struct sum_message msg;
        sscanf(message, "%d %d", &msg.a, &msg.b);

        int sum = msg.a + msg.b;
        printf("Sum: %d\n", sum);

        if (mq_send(mq, (char *)&sum, sizeof(sum), 0) == -1)
        {
            perror("mq_send");
            exit(1);
        }
    }
}

int main(int argc, char **argv)
{

    mq_open("/123", O_RDWR | O_CREAT, 0666, NULL);
    

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

    queuenames args;

    

    sprintf(args.name_s, "/%d_s", pid);
    sprintf(args.name_d, "/%d_d", pid);
    sprintf(args.name_m, "/%d_m", pid);

    printf("args.name_s: %s\n", args.name_s);
    printf("args.name_d: %s\n", args.name_d);
    printf("args.name_m: %s\n", args.name_m);

    int clientPID;

   
    switch (clientPID = fork())
    {
    case -1:
        ERR("fork");
    case 0:
        ClientWork(args);
        exit(0);
    default:
        break;
    }

    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = serverThread;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = &mq_s;
    if (mq_notify(mq_s, &sev) == -1)
    {
        perror("mq_notify");
        exit(1);
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