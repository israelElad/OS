//Elad Israel 313448888

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>

#define MAX_LEN 151
#define ERROR -1
#define STDERR 2

void handleSpecificDirectory(char *directoryInMainFolderPath, char *stuName, int resultsFileFD, char *inputPath,
                             char *correctOutputPath, bool *isCFileFound);

void iterateAllItemsInMainFolder(char *mainFolderPath, char *inputPath, char *correctOutputPath);

int compileCFile(char filePath[MAX_LEN]);

void gradeStuInResults(char *stuName, char *grade, char *cause, int resultsFileFD);

int executeCFile(char *inputPath);

int compareStuOutputToCorrectOutput(char *correctOutputPath);

/**
 * reads the config file, then extracts the 3 path lines it contains,
 * and calls the handling function.
 * @param argc num of args
 * @param argv path to config file
 * @return 0 success
 */
int main(int argc, char *argv[]) {

    //expecting a path for config file (and fileName as argv[0])
    if (argc != 2) {
        write(STDERR, "Invalid arguments!\n",strlen("Invalid arguments!\n"));
        exit(ERROR);
    }
    char pathOfConfFile[MAX_LEN];
    strcpy(pathOfConfFile, argv[1]);
    //open configurations file
    int fileFD = open(pathOfConfFile, O_RDONLY);
    if (fileFD == ERROR) {
        write(STDERR, "Couldn't open file\n",strlen("Couldn't open file\n"));
        close(fileFD);
        exit(ERROR);
    }
    char fileContent[MAX_LEN * 3] = {};
    //read 151*3 characters(max characters of 3 line)
    ssize_t bytesRead = read(fileFD, fileContent, MAX_LEN * 3);
    if (bytesRead == ERROR) {
        write(STDERR, "Read error\n",strlen("Read error\n"));
        close(fileFD);
        exit(ERROR);
    }

    /**
     * split the 3 lines to the appropriate variables
     * subfoldersPath- students dir
     * inputPath- input file for all students c files
     * outputPath- currectOutput file from all students c files
     */

    char *mainFolderPath = strtok(fileContent, "\n");
    char *inputPath = strtok(NULL, "\n");
    char *correctOutputPath = strtok(NULL, "\n");
    if (mainFolderPath == NULL || inputPath == NULL || correctOutputPath == NULL) {
        write(STDERR, "invalid config file!\n",strlen("invalid config file!\n"));
    }

    iterateAllItemsInMainFolder(mainFolderPath, inputPath, correctOutputPath);

    if (close(fileFD) == ERROR) {
        write(STDERR, "Error closing file\n",strlen("Error closing file\n"));
        exit(ERROR);
    }
}

/**
 * iterate over all folders in main folder- "students", and for each folder call the specific folder handler function.
 * @param mainFolderPath path to main folder containing the subfolders
 * @param inputPath path to users' input files.
 * @param correctOutputPath path to the correct output the users are supposed to do.
 */
void iterateAllItemsInMainFolder(char *mainFolderPath, char *inputPath, char *correctOutputPath) {
    //pointer to main folder
    DIR *mainFolder;
    if ((mainFolder = opendir(mainFolderPath)) == NULL) {
        write(STDERR, "Error opening directory\n",strlen("Error opening directory\n"));
        exit(ERROR);
    }
    //file to write the output of the students into
    int resultsFileFD = open("results.csv", O_WRONLY | O_CREAT, 0777 | O_APPEND);
    if (resultsFileFD == ERROR) {
        write(STDERR, "couldn't create a file\n",strlen("couldn't create a file\n"));
        closedir(mainFolder);
        exit(ERROR);
    }

    // looping through the directories inside the main directory(subfolders) = students names
    struct dirent *directoriesInMainFolder = readdir(mainFolder);
    while (directoriesInMainFolder != NULL) {
        //create directoryInMainFolderPath - main path+ "/" +directory name
        char directoryInMainFolderPath[MAX_LEN] = {};
        char dirName[MAX_LEN] = {};
        strcpy(dirName, directoriesInMainFolder->d_name);
        strcpy(directoryInMainFolderPath, mainFolderPath);
        strcat(directoryInMainFolderPath, "/");
        strcat(directoryInMainFolderPath, dirName); //specific directory name
        bool isCFileFound = 0;
        //handles a student
        if (strcmp(dirName, "..") != 0 && strcmp(dirName, ".") != 0) {
            handleSpecificDirectory(directoryInMainFolderPath, dirName, resultsFileFD, inputPath, correctOutputPath,
                                    &isCFileFound);
            if (!isCFileFound) { //c file wasn't found
                gradeStuInResults(dirName, "0", "NO_C_FILE", resultsFileFD);
            }
        }
        //next student
        directoriesInMainFolder = readdir(mainFolder);
    }

    if (close(resultsFileFD) == ERROR) {
        write(STDERR, "Error closing file\n",strlen("Error closing file\n"));
        exit(ERROR);
    }
    if (closedir(mainFolder) == ERROR) {
        write(STDERR, "Error closing dir\n",strlen("Error closing dir\n"));
        exit(ERROR);
    }
}

