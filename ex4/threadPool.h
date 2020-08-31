//Elad Israel 313448888

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <stdbool.h>
#include <sys/types.h>
#include "osqueue.h"

typedef struct task_struct
{
    void (*computeFunc) (void *);
    void* param;
}Task;

typedef struct thread_pool
{
    pthread_t* threadsArr;
    OSQueue* tasksQueue;
    pthread_mutex_t queueMutex;
    pthread_cond_t pthreadCondVar;
    pthread_mutex_t CondVarMutex;
    int numOfThreads;
    volatile bool runAdditionalTasks;
}ThreadPool;


ThreadPool* tpCreate(int numOfThreads);

void* executeTasks(void* threadPoolVoid);

void executeSingleTask(ThreadPool *threadPool);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

void mallocCheck(void *obj,ThreadPool *threadPool);

void testRetVal(int retVal,ThreadPool *threadPool);

void freeAllocated(ThreadPool *threadPool);

#endif
