#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

struct Philosopher{
    int status;
    int number;
    int thinkingPeriod;
    int eatingPeriod;
    int rice;
    int leftForkIndex;
    int rightForkIndex;
    int eatenTimes;
    int masaIndex;
    int sitting;
    pthread_t thread_id;
};
struct Chopstick{
    int index;
    sem_t mutex;
};
struct Table{
    sem_t mutex;
    int tableIndex;
    int capacity;
    int masaTazeleme;
    int riceLeft;
    int hungryPerson;
    struct Philosopher *philos[8];
};

void*  philosopher_thread(void *);
void   philosopherThinking(struct Philosopher*);
int    philosopherPickedLeftChopstick(struct Philosopher*);
int    philosopherPickedRightChopstick(struct Philosopher*, int);
void   philosopherPutDownLeftChopStick(struct Philosopher*);
void   philosopherEatingFirstTime();
int    addPhilosToTable(struct Philosopher*);
void   emptyTable(struct Table*);
void   masaTazele(struct Table*);

struct Chopstick* chopsticks;
struct Table* table;

sem_t global_mutex, tableMutex, eatMutex;
int notEatenYet = 0;
int eating= 1, fulltable = 0;
double para = 0;

int everyoneHasEaten(){
    int counter = 0;
    sem_wait(&global_mutex);
    counter = notEatenYet;
    sem_post(&global_mutex);
    return counter == 0;
}

void checkTable(struct Table* currentTable) {
    if(currentTable->hungryPerson <= 0 && currentTable->capacity == 0) {
        emptyTable(currentTable);
    }
    if (currentTable->riceLeft == 0 && currentTable->hungryPerson > 0){
        masaTazele(currentTable);
    }
}

void* philosopher_thread(void *argument){
    struct Philosopher *philosopher = (struct Philosopher*)argument;

    if (fulltable <10)
        addPhilosToTable(philosopher);

    while (fulltable == 10);
    if(philosopher->sitting == 0){
        printf("just sit philo %i %i\n", philosopher->number, philosopher->masaIndex);
        addPhilosToTable(philosopher);
    }

    while(eating) {
        philosopherThinking(philosopher);

        int waiting_times = philosopherPickedLeftChopstick(philosopher);

        while (waiting_times > 0)
            waiting_times = philosopherPickedRightChopstick(philosopher, waiting_times);

        if (waiting_times == 0) {
            //  printf("Philosopher %ld cannot take second chopstick.\n", philosopher->number);
            philosopherPutDownLeftChopStick(philosopher);
        }else if (waiting_times == -1){
            //    printf("Philosopher %ld cannot eat at this moment, left chopstick is being used.\n", philosopher->number);
        }else {
            philosopherPutDownLeftChopStick(philosopher);
            // printf("Philosopher %ld has eaten successfully...\n", philosopher->number);
        }

        checkTable(&table[philosopher->masaIndex]);
        eating = !everyoneHasEaten();
    }

}

int addPhilosToTable(struct Philosopher* philosopher){
    if(sem_trywait(&table[philosopher->masaIndex].mutex) == 0) {
        if (0 < table[philosopher->masaIndex].capacity) {
            table[philosopher->masaIndex].philos[(8 - table[philosopher->masaIndex].capacity)] = philosopher;
            table[philosopher->masaIndex].capacity--;
            table[philosopher->masaIndex].hungryPerson++;
            philosopher->sitting = 1;
            sem_wait(&tableMutex);
            if(table[philosopher->masaIndex].capacity == 0) fulltable++;
            sem_post(&tableMutex);
            sem_post(&table[philosopher->masaIndex].mutex);
            return 1;
        }else{
            return 0;
        }
    }
}

void emptyTable(struct  Table* emptyTable) {
    sem_wait(&emptyTable->mutex);
    sem_wait(&eatMutex);
    printf("\nCLEANING TABLE %i\n", emptyTable->tableIndex);

    double hesap = 0;
    int i;
    for (i = 0; i < 8; i++) {
        //       printf("PHILO %i has eaten %i rice so far.\n", emptyTable->philos[i]->number, emptyTable->philos[i]->rice);
        hesap += emptyTable->philos[i]->rice * 1; // 100gram pirinç 1tl olsun
    }

    hesap += 99.90 + emptyTable->masaTazeleme * 19.90;
    printf("Masa %i hesap %f\n", emptyTable->tableIndex, hesap);

    sem_wait(&tableMutex);
    para += hesap;
    fulltable--;
    sem_post(&tableMutex);

    emptyTable->masaTazeleme = 0;
    emptyTable->riceLeft = 2000;
    emptyTable->capacity = 8;
    emptyTable->hungryPerson = 0;

    sem_post(&eatMutex);
    sem_post(&emptyTable->mutex);
    printf("\n");
}

void masaTazele(struct  Table* fillTable){
    sem_trywait(&fillTable->mutex);
    sem_wait(&eatMutex);
    fillTable->masaTazeleme += 1;
    fillTable->riceLeft = 2000;
    printf("\n");
    printf("Masa %i tazelendi. \n", fillTable->tableIndex);
    printf("\n");
    sem_post(&eatMutex);
    sem_post(&fillTable->mutex);
}

