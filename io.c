#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>                // for gettimeofday()
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>

#define KB                  (1024)
#define MB                  (1024 * KB)
#define GB                  (1024 * MB)

#define DISK_PATH           ("/dev/xvdl")
#define DISK_SIZE_BYTES     (5ULL * GB)
#define DIV_ROUND_UP(n,d)   (((n) + (d) - 1) / (d))
#define BYTES_TO_SEC(b)     (DIV_ROUND_UP(b, 512))
#define SEC_TO_BYTES(s)     (s * 512)
#define NUM_IO_PER_THREAD   (1000)
#define IO_BUFFER_SIZE      (4 * MB)

struct threadRecord
{
    double elapsedTime;
    uint64_t numIOCompleted;
};


struct threadIoCntx
{
    struct threadRecord tr;
    char *devPath;
    char *buffer;
    uint64_t ioSizeSectors;
    float percentRead;
};

static inline void diskWrite(int fd, uint64_t offset, ssize_t size, char *buffer)
{
    //printf("WRITE %lu %lu\n", offset, size);
    ssize_t amountWritten;

    while(size)
    {
        amountWritten = pwrite(fd, buffer, size, offset);
        assert(amountWritten >= 0);

        size -= amountWritten;
        offset += amountWritten;
        buffer += amountWritten;
    }
}

static inline void diskRead(int fd, uint64_t offset, uint64_t size, char *buffer)
{
    //printf("READ(%lu, %lu)\n", offset, size);
    ssize_t amountRead;

    while(size)
    {
        amountRead = pread(fd, buffer, size, offset);
        assert(amountRead >= 0);

        size -= amountRead;
        offset += amountRead;
        buffer += amountRead;
    }
}



static inline int openDisk(char *devPath)
{
    int fd = open(devPath, O_DIRECT|O_LARGEFILE|O_SYNC|O_RDWR);

    if(fd < 0)
    {
        printf("ERROR OPEN %s\n", strerror(errno));
    }

    return fd;
}

uint64_t getElapsedTimeUS(struct timespec *start, struct timespec *end)
{
    return ((end->tv_sec - start->tv_sec) * 1000000) + ((end->tv_nsec - start->tv_nsec) / 1000);
}

static inline uint64_t randomNumber(uint64_t min, uint64_t max)
{
    return (rand() % (max + 1 - min) + min);
}

void *execFunc(void *arg)
{
    struct timespec t1, t2;
    struct threadIoCntx *cntx = (struct threadIoCntx *)arg;
    int readCount = NUM_IO_PER_THREAD * cntx->percentRead;
    int writeCount = NUM_IO_PER_THREAD - readCount;

    //printf("S: %d %d %d\n", readCount, writeCount, NUM_IO_PER_THREAD);

    int fd = openDisk(cntx->devPath);
    assert(fd >= 0);

    srand(time(NULL));
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

    while(readCount)
    {
        uint64_t offset = SEC_TO_BYTES(randomNumber(0, BYTES_TO_SEC(DISK_SIZE_BYTES)));
        diskRead(fd, offset, SEC_TO_BYTES(cntx->ioSizeSectors), cntx->buffer);
        readCount--;
    }

    while(writeCount)
    {
        uint64_t offset = SEC_TO_BYTES(randomNumber(0, BYTES_TO_SEC(DISK_SIZE_BYTES)));
        diskWrite(fd, offset, SEC_TO_BYTES(cntx->ioSizeSectors), cntx->buffer);
        writeCount--;
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &t2);
    
    cntx->tr.elapsedTime = (double)getElapsedTimeUS(&t1, &t2);
    cntx->tr.numIOCompleted = NUM_IO_PER_THREAD;

    close(fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    if(argc != 5)
    {
        printf("Usage: ./randio <dev Path> <num threads> <io size> <percent read>\n");
        exit(0);
    }

    char *devPath = argv[1];
    char *numThreadStr = argv[2];
    char *ioSizeStr = argv[3];
    char *percentReadStr = argv[4];

    /*
    printf("devPath: %s\n", devPath);
    printf("numThread: %s | %d\n", numThreadStr, atoi(numThreadStr));
    printf("ioSize: %s | %lu\n", ioSizeStr, strtoul(ioSizeStr, NULL, 0));
    printf("percentRead: %s | %f\n", percentReadStr, strtof(percentReadStr, NULL));
    */

    int numThread = atoi(numThreadStr);
    uint64_t ioSize = strtoul(ioSizeStr, NULL, 0);
    float percentRead = strtof(percentReadStr, NULL);

    //make sure buffer size can handle the io size
    assert(IO_BUFFER_SIZE >= ioSize);

    void *buffer = NULL;
    int status = posix_memalign(&buffer, 512, IO_BUFFER_SIZE);
    assert(!status);
    assert(buffer);

    pthread_t threadIDs[numThread];
    struct threadIoCntx threadIOContexts[numThread];

    for(int i = 0; i < numThread; i++)
    {
        threadIOContexts[i].tr = {0, 0};
        threadIOContexts[i].devPath = devPath;
        threadIOContexts[i].buffer = (char *)buffer;
        threadIOContexts[i].ioSizeSectors = BYTES_TO_SEC(ioSize);
        threadIOContexts[i].percentRead = percentRead;
        pthread_create(&threadIDs[i], NULL, execFunc, (void *)&threadIOContexts[i]);
    }

    for (int i = 0; i < numThread; i++)
       pthread_join(threadIDs[i], NULL);

    double avgIOPS = 0;
    double threadIOPS = 0;

    for (int i = 0; i < numThread; i++)
    {
        threadIOPS = ((double)threadIOContexts[i].tr.numIOCompleted / threadIOContexts[i].tr.elapsedTime) * 1000000;
        avgIOPS += threadIOPS;
    }

    printf("%f", avgIOPS);
    
    return 0;
}

