//Elad Israel 313448888

#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#define SYS_ERR "Error in system call\n"
#define MAX_LEN 512
#define SLEEP_T 5000

typedef struct {
    pid_t jobPID;
    char jobName[MAX_LEN];
} Job;

void getLineFromUser(char *str, int *isAmper);

void deleteCompletedJobs(Job jobs[MAX_LEN]);

void compressJobsArr(Job jobs[MAX_LEN], short *jobsIndex);

void printJobsArr(Job jobs[MAX_LEN]);

void cdInput(char str[MAX_LEN], char previousWD[PATH_MAX], char *split[MAX_LEN], size_t sizeOfpreviousWD);


int main() {
    //string from user
    char str[MAX_LEN] = {};
    //each word is a different token
    char *token;
    //flag- if user entered '&' in one of the tokens- run in background
    int isAmper;
    //array of jobs(tasks running in the background)
    Job jobs[MAX_LEN] = {};
    short jobsIndex = 0;
    //save previous working directory
    char previousWD[PATH_MAX] = {};
    if (getcwd(previousWD, sizeof(previousWD)) == NULL) {
        fprintf(stderr, SYS_ERR);
    }
    getLineFromUser(str, &isAmper);

    while (strcmp(str, "exit") != 0) {

        //take care of empty input
        if (strcmp(str, "") == 0 || isspace(str[0])) {
            //get new command
            getLineFromUser(str, &isAmper);
            continue;
        }

            //take care of "jobs" input
        else if (strcmp(str, "jobs") == 0) {
            deleteCompletedJobs(jobs);
            compressJobsArr(jobs, &jobsIndex);
            printJobsArr(jobs);
            //get new command
            getLineFromUser(str, &isAmper);
            continue;
        }

        //divide the input to tokens - strtok modifies the original string so a copy is needed.
        char strCopy[MAX_LEN] = {};
        //array containing the split words
        char *split[MAX_LEN] = {};
        strcpy(strCopy, str);
        token = strtok(strCopy, " ");
        int i;
        for (i = 0; token != NULL; i++) {
            if (strcmp(token, "&") == 0) {
                isAmper = 1;
                break;
            }
            split[i] = token;
            //walk through other tokens(because NULL is passed as first parameter)
            token = strtok(NULL, " ");
        }

        //take care of "cd" input
        if (strcmp(split[0], "cd") == 0) {
            cdInput(str, previousWD, split, sizeof(previousWD));
            //get new command
            getLineFromUser(str, &isAmper);
            continue;
        }

        //create another process, than execvp the child, and wait/continue to the next command(with the father).
        pid_t forkRetVal = fork();

        if (forkRetVal == -1) {
            fprintf(stderr, SYS_ERR);
        } else if (forkRetVal == 0) { //child
            int execvpRet;
            //print the pid of the child
            printf("%ld\n", (long) getpid());
            fflush(stdout);
            execvpRet = execvp(split[0], split);
            if (execvpRet == -1) {
                fprintf(stderr, SYS_ERR);
                exit(2);
            }
        } else { //father- fork return value is the pid of the child
            //if ampersand was entered- job will run in background

            if (isAmper == 1) {
                //erase '&'
                for (i = 0; i < MAX_LEN; i++) {
                    if (str[i] == '&') {
                        str[i] = 0;
                    }
                }
                //insert job to jobs array
                jobs[jobsIndex].jobPID = forkRetVal;
                strcpy(jobs[jobsIndex].jobName, str);
                jobsIndex++;
            } else { //user didn't enter ampersand
                int status;
                int waitRet = wait(&status);
                if (waitRet == -1) {
                    fprintf(stderr, SYS_ERR);
                }
            }
        }
        //get new command
        getLineFromUser(str, &isAmper);
    }
    //print father's PID when "exit" is entered.
    printf("%ld\n", (long) getpid());
    return 0;
}

/**
 * get new input from the user, and reset ampersand value
 * @param str string to insert the input into.
 */
void getLineFromUser(char str[MAX_LEN], int *isAmper) {
    //reset isAmper
    *isAmper = 0;
    usleep(SLEEP_T);
    printf("> ");
    fgets(str, MAX_LEN, stdin);
    //get rid of new line character- returns a pointer to it.
    char *newline = strchr(str, '\n');
    if (newline) { *newline = 0; }
}