void  philosopherThinking(struct Philosopher* philosopher) {
    usleep(philosopher->thinkingPeriod * 1000);
}

int philosopherPickedLeftChopstick(struct Philosopher* philosopher) {
    int waiting_times;
    if (sem_trywait (&chopsticks[philosopher->leftForkIndex].mutex) == 0) {
        //   printf("Philosopher %ld got the left  chopstick %i. \n", philosopher->number, philosopher->leftForkIndex);
        waiting_times = 10 + rand() % 50;
        return waiting_times;
    }
    return  -1;
}

int philosopherPickedRightChopstick(struct Philosopher* philosopher, int waiting_times) {
    if (sem_trywait(&chopsticks[philosopher->rightForkIndex].mutex) == 0) {
        //    printf("Philosopher %ld got the right chopstick %i and is eating now. \n", philosopher->number, philosopher->rightForkIndex);
        sem_trywait(&table[philosopher->masaIndex].mutex);
        sem_wait(&eatMutex);
        if (!philosopher->eatenTimes) {
            philosopherEatingFirstTime(philosopher);
            table[philosopher->masaIndex].hungryPerson--;
        }
        philosopher->eatenTimes++;
        usleep(philosopher->eatingPeriod * 1000);

        philosopher->rice += 100;
        table[philosopher->masaIndex].riceLeft -= 100;

        sem_post(&chopsticks[philosopher->rightForkIndex].mutex);
        sem_post(&eatMutex);
        sem_post(&table[philosopher->masaIndex].mutex);
//        printf("Philosopher %ld finished eating and put down the right chopstick %i\n", philosopher->number, philosopher->rightForkIndex);

        return -2;
    } else {
        // can't get right fork
        waiting_times--;
        usleep(3000);
    }
    return waiting_times;
}

void philosopherPutDownLeftChopStick(struct Philosopher* philosopher){
    sem_post(&chopsticks[philosopher->leftForkIndex].mutex);
}

void philosopherEatingFirstTime() {
    sem_wait(&global_mutex);
    notEatenYet--;
    sem_post(&global_mutex);
}

int main(int argc, char* argv[]){
    pthread_attr_t attr;
    struct sched_param param;
    struct Philosopher* philosophers;
    int i, count;
    count = atoi(argv[1]);

    pthread_attr_init (&attr);
    pthread_attr_getschedparam (&attr, &param);

    philosophers = (struct Philosopher*) malloc(sizeof(struct Philosopher) * count);
    chopsticks = (struct Chopstick*) malloc(sizeof(struct Chopstick) * count);
    table = (struct Table*) malloc(sizeof (struct Table) * 10);

    int j ;
    for( j=0 ; j<10 ; j++) {
        sem_init(&table[j].mutex,0,1);
        table[j].tableIndex = j;
        table[j].capacity = 8;
        table[j].masaTazeleme = 1;
        table[j].riceLeft = 2000; //2kg
        table[j].hungryPerson = 0;
    }
    for(i = 0; i <10; i++)
        for(j = 0 ; j<8; j++)
            table[i].philos[j] = (struct Philosopher*) malloc(sizeof(struct Philosopher));


    sem_init(&global_mutex,0,1);
    sem_init(&tableMutex,0,1);
    sem_init(&eatMutex,0,1);

    notEatenYet = count;

    for(i=0; i<count; i++){
        sem_init(&chopsticks[i].mutex,0,1);

        philosophers[i].eatenTimes = 0;
        philosophers[i].number = i + 1;
        philosophers[i].status = (rand() % 5) + 1;

        philosophers[i].rice = 0;
        if( 10 <= (i/8)) philosophers[i].masaIndex = (i / 8) - 10 ;
        else philosophers[i].masaIndex = i / 8;

        philosophers[i].sitting = 0;
        philosophers[i].eatingPeriod = ((rand() % 5) + 1);
        philosophers[i].thinkingPeriod = ((rand() % 5) + 1);
        philosophers[i].leftForkIndex = i;

        if (i+1 == count) philosophers[i].rightForkIndex = 0;

        else philosophers[i].rightForkIndex = i+1;
    }

    for(i=0; i<count; i++) {
        param.sched_priority = philosophers[i].status;
        pthread_attr_setschedparam(&attr, &param);
        pthread_create(&philosophers[i].thread_id, NULL, philosopher_thread, &philosophers[i]);
    }

    for(i=0; i<count; i++)
        pthread_join(philosophers[i].thread_id, NULL);

    /* herkes kaç kere yedigini ogrenmek etmek isterseniz acin
     for(i=0; i<count; i++)
        printf("Philosopher %ld || %i eaten for %d times\n", philosophers[i].thread_id, philosophers[i].number, philosophers[i].eatenTimes);
*/

    printf("fatura: %f\n", para);

    free(table);
    free(chopsticks);
    free(philosophers);
    return 0;
}