/**
 * recursive function which handles a specific student.
 * enters each folder inside the student's folder and searches for a c file, then grades accordingly.
 * @param directoryInMainFolderPath main dir
 * @param stuName student name
 * @param resultsFileFD results file to write the results of the student into.
 * @param inputPath path to users' input files.
 * @param correctOutputPath path to the correct output the users are supposed to do.
 * @param isCFileFound bool was c found for the student
 */
void handleSpecificDirectory(char *directoryInMainFolderPath, char *stuName, int resultsFileFD, char *inputPath,
                             char *correctOutputPath, bool *isCFileFound) {
    //pointer to main folder
    DIR *dirInMainFolder;
    if ((dirInMainFolder = opendir(directoryInMainFolderPath)) == NULL) {
        write(STDERR, "Error opening directory\n",strlen("Error opening directory\n"));
        exit(ERROR);
    }
    // looping through all items inside the specific directory(subfolder) = student name
    struct dirent *itemInDirectory = readdir(dirInMainFolder);

    //handles a student's files/folders
    while (itemInDirectory != NULL) {
        struct stat statusOfItem;
        char *itemName = itemInDirectory->d_name;
        //create item's Path - main path+ "/" +item name
        char itemPath[MAX_LEN] = {};
        strcpy(itemPath, directoryInMainFolderPath);
        strcat(itemPath, "/");
        strcat(itemPath, itemName);

        //declare stat so we can ask for details about the item
        if (stat(itemPath, &statusOfItem) == ERROR) {
            write(STDERR, "Error in stat\n",strlen("Error in stat\n"));
            closedir(dirInMainFolder);
            exit(ERROR);
        }
        if (S_ISREG(statusOfItem.st_mode)) {//item is a regular file
            //check if file is a 'c' file
            if (itemName[strlen(itemName) - 2] == '.' && itemName[strlen(itemName) - 1] == 'c' &&
                itemName[strlen(itemName)] == '\0') {
                //compile the file
                int isCompiled = compileCFile(itemPath);
                //compilation failed`
                if (!isCompiled) {
                    gradeStuInResults(stuName, "20", "COMPILATION_ERROR", resultsFileFD);
                }
                    //execute file
                else {
                    if (!executeCFile(inputPath)) {
                        gradeStuInResults(stuName, "40", "TIMEOUT", resultsFileFD);
                    } else { //compare outputs
                        int comparingRetVal = compareStuOutputToCorrectOutput(correctOutputPath);
                        if (comparingRetVal == 2) { //student's output different from correct output
                            gradeStuInResults(stuName, "60", "BAD_OUTPUT", resultsFileFD);
                        } else if (comparingRetVal == 3) { //student's output similar to the correct output
                            gradeStuInResults(stuName, "80", "SIMILAR_OUTPUT", resultsFileFD);
                        } else if (comparingRetVal == 1) { //student's output is identical to the correct output
                            gradeStuInResults(stuName, "100", "GREAT_JOB", resultsFileFD);
                        }
                    }
                    if (unlink("studentOutput.txt") == ERROR) {
                        write(STDERR, "couldn't delete a file\n",strlen("couldn't delete a file\n"));
                        closedir(dirInMainFolder);
                        exit(ERROR);
                    }
                }
                *isCFileFound = true;
            }

        } else if (S_ISDIR(statusOfItem.st_mode)) {//item is a directory
            //ignore links
            if (strcmp(itemName, "..") != 0 && strcmp(itemName, ".") != 0) {
                handleSpecificDirectory(itemPath, stuName, resultsFileFD, inputPath, correctOutputPath, isCFileFound);
            }
        }
        //move on to the next item
        itemInDirectory = readdir(dirInMainFolder);
    }
    if (closedir(dirInMainFolder) == ERROR) {
        write(STDERR, "Error closing dir\n",strlen("Error closing dir\n"));
        exit(ERROR);
    }
}

/**
 * using comp.out from the first part of ex3 to compare the student's output to the correct one.
 * @param correctOutputPath path to correct one.
 * @return identical: 1, similar: 3, different: 2
 */
