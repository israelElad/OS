//Elad Israel 313448888
#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_LEN 151
#define IDENTICAL 1
#define DIFFERENT 2
#define SIMILAR 3
#define ERROR -1
#define STDERR 2

void readUntilNonSpace(int fileFD, char *buffer);

void closeFiles(int firstFileFD, int secondFileFD);

int treatLeftovers(ssize_t bytesReadFirst, ssize_t bytesReadSecond, bool *isIdentical, int firstFileFD,
        int secondFileFD, char *buffer1, char *buffer2);

/**
 * The program checks if to files are identical, different,
 * or similar(identical except whitespaces and upper/lower-case
 * @param argc Number of elements in argv
 * @param argv Two paths of the two files
 * @return identical: 1, similar: 3, different: 2
 */
int main(int argc, char *argv[]) {
    //expecting two paths(and fileName as argv[0])
    if (argc != 3) {
        write(STDERR, "Invalid arguments!",strlen("Invalid arguments!"));
        exit(ERROR);
    }
    char firstPath[MAX_LEN];
    char secondPath[MAX_LEN];
    strcpy(firstPath, argv[1]);
    strcpy(secondPath, argv[2]);

    int firstFileFD = open(firstPath, O_RDONLY);
    int secondFileFD = open(secondPath, O_RDONLY);
    if (firstFileFD == ERROR || secondFileFD == ERROR) {
        write(STDERR, "Couldn't open files",strlen("Couldn't open files"));
        exit(ERROR);
    }
    char buffer1 = 0;
    char buffer2 = 0;
    ssize_t bytesReadFirst = read(firstFileFD, &buffer1, 1);
    ssize_t bytesReadSecond = read(secondFileFD, &buffer2, 1);
    bool isIdentical = true;

    //read until both files are at the end
    while (bytesReadFirst > 0 && bytesReadSecond > 0) {
        if (bytesReadFirst == ERROR || bytesReadSecond == ERROR) {
            write(STDERR, "Read error",strlen("Read error"));
            closeFiles(firstFileFD, secondFileFD);
            exit(ERROR);
        }
        //current buffers aren't identical
        if (buffer1 != buffer2) {
            //not identical
            isIdentical = false;
            readUntilNonSpace(firstFileFD, &buffer1);
            readUntilNonSpace(secondFileFD, &buffer2);
            buffer1 = (unsigned char) toupper(buffer1);
            buffer2 = (unsigned char) toupper(buffer2);
            if (buffer1 != buffer2) { //files are different
                closeFiles(firstFileFD, secondFileFD);
                return DIFFERENT;
            }
        }
        bytesReadFirst = read(firstFileFD, &buffer1, 1);
        bytesReadSecond = read(secondFileFD, &buffer2, 1);
    }
    if (treatLeftovers(bytesReadFirst, bytesReadSecond, &isIdentical, firstFileFD, secondFileFD, &buffer1, &buffer2) ==
            DIFFERENT) {
        return DIFFERENT;
    }
    closeFiles(firstFileFD, secondFileFD);
    if (isIdentical) {
        return IDENTICAL;
    } else { //similar
        return SIMILAR;
    }
}

int
treatLeftovers(ssize_t bytesReadFirst, ssize_t bytesReadSecond, bool *isIdentical, int firstFileFD, int secondFileFD,
               char *buffer1, char *buffer2) {
    if (bytesReadFirst > 0) { //still something left in first file
        //not identical
        *isIdentical = false;
        //check if it is only spaces
        readUntilNonSpace(firstFileFD, buffer1);
        //there is non-space character left on first file
        if (!isspace(*buffer1)) {
            closeFiles(firstFileFD, secondFileFD);
            return DIFFERENT;
        }
    }
    if (bytesReadSecond > 0) { //still something left in second file
        //not identical
        *isIdentical = false;
        //check if it is only spaces
        readUntilNonSpace(secondFileFD, buffer2);
        //there is non-space character left on second file
        if (!isspace(*buffer2)) {
            closeFiles(firstFileFD, secondFileFD);
            return DIFFERENT;
        }
    }
    return 0;
}


void closeFiles(int firstFileFD, int secondFileFD) {
    if (close(firstFileFD) == ERROR) {
        write(STDERR, "Couldn't close files",strlen("Couldn't close files"));
        exit(ERROR);
    }
    if (close(secondFileFD) == ERROR) {
        write(STDERR, "Couldn't close files",strlen("Couldn't close files"));
        exit(ERROR);
    }
}

void readUntilNonSpace(int fileFD, char *buffer) {
    while (isspace(*buffer)) {
        ssize_t bytesReadFirst = read(fileFD, buffer, 1);
        if (bytesReadFirst == 0) {
            break;
        }
    }
}