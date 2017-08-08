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

#define KB				(1024)
#define MB				(1024 * KB)
#define GB				(1024 * MB)

#define DISK_PATH		("/dev/xvdl")
#define NUM_THREADS		(8)
#define DISK_SIZE_BYTES	(5ULL * GB)
#define BYTES_TO_SEC(b)	(b / 512)
#define IO_SIZE_SEC		(1)
#define SEC_TO_BYTES(s) (s * 512)
#define NUM_IO_PER_THREAD	(1000)

struct threadRecord
{
	double elapsedTime;
	uint64_t numIOCompleted;
};

struct threadRecord threadRecords[NUM_THREADS];

static inline int writeToDisk(int fd, uint64_t offset, uint64_t size, char *buffer)
{
	//printf("WRITE %lu %lu\n", offset, size);

	ssize_t amnt = pwrite(fd, buffer, size, offset);

	if(amnt != size)
	{
		printf("ERROR WRITE %s %d %ld\n", strerror(errno), fd, amnt);
	}

	return amnt;
}

static inline int openDisk(char *diskPath)
{
	//int fd = open(diskPath, O_DIRECT|O_LARGEFILE|O_SYNC|O_RDWR);
	int fd = open(diskPath, O_LARGEFILE|O_RDWR);

	if(fd < 0)
	{
		printf("ERROR OPEN %s\n", strerror(errno));
	}

	return fd;
}

static inline uint64_t randomNumber(uint64_t min, uint64_t max)
{
	return (rand() % (max + 1 - min) + min);
}

void *execFunc(void *arg)
{
	struct timeval t1, t2;
    double elapsedTime;

	int count = NUM_IO_PER_THREAD;
	char *buf = pvalloc(SEC_TO_BYTES(IO_SIZE_SEC));

	int fd = openDisk(DISK_PATH);

	srand ( time(NULL) );

	gettimeofday(&t1, NULL);
	
	while(count)
	{
		uint64_t offset = SEC_TO_BYTES(randomNumber(0, BYTES_TO_SEC(DISK_SIZE_BYTES)));
		writeToDisk(fd, offset, SEC_TO_BYTES(IO_SIZE_SEC), buf);
		//writeToDisk(fd, 0, 4096, buf);
		count--;
	}

	gettimeofday(&t2, NULL);
	
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    //printf("elapsedTime: %f ms\n", elapsedTime);

    struct threadRecord *tr = (struct threadRecord *)arg;
    tr->elapsedTime = elapsedTime;
    tr->numIOCompleted = NUM_IO_PER_THREAD;

	close(fd);
	return NULL;
}

int main()
{
	pthread_t threadIDs[NUM_THREADS];

	for(int i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&threadIDs[i], NULL, execFunc, (void *)&threadRecords[i]);
	}

    for (int i = 0; i < NUM_THREADS; i++)
       pthread_join(threadIDs[i], NULL);

   	double avgIOPS = 0;
   	double threadIOPS = 0;

   	for (int i = 0; i < NUM_THREADS; i++)
   	{
   		threadIOPS = ((double)threadRecords[i].numIOCompleted / threadRecords[i].elapsedTime) * 1000;
   		avgIOPS += threadIOPS;
   	}

   	printf("%f", avgIOPS);
	
	return 0;
}