int compareStuOutputToCorrectOutput(char *correctOutputPath) {
    //create another process, than execvp the child.
    pid_t forkRetVal = fork();
    if (forkRetVal == ERROR) {
        write(STDERR, "forking failed\n",strlen("forking failed\n"));
        exit(ERROR);
    } else if (forkRetVal == 0) { //child
        char *inputPaths[MAX_LEN] = {};
        inputPaths[0] = "./comp.out";
        inputPaths[1] = correctOutputPath;
        inputPaths[2] = "studentOutput.txt";
        //execute student's program
        if (execvp("./comp.out", inputPaths) == ERROR) {
            write(STDERR, "execution failed\n",strlen("execution failed\n"));
            exit(ERROR);
        }
    } else {
        int status;
        //wait for the child(same group)
        if (waitpid(forkRetVal, &status, 0) == ERROR) {
            write(STDERR, "waitpid failed\n",strlen("waitpid failed\n"));
            exit(ERROR);
        }
        //return comparison result
        return WEXITSTATUS(status);
    }
}

/**
 * execute the c file found in the student's folder.
 * @param inputPath path to the input for the c file
 * @return 1 if child finished within 5 sec, 0 if wasn't
 */
int executeCFile(char *inputPath) {
    //create another process, than execvp the child.
    pid_t forkRetVal = fork();
    if (forkRetVal == ERROR) {
        write(STDERR, "forking failed\n",strlen("forking failed\n"));
        exit(ERROR);
    } else if (forkRetVal == 0) { //child -execute file

        //use inputFile as input to the student's program
        int inputPathFD = open(inputPath, O_RDONLY);
        if (inputPathFD == ERROR) {
            write(STDERR, "couldn't create a file\n",strlen("couldn't create a file\n"));
            exit(ERROR);
        }
        //everything that will be asked from stdin(fd=0) - get from the file(inputFileFD)
        if (dup2(inputPathFD, 0) == ERROR) {
            write(STDERR, "dup2 failed\n",strlen("dup2 failed\n"));
            close(inputPathFD);
            exit(ERROR);
        }
        //close oldFD
        if (close(inputPathFD) == ERROR) {
            write(STDERR, "closing file failed\n",strlen("closing file failed\n"));
            exit(ERROR);
        }

        //file to write the output of the student into
        int outputFileFD = open("studentOutput.txt", O_WRONLY | O_CREAT, 0777);
        if (outputFileFD == ERROR) {
            write(STDERR, "couldn't create a file\n",strlen("couldn't create a file\n"));
            exit(ERROR);
        }
        //everything that will be written to stdout(fd=1) - write to the file(outputFileFD)
        if (dup2(outputFileFD, 1) == ERROR) {
            write(STDERR, "dup2 failed\n",strlen("dup2 failed\n"));
            close(outputFileFD);
            exit(ERROR);
        }
        //close oldFD
        if (close(outputFileFD) == ERROR) {
            write(STDERR, "closing file failed\n",strlen("closing file failed\n"));
            exit(ERROR);
        }
        char *args[MAX_LEN] = {"./a.out", NULL};
        if (execvp(args[0], args) == ERROR) {
            write(STDERR, "executing failed\n",strlen("executing failed\n"));
            exit(ERROR);
        }

    } else { //father
        int status;
        //wait 5 second for the child to see if it finished
        sleep(5);
        int waitRetVal = waitpid(forkRetVal, &status, WNOHANG);
        if (waitRetVal == ERROR) {
            write(STDERR, "waitpid failed\n",strlen("waitpid failed\n"));
            exit(ERROR);
        } else if (waitRetVal == 0) { //not finished
            return 0;
        }
        //waitpid returns pid of the child- child finished within 5 sec
        return 1;
    }
}

/**
 * grade the student
 * @param stuName student name
 * @param grade grade
 * @param cause reason for grade
 * @param resultsFileFD results file to write into
 */
void gradeStuInResults(char *stuName, char *grade, char *cause, int resultsFileFD) {
    char stuStrResult[MAX_LEN] = {};
    sprintf(stuStrResult, "%s, %s, %s\n", stuName, grade, cause);
    write(resultsFileFD, stuStrResult, strlen(stuStrResult));
}

/**
 * compile the c file
 * @param filePath path to c file
 * @return 1- compiled successfully, 0 - wasn't.
 */
int compileCFile(char filePath[MAX_LEN]) {
    //create another process, than execvp the child.
    pid_t forkRetVal = fork();
    if (forkRetVal == ERROR) {
        write(STDERR, "forking failed\n",strlen("forking failed\n"));
        exit(ERROR);
    } else if (forkRetVal == 0) { //child
        //compile file
        char *args[MAX_LEN] = {"gcc", filePath, NULL};
        if (execvp(args[0], args) == ERROR) {
            write(STDERR, "compiling failed\n",strlen("compiling failed\n"));
            exit(ERROR);
        }
    } else { //father
        int status;
        //wait for the child(same group)
        if (waitpid(forkRetVal, &status, 0) == ERROR) {
            write(STDERR, "waitpid failed\n",strlen("waitpid failed\n"));
            exit(ERROR);
        }
        //compile failed
        if (WEXITSTATUS(status) != 0) {
            return 0;
        }
        //compiled successfully
        return 1;
    }
}
