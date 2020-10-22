#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <semaphore.h>

#define gettid() syscall(SYS_gettid)
#define NUM_THREADS 3
#define BUFFER_SIZE 20

//定义缓冲区
typedef int buffer_item;
typedef struct buffer{
    int rear;
    int front;
    buffer_item buffer[BUFFER_SIZE];
}buffer;

sem_t *full;
sem_t *empty;
sem_t *mutex;
void *ptr;

double sleep_time(double lambda_p);//返回一个符合负指数分布的随机变量
void *producer(void *param); //生产者

int main(int argc, char *argv[])
{
    buffer bf;
    memset(&bf, 0, sizeof(buffer));

    double lambda_p = atof(argv[1]);//读取lambda p转化为数字
    
    //打开具名信号量并初始化 
    full = sem_open("full", O_CREAT, 0666, 0);
    empty = sem_open("empty", O_CREAT, 0666, 0);
    mutex = sem_open("mutex", O_CREAT, 0666, 0);
    sem_init(full, 1, 0);
    sem_init(empty, 1, BUFFER_SIZE);
    sem_init(mutex, 1, 1);

    if (argc != 2){
        printf("The number of supplied arguments are false.\n");
        return -1;
    }

    if (atof(argv[1]) < 0){
        printf("The lambda entered should be greater than 0.\n");
        return -1;
    }

    //创建共享内存
    int shm_fd = shm_open("Buffer", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(buffer));
    ptr = mmap(0, sizeof(buffer), PROT_WRITE, MAP_SHARED, shm_fd, 0);

    //创建3个线程
    pthread_t tid[NUM_THREADS];
    pthread_attr_t attr[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++){
        pthread_attr_init(&attr[i]);
        pthread_create(&tid[i], &attr[i], producer, &lambda_p);
    }

    for (int j = 0; j < NUM_THREADS; j++){
        pthread_join(tid[j], NULL);
    }

    return 0;
}


double sleep_time(double lambda_p){
    double r;
    r = ((double)rand() / RAND_MAX);

    while(r == 0 || r == 1){
        r = ((double)rand() / RAND_MAX);
    }

    r = (-1 / lambda_p) * log(1-r);

    return r;
}

void *producer(void *param){
    double lambda_p = *(double *)param;
    do{
        double interval_time = lambda_p;
        unsigned int sleepTime = (unsigned int)sleep_time(interval_time);
        sleep(sleepTime);
        buffer_item item = rand() % 200;
        buffer *shm_ptr = ((buffer *)ptr);
        sem_wait(empty);
        sem_wait(mutex);
        printf("Sleep Time: %d s | Producing the data %d to buffer[%d] by thread %ld in process %d.\n", sleepTime, item,shm_ptr->rear,gettid(), getpid());
        shm_ptr->buffer[shm_ptr->rear] = item;
        shm_ptr->rear = (shm_ptr->rear+1) % BUFFER_SIZE;
        sem_post(mutex);
        sem_post(full);
    }while(1);
    pthread_exit(0);
}

