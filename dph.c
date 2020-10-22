#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <fcntl.h>

#define NUM_PH 5

enum {THINKING,HUNGRY,EATING}state[NUM_PH];

pthread_cond_t self[NUM_PH];// Behalf on each philosophers
pthread_mutex_t mutex[NUM_PH]; // For conditional variables

void *philo(void *param);
void pickup_forks(int id);
void return_forks(int id);
void test(int id);

int main(){
    for(int i = 0; i < NUM_PH; i++){
        pthread_cond_init(&self[i], NULL);
        pthread_mutex_init(&mutex[i], NULL);
    }

    int id[NUM_PH];// For philo function
    pthread_t tid[NUM_PH];// For pthread_create()
    pthread_attr_t attr[NUM_PH];

    for(int j = 0; j < NUM_PH; j++){
        id[j] = j;
        pthread_attr_init(&attr[j]);
        pthread_create(&tid[j], &attr[j], philo, &id[j]);
    }

    for(int k = 0; k < NUM_PH; k++){
        pthread_join(tid[k], NULL);
    }

    return 0;
}


void *philo(void *param){
    do{
    int id = *( (int *)param);
    /* Try to pickup a chopstick*/
    pickup_forks(id);
    printf("The philosopher %d is eating...\n",id);
    /* Eat a while*/
    srand((unsigned)time(NULL));
    int sec = (rand()%((3-1)+1)) +1;// make sec in [1,3]
    sleep(sec);
    /* Return a chopstick */
    return_forks(id);
    printf("The philosopher %d is thinking...\n",id);
    /* Think a while */
    srand((unsigned)time(NULL));
    sec = (rand()%((3-1)+1)) +1;// make sec in [1,3]
    sleep(sec);
    }while(1);
    pthread_exit(NULL);
}

void pickup_forks(int i){
    state[i] = HUNGRY;// Wants to eat
    test(i);// Check can eat or not
    pthread_mutex_lock(&mutex[i]);
    while (state[i] != EATING){
        pthread_cond_wait(&self[i],&mutex[i]);//Wait his neighbors ate
    }
    pthread_mutex_unlock(&mutex[i]);
}

void return_forks(int i){
    state[i] = THINKING;
    //Notify his neighbor that I was ate.
    test((i+4)%5);
    test((i+1)%5);
}

void test(int i){
    // A philosopher can eat when he wants to eat and his neighbors aren't eating.
    if ( (state[(i+4)%5] != EATING)&&
    (state[i] == HUNGRY) &&
    (state[(i+1)%5] != EATING)
    ){
        pthread_mutex_lock(&mutex[i]);
        state[i] = EATING;
        pthread_cond_signal(&self[i]);
        pthread_mutex_unlock(&mutex[i]);
    }

}