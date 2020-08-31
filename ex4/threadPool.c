//Elad Israel 313448888
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include "threadPool.h"

#define STDERR 2
#define FAILURE -1
#define SUCCESS 0
#define SYSCALL_ERR_MSG "Error in system call\n"
#define MALLOC_ERR_MSG "malloc failed\n"

volatile bool isBeingDestroyed;

/**
 * Create a new threadpool.
 * @param numOfThreads num of threads in the threadpool
 * @return pointer to ThreadPool struct
 */
ThreadPool *tpCreate(int numOfThreads) {
    //create threadpool
    ThreadPool *threadPool = malloc(sizeof(ThreadPool));
    mallocCheck(threadPool, threadPool);
    //create threadpool's fields
    threadPool->numOfThreads = numOfThreads;
    threadPool->threadsArr = malloc(sizeof(pthread_t) * numOfThreads);
    mallocCheck(threadPool->threadsArr, threadPool);
    threadPool->tasksQueue = osCreateQueue();
    int i;
    for (i = 0; i < numOfThreads; i++) {
        testRetVal(pthread_create(&threadPool->threadsArr[i], NULL, executeTasks, threadPool), threadPool);
    }
    isBeingDestroyed = false;
    testRetVal(pthread_mutex_init(&threadPool->queueMutex, NULL), threadPool);
    testRetVal(pthread_mutex_init(&threadPool->CondVarMutex, NULL), threadPool);
    testRetVal(pthread_cond_init(&threadPool->pthreadCondVar, NULL), threadPool);
    threadPool->runAdditionalTasks = true;
    return threadPool;
}

/**
 * Destroy the threadPool and release it's memory.
 * after calling this function, no more tasks can be added to the threadPool.
 * @param threadPool
 * @param shouldWaitForTasks 0- only wait for currently running tasks. !0 -also wait for all tasks in queue.
 */
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    if (isBeingDestroyed) {
        return;
    }
    isBeingDestroyed = true;

    //if shouldWaitForTasks is 0 - wait only for the running tasks to finish
    if (shouldWaitForTasks == 0) {
        threadPool->runAdditionalTasks = false;
    }
    //wake all threads so they can finish
    testRetVal(pthread_cond_broadcast(&threadPool->pthreadCondVar), threadPool);
    //wait for all threads to finish
    int i;
    for (i = 0; i < threadPool->numOfThreads; i++) {
        testRetVal(pthread_join(threadPool->threadsArr[i], NULL), threadPool);
    }
    freeAllocated(threadPool);
}

/**
 * free all allocated objects in the threadpool.
 * @param threadPool
 */
void freeAllocated(ThreadPool *threadPool) {
    if (threadPool == NULL) {
        return;
    }   //threadPool != NULL
    if (threadPool->tasksQueue != NULL) {
        while (!osIsQueueEmpty(threadPool->tasksQueue)) {
            if (pthread_mutex_lock(&threadPool->queueMutex) != 0) {
                write(STDERR, SYSCALL_ERR_MSG, strlen(SYSCALL_ERR_MSG));
                _exit(FAILURE);
            }
            free(osDequeue(threadPool->tasksQueue));
            if (pthread_mutex_unlock(&threadPool->queueMutex) != 0) {
                write(STDERR, SYSCALL_ERR_MSG, strlen(SYSCALL_ERR_MSG));
                _exit(FAILURE);
            }
        }
        osDestroyQueue(threadPool->tasksQueue);
    }
    if (threadPool->threadsArr != NULL) {
        free(threadPool->threadsArr);
    }
    free(threadPool);
}

/**
 * Insert a task to the threadpool.
 * @param threadPool
 * @param computeFunc task's function to run
 * @param param computeFunc's parameters
 * @return 0 successful, -1 failed(if threadpool is being destroyed).
 */
int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    //if threadpool is being destroyed - cannot insert more tasks
    if (isBeingDestroyed) {
        return FAILURE;
    }
    //create and insert the task
    Task *task = malloc(sizeof(Task));
    mallocCheck(task, threadPool);
    task->param = param;
    task->computeFunc = computeFunc;
    //insertion
    testRetVal(pthread_mutex_lock(&threadPool->queueMutex), threadPool);
    osEnqueue(threadPool->tasksQueue, task);
    testRetVal(pthread_mutex_unlock(&threadPool->queueMutex), threadPool);
    //alert a blocked thread that a new task was inserted so it will execute the task
    testRetVal(pthread_cond_signal(&threadPool->pthreadCondVar), threadPool);
    return SUCCESS;
}

/**
 * Check malloc's return value(returns NULL on failure).
 * @param obj malloc return val
 * @param threadPool
 */
void mallocCheck(void *obj, ThreadPool *threadPool) {
    if (obj == NULL) {
        write(STDERR, MALLOC_ERR_MSG, strlen(MALLOC_ERR_MSG));
        freeAllocated(threadPool);
        _exit(FAILURE);
    }
}

/**
 * Check function's return value(returns -1 on failure).
 * @param retVal return val
 * @param threadPool
 */
void testRetVal(int retVal, ThreadPool *threadPool) {
    if (retVal != 0) {
        write(STDERR, SYSCALL_ERR_MSG, strlen(SYSCALL_ERR_MSG));
        freeAllocated(threadPool);
        _exit(FAILURE);
    }
}

/**
 * the threadpool threads' function.
 * Will execute the tasks inserted.
 * @param threadPoolVoid threadPool as void
 * @return NULL
 */
void *executeTasks(void *threadPoolVoid) {
    ThreadPool *threadPool = threadPoolVoid;
    //while there are tasks to run and you should run more tasks
    while (!osIsQueueEmpty(threadPool->tasksQueue) && threadPool->runAdditionalTasks) {
        //if there are tasks in queue, take one and run it
        executeSingleTask(threadPool);
        //if there aren't tasks in queue, and the threadpool is still active - wait until one is inserted(and wake you)
        if (osIsQueueEmpty(threadPool->tasksQueue) && !isBeingDestroyed) {
            testRetVal(pthread_mutex_lock(&threadPool->CondVarMutex), threadPool);
            testRetVal(pthread_cond_wait(&threadPool->pthreadCondVar, &threadPool->CondVarMutex), threadPool);
            testRetVal(pthread_mutex_unlock(&threadPool->CondVarMutex), threadPool);
        }
    }
    return NULL;
}

/**
 * Executes a single task.
 * @param threadPool
 */
void executeSingleTask(ThreadPool *threadPool){
    testRetVal(pthread_mutex_lock(&threadPool->queueMutex), threadPool);
    Task *task = osDequeue(threadPool->tasksQueue);
    testRetVal(pthread_mutex_unlock(&threadPool->queueMutex), threadPool);
    task->computeFunc(task->param);
    free(task);
}