/**
 * search for already completed jobs and delete them from the jobs array.
 * @param jobs array
 */
void deleteCompletedJobs(Job jobs[MAX_LEN]) {
    int status;
    int i;
    for (i = 0; i < MAX_LEN; i++) {
        if (jobs[i].jobPID > 0) {
            int waitpidRet;
            waitpidRet = waitpid(jobs[i].jobPID, &status, WNOHANG);
            if (WIFEXITED(status) && waitpidRet > 0) { //child exited
                jobs[i].jobPID = 0;
                strcpy(jobs[i].jobName, "");
            }
        }
    }
}

/**
 * compress the jobs array so that only cells that contain a job will remain
 * @param jobs array
 * @param jobsIndex jobs array's index
 */
void compressJobsArr(Job jobs[MAX_LEN], short *jobsIndex) {
    Job temp[MAX_LEN] = {};
    //copy jobs array to temp;
    int i;
    for (i = 0; i < MAX_LEN; i++) {
        temp[i] = jobs[i];
    }
    //reset all jobs cells to 0.
    for (i = 0; i < MAX_LEN; i++) {
        jobs[i].jobPID = 0;
        strcpy(jobs[i].jobName, "");
    }
    //insert back only jobs that are still running
    short updatedJobsIndex = 0;
    for (i = 0; i < MAX_LEN; i++) {
        if (temp[i].jobPID != 0 && strcmp(temp[i].jobName, "") != 0) {
            jobs[updatedJobsIndex] = temp[i];
            updatedJobsIndex++;
        }
    }
    //update index
    *jobsIndex = updatedJobsIndex;
}

/**
 * prints the jobs array
 * @param jobs array to print
 */
void printJobsArr(Job jobs[MAX_LEN]) {
    int i;
    for (i = 0; i < MAX_LEN; i++) {
        if (jobs[i].jobPID != 0) {
            printf("%ld %s\n", (long) jobs[i].jobPID, jobs[i].jobName);
        }
    }
}

/**
 * Takes care of CD input- changes the working directory
 * @param str user's input
 * @param previousWD working directory before the user's command
 * @param split the split user's input
 * @param sizeOfpreviousWD size of the working directory before the user's command
 */
void cdInput(char str[MAX_LEN], char previousWD[PATH_MAX], char *split[MAX_LEN], size_t sizeOfpreviousWD) {
    printf("%ld\n", (long) getpid());
    int chdirRet;
    char previousWDCpy[PATH_MAX];
    strcpy(previousWDCpy, previousWD);
    if (split[1] == NULL || strcmp(split[1], "~") == 0) {
        //change previousWD
        if (getcwd(previousWD, sizeOfpreviousWD) == NULL) {
            fprintf(stderr, SYS_ERR);
        }
        chdirRet = chdir(getenv("HOME"));
    } else if (strcmp(split[1], "-") == 0) {
        //save previous working directory
        char currentWD[PATH_MAX];
        if (getcwd(currentWD, sizeof(currentWD)) == NULL) {
            fprintf(stderr, SYS_ERR);
        }
        chdirRet = chdir(previousWD);
        strcpy(previousWD, currentWD);
    } else { //a path was entered
        //change previousWD
        if (getcwd(previousWD, sizeOfpreviousWD) == NULL) {
            fprintf(stderr, SYS_ERR);
        }
        //combine arguments in case there were spaces in the path
        char *quotes = strchr(str, '"');
        if (quotes != NULL) {
            int quotesIndex = (int) (quotes - str);
            char path[MAX_LEN] = {};
            //cut path without quotes: strncpy(dest, src + beginIndex, endIndex - beginIndex);
            strncpy(path, str + quotesIndex + 1, strlen(str) - 1 - (quotesIndex + 1));
            chdirRet = chdir(path);
        } else {
            chdirRet = chdir(split[1]);
        }
    }
    //cd failed
    if (chdirRet == -1) {
        fprintf(stderr, SYS_ERR);
        //restore last WD
        strcpy(previousWD, previousWDCpy);
    }
}