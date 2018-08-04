/*
 * TIING YIN
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <semaphore.h>



int* bounded_buffer;

typedef struct producer
{
    int producerId;
    int max_prod;//argv[4]
    int thread_num;

} producerStruct;

typedef struct consumer
{
    int consumerId;
    int max_cons;
    int thread_num;

} consumerStruct;

//index for arrays of what producers produces and what consumers consumed
int producerCounter = 1;
int consumerCounter = 1;
//arrays to keep track of what producers produced and what consumers consumed
int *producerArray;
int *consumerArray;

//arrays of producers and consumers
producerStruct* producerInfo;
consumerStruct* consumerInfo;

//index of bounded buffer for p and c
int cBufferIndex = 0;
int pBufferIndex = 0;

int max_each_produce = 1;

int buffer_cnt_max;//argv[1]
int numOfProducer;//argv[2]
int numOfConsumer;//argv[3]
//int consumedItems;
int pTime;
int cTime;

int item = 1;
int overConsume = 0;
int match = 1;

//thread 
pthread_t* producerThread;
pthread_t* consumerThread;
pthread_attr_t attr;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // creating a mutex that is ready to be locked!

sem_t full;
sem_t empty;

//no block in enqueuq(){} or dequeue()

/* 
 * Function to add item.
 * Item added is returned.
 * It is up to you to determine
 * how to use the ruturn value.
 * If you decide to not use it, then ignore
 * the return value, do not change the
 * return type to void. 
 */
int enqueue_item(int item)
{
    bounded_buffer[pBufferIndex] = item;
    pBufferIndex = (pBufferIndex + 1)%buffer_cnt_max;
    return item;
}


void* producer(void* param)
{
    producerStruct* producer = (producerStruct*)param;

    int count = 0;

    while ((count++ < producer->max_prod))
    {
        sem_wait(&empty);
        pthread_mutex_lock(&m);

        printf("\n%d was produced by thread-> %d", item, producer->producerId+1);
        
        enqueue_item(item);
        producerArray[producerCounter] = item;
        
        item++;
        producerCounter++;

        sleep(pTime);
        pthread_mutex_unlock(&m);
        sem_post(&full);
    }

    pthread_exit(0);
}

/* 
 * Function to remove item.
 * Item removed is returned
 */
int dequeue_item()
{
    consumerArray[consumerCounter] = bounded_buffer[cBufferIndex];
    cBufferIndex = (cBufferIndex + 1)%buffer_cnt_max;
    return consumerArray[consumerCounter];
}

void* consumer(void* param)
{
    consumerStruct* consumer = (consumerStruct*)param;

    int count = 0;

    while (count++ < consumer->max_cons)
    {
        sleep(cTime);
        sem_wait(&full);
        pthread_mutex_lock(&m);

        printf("\n%d was consumed by thread-> %d", bounded_buffer[cBufferIndex], consumer->consumerId+1); // consumption
        
        dequeue_item();
        
        consumerCounter++;

        pthread_mutex_unlock(&m);
        sem_post(&empty);
    }

    pthread_exit(0);
}

void initializeData()
{
    pthread_attr_init(&attr);
    pthread_mutex_init(&m, NULL);
    
    bounded_buffer = calloc(buffer_cnt_max, sizeof(int));
   
    producerArray = (int *)malloc(sizeof(int) * (numOfProducer * max_each_produce));
    consumerArray = (int *)malloc(sizeof(int) * (numOfProducer * max_each_produce));//over consume
    
    producerThread = (pthread_t *)malloc(numOfProducer * sizeof(pthread_t));
    consumerThread = (pthread_t *)malloc(numOfConsumer * sizeof(pthread_t));

    producerInfo = (producerStruct *)malloc(numOfProducer * sizeof(producerStruct));
    consumerInfo = (consumerStruct *)malloc(numOfConsumer * sizeof(consumerStruct));
    
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, buffer_cnt_max);

    pthread_mutex_init(&m, NULL);
}

