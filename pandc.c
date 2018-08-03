#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <sys/time.h>
#include <math.h>

int *buffer;

// Used for comparing arrays
int producerCounter = 1;
int consumerCounter = 1;

int buffer_capacity = 0;
int head = 0;
int itemsInBuffer = 1;
int buffer_size = 0;

// Values for arguments
int bufferAmt;
int numOfProducer;
int numOfConsumer;
int consumedItems;
int pTime;
int cTime;

int count_items = 1;

// Values use to check for
int cBufferIndex = 0;
int pBufferIndex = 0;

////////////////////
int in, out = 0;

// testing purposes
int *producerArray;
int *consumerArray;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // creating a mutex that is ready to be locked!

sem_t full;
sem_t empty;

typedef struct producerInit // for a more flexible locking primitive
{
    int producerId;
    int totalProducerTime;
    int producerFillCount;
    int producerRemainCount;

} producers;

typedef struct consumerInit
{
    int consumerId;
    int totalConsumerTime;
    int consumerFillCount;
    int consumerRemainCount;

} consumers;

void *producer(void *param)
{
    producers *producer = (producers *)param;
    int capacity = producer->producerFillCount;
    int prodId = producer->producerId;

    int count = 0;

    while (count++ < capacity)
    {
        // sleep(producer->totalProducerTime);
        sem_wait(&empty);
        pthread_mutex_lock(&m);

        printf("\n%d was produced by thread-> %d", count_items, prodId + 1);

        //enqueue item in buffer
        // buffer[pBufferIndex] = count_items;
        producerArray[producerCounter] = count_items;

        //enqueue item in buffer
        buffer[pBufferIndex] = count_items;
        // count++;
        count_items++;
        producerCounter++;
        buffer_capacity++;

        // iterate through the proudcer buffer until reached N size buffers
        if (pBufferIndex == (bufferAmt - 1))
        {
            pBufferIndex = 0; // reset
        }
        else
        {
            pBufferIndex++;
        }

        sleep(producer->totalProducerTime);

        pthread_mutex_unlock(&m);
        sem_post(&full);
    }

    pthread_exit(0);
}

void *consumer(void *param)
{
    consumers *consumer = (consumers *)param;
    int capacity = consumer->consumerFillCount;
    int cmsID = consumer->consumerId;

    int count = 0;

    while (count++ < capacity)
    {
        sleep(consumer->totalConsumerTime);
        sem_wait(&full);
        pthread_mutex_lock(&m);

        // set the cBufferIndex = dequeue();

        // prints item and consume
        printf("\n%d is consumed by thread-> %d", buffer[cBufferIndex], cmsID + 1); // consumption

        consumerArray[consumerCounter] = buffer[cBufferIndex];
        consumerCounter++;

        // iterate through the consumer buffer until reached N size buffers

        if (cBufferIndex == (bufferAmt - 1))
        {
            cBufferIndex = 0; // reset to 0
        }
        else
        {
            cBufferIndex++;
        }

        // sleep(consumer->totalConsumerTime);

        pthread_mutex_unlock(&m);
        sem_post(&empty);
    }

    pthread_exit(0);
}