int main(int argc, char const *argv[])
{
    time_t curTime;
    struct timeval tvStart, tvEnd;
    int startTime, endTime;


    if (argc != 7)
    {
        fprintf(stderr, "invalid input");

        return EXIT_FAILURE;
    }

    curTime = time(NULL); 
    printf("\nCurrent time: %s\n", ctime(&curTime));
    
    buffer_cnt_max = atoi(argv[1]);
    printf("Number of Buffers : %d\n", buffer_cnt_max); // N
    numOfProducer = atoi(argv[2]);
    printf("Number of Producers : %d\n", numOfProducer); // P
    numOfConsumer = atoi(argv[3]);
    printf("Number of Consumers : %d\n", numOfConsumer); // C
    max_each_produce = atoi(argv[4]);
    printf("Number of items Produced by each producer : %d\n", max_each_produce); // X
    int each_consume = numOfProducer * max_each_produce / numOfConsumer;
    printf("Number of items consumed by each consumer : %d\n", each_consume);    
    if(numOfProducer * max_each_produce > each_consume * numOfConsumer)
        overConsume =1;
    printf("Over consume on? : %d\n", overConsume);
    printf("Over consume amount : %d\n", each_consume + 
            numOfProducer * max_each_produce - each_consume * numOfConsumer);
    pTime = atoi(argv[5]);
    printf("Time each Producer Sleeps (seconds) : %d\n", pTime);
    cTime = atoi(argv[6]);
    printf("Time each Consumer Sleeps (seconds) : %d\n", cTime);

    initializeData();

    gettimeofday(&tvStart, NULL); 
    startTime = tvStart.tv_sec;

    int i = 0;
    while (i < numOfProducer)
    {
        producerInfo[i].producerId = i;
        producerInfo[i].thread_num = numOfProducer;
        producerInfo[i].max_prod = max_each_produce;

        pthread_create(&producerThread[i], &attr, producer, &producerInfo[i]);

        i++;
    }

    i = 0;
    while (i < numOfConsumer)
    {

        consumerInfo[i].consumerId = i;
        consumerInfo[i].thread_num = numOfConsumer;
        //set over consume thread max_cons 
        if(i==0){
            consumerInfo[i].max_cons = each_consume + 
                    numOfProducer * max_each_produce - each_consume * numOfConsumer;
        }else{
            consumerInfo[i].max_cons = numOfProducer * max_each_produce / numOfConsumer;
        }
        pthread_create(&consumerThread[i], &attr, consumer, &consumerInfo[i]);

        i++;
    }
    for (int i = 0; i < numOfProducer; i++)
    {
        pthread_join(producerThread[i], NULL);
        printf("\nProducer Thread Joined:  %d", (i + 1));
    }

    for (int i = 0; i < numOfConsumer; i++)
    {
        pthread_join(consumerThread[i], NULL);
        printf("\nConsumer Thread Joined:  %d", (i + 1));
    }

    curTime = time(NULL); 

    printf("\nCurrent time: %s\n", ctime(&curTime));
    gettimeofday(&tvEnd, NULL);

    fprintf(stderr, "Producer Array  | Consumer Array\n");
    for (int i = 0; i < numOfProducer * max_each_produce; i++)
    {
        fprintf(stderr, "%-15d | %-15d\n", producerArray[i + 1], consumerArray[i + 1]);
    }

    for (int i = 1; i < numOfProducer * max_each_produce; i++){
        if (producerArray[i] != consumerArray[i]){
            match = 0;
            break;
        }
    }
    if(match==1){
        fprintf(stderr, "\nConsume and Produce Arrays Match!\n");
    }else{
        fprintf(stderr, "\nConsume and Produce Arrays Doesn't Match!\n");
    }

    endTime = tvEnd.tv_sec;
    printf("Total Runtime:: %i secs\n", (endTime - startTime));

    // cleanup

    free(bounded_buffer);
    free(producerThread);
    free(producerInfo);
    free(consumerThread);
    free(consumerInfo);

    sem_destroy(&full);
    sem_destroy(&empty);
    pthread_mutex_destroy(&m);

    return 0;
}