int main(int argc, char const *argv[])
{
    time_t tic, toc;                // Display time
    struct timeval tvBegin, tvStop; // Track time duration
    int tStart, tEnd;

    int threadErrorCode;
    int readIndex;

    pthread_t *producerThread;
    pthread_t *consumerThread;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_mutex_init(&m, NULL);

    // sem_init(&full, 0, 0);
    // sem_init(&empty, 0, bufferAmt);

    producers *producerInfo;
    consumers *consumerInfo;

    // Command prompt with valid use
    if (argc != 7)
    {
        fprintf(stderr, "Valid Usage: ./pandc nb np nc x pTime cTime\n"
                        "nb = number of buffers\n"
                        "np = number of producer\n"
                        "nc = number of consumers\n"
                        "x = number of items produced by each producer thread\n"
                        "pTime = time sent busy waiting between PRODUCED item in SECONDS\n"
                        "cTime = time sent busy waiting between CONSUMED item in SECONDS\n");

        return EXIT_FAILURE;
    }

    // Parsing commandline arguments
    bufferAmt = atoi(argv[1]);
    printf("N = %d ", bufferAmt); // N
    numOfProducer = atoi(argv[2]);
    printf("P = %d ", numOfProducer); // P
    numOfConsumer = atoi(argv[3]);
    printf("C = %d ", numOfConsumer); // C
    itemsInBuffer = atoi(argv[4]);
    printf("X = %d ", itemsInBuffer); // X
    // Calculates (PSize*X)/CSize
    consumedItems = (numOfProducer * itemsInBuffer / numOfConsumer);

    pTime = atoi(argv[5]);
    printf("Ptime = %d ", pTime);
    cTime = atoi(argv[6]);
    printf("Ctime = %d\n", cTime);

    // Allocates Buffer
    buffer = calloc(bufferAmt, sizeof(int));
    // buffer = (int *)malloc(sizeof(int) * bufferAmt);
    producerArray = (int *)malloc(sizeof(int) * (numOfProducer * itemsInBuffer));
    consumerArray = (int *)malloc(sizeof(int) * (numOfConsumer * consumedItems));
    // allocating memory for producer, consumer thread

    producerThread = (pthread_t *)malloc(numOfProducer * sizeof(pthread_t));

    if (producerThread == NULL)
    {
        fprintf(stderr, "ERROR: malloc failed for producerThread");
        return EXIT_FAILURE;
    }

    consumerThread = (pthread_t *)malloc(numOfConsumer * sizeof(pthread_t));
    if (consumerThread == NULL)
    {
        fprintf(stderr, "ERROR: malloc failed for consumerThread");
        return EXIT_FAILURE;
    }

    // allocating memory for producerInfo, consumerInfo
    producerInfo = (producers *)malloc(numOfProducer * sizeof(producers));
    if (producerInfo == NULL)
    {
        fprintf(stderr, "ERROR: malloc failed for producerInfo");
        return EXIT_FAILURE;
    }

    consumerInfo = (consumers *)malloc(numOfConsumer * sizeof(consumers));
    if (consumerInfo == NULL)
    {
        fprintf(stderr, "ERROR: malloc failed for consumerInfo");
        return EXIT_FAILURE;
    }

    // Semaphore

    sem_init(&full, 0, 0);
    sem_init(&empty, 0, bufferAmt);

    // Initiating mutex
    threadErrorCode = pthread_mutex_init(&m, NULL);
    if (threadErrorCode)
    {
        fprintf(stderr, "ERROR: pthread_mutex_init return code is: %d\n", threadErrorCode);
        return EXIT_FAILURE;
    }

    // Time Tracking Starts Here
    tic = time(NULL); // Used to display current time
    printf("\nStart time: %s\n", ctime(&tic));

    gettimeofday(&tvBegin, NULL); // used to calculate execution duration
    tStart = tvBegin.tv_sec;

    // setting up producer details
    readIndex = 0;
    while (readIndex < numOfProducer)
    {
        producerInfo[readIndex].producerId = readIndex;
        producerInfo[readIndex].producerRemainCount = bufferAmt;
        producerInfo[readIndex].producerFillCount = itemsInBuffer;
        producerInfo[readIndex].totalProducerTime = pTime;

        threadErrorCode = pthread_create(&producerThread[readIndex], &attr, producer, &producerInfo[readIndex]);
        if (threadErrorCode != 0)
        {
            fprintf(stderr, "ERROR: producer pthread_create return code is: %d\n", threadErrorCode);
            return EXIT_FAILURE;
        }

        readIndex++;
    }

    // setting up consumer details
    readIndex = 0;
    while (readIndex < numOfConsumer)
    {

        consumerInfo[readIndex].consumerId = readIndex;
        consumerInfo[readIndex].consumerRemainCount = bufferAmt;
        consumerInfo[readIndex].consumerFillCount = consumedItems;
        consumerInfo[readIndex].totalConsumerTime = cTime;

        threadErrorCode = pthread_create(&consumerThread[readIndex], &attr, consumer, &consumerInfo[readIndex]);
        if (threadErrorCode != 0)
        {
            fprintf(stderr, "ERROR: consumer pthread_create return code is: %d\n", threadErrorCode);
            return EXIT_FAILURE;
        }

        readIndex++;
    }
    for (int i = 0; i < numOfProducer; i++)
    {
        threadErrorCode = pthread_join(producerThread[i], NULL);
        printf("\nProducer Thread Joined:  %d", (i + 1));

        if (threadErrorCode != 0)
        {
            fprintf(stderr, "ERROR: producer pthread_join return code is: %d\n", threadErrorCode);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < numOfConsumer; i++)
    {
        threadErrorCode = pthread_join(consumerThread[i], NULL);
        printf("\nConsumer Thread Joined:  %d", (i + 1));

        if (threadErrorCode != 0)
        {
            fprintf(stderr, "ERROR: producer pthread_join return code is: %d\n", threadErrorCode);
            return EXIT_FAILURE;
        }
    }

    // Waiting for the producer tread to finish/join
    readIndex = 0;
    while (readIndex < numOfProducer)
    {

        if (threadErrorCode != 0)
        {
            fprintf(stderr, "ERROR: producer pthread_join return code is: %d\n", threadErrorCode);
            return EXIT_FAILURE;
        }

        readIndex++;
    }

    // Waiting for the consumer tread to finish/join
    readIndex = 0;
    while (readIndex < numOfConsumer)
    {
        // threadErrorCode = pthread_join(consumerThread[readIndex], NULL);
        // printf("\nConsumer Thread Joined:  %d", (readIndex + 1));

        if (threadErrorCode != 0)
        {
            fprintf(stderr, "ERROR: consumer pthread_join return code is: %d\n", threadErrorCode);
            return EXIT_FAILURE;
        }

        readIndex++;
    }

    // Time Tracking Ends Here
    toc = time(NULL); // Used to display current time'

    printf("\nEnd time: %s\n", ctime(&toc));
    // Used to calculate duration
    gettimeofday(&tvStop, NULL);
    //

    // Lets check if they match {check_output}
    fprintf(stderr, "Producer Array  | Consumer Array\n");
    for (int i = 0; i < numOfProducer * itemsInBuffer; i++)
    {
        fprintf(stderr, "%-15d | %-15d\n", producerArray[i + 1], consumerArray[i + 1]);
    }

    if (producerCounter == consumerCounter)
    {
        fprintf(stderr, "\nConsumer and Produce Arrays Match!\n");
    }
    else
    {
        fprintf(stderr, "\nConsume and Produce Arrays Didn't MAtch!\n");
    }

    tEnd = tvStop.tv_sec;
    printf("Duration of Execution: %i secs\n", (tEnd - tStart));

    // // Memory  and thread cleanup

    free(buffer);
    free(producerThread);
    free(producerInfo);
    free(consumerThread);
    free(consumerInfo);

    sem_destroy(&full);
    sem_destroy(&empty);
    pthread_mutex_destroy(&m);

    return EXIT_SUCCESS;
}
