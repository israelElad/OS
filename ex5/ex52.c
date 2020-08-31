//Elad Israel 313448888

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include <signal.h>

#define ROWS_NUM 20
#define COLUMNS_NUM 20
#define MINUSES_NUM 3
#define STDERR 2
#define ERROR -1
#define SYSCALL_ERR_MSG "Error in system call\n"
#define FILENAME "./draw.out"
#define INVALID_INPUT "Invalid input!"


void initializeMinuses();

void emptyBoard();

void printBoard();

void alarmSigHandler();

void inputSigHandler();

void testSysCall(int retVal);

void wRotateKey();

typedef struct CellPosStruct {
    int row;
    int column;
} CellPos;

typedef struct boardStruct {
    char boardMatrix[ROWS_NUM][COLUMNS_NUM];
    bool horizontalMinuses;
    CellPos minuses[MINUSES_NUM];
} Board;

Board board;

int main() {
    initializeMinuses();
    printBoard();
    //set alarm to lower the minuses every second
    signal(SIGALRM, alarmSigHandler);
    alarm(1);
    //set signal handler to deal with user's input
    signal(SIGUSR2, inputSigHandler);
    while (true) {
        pause(); //wait for either of the signals
    }
}

/**
 * initialize the minuses to be horizontal at the first row
 */
void initializeMinuses() {

    board.horizontalMinuses = true;

    board.minuses[0].column = COLUMNS_NUM / 2 - 1;
    board.minuses[1].column = COLUMNS_NUM / 2;
    board.minuses[2].column = COLUMNS_NUM / 2 + 1;

    board.minuses[0].row = 0;
    board.minuses[1].row = 0;
    board.minuses[2].row = 0;
}

/**
 * initialize the board as an empty board(only frame and spaces)
 */
void emptyBoard() {
    //initialize board matrix
    int i;
    for (i = 0; i < ROWS_NUM; i++) {
        int j;
        for (j = 0; j < COLUMNS_NUM; j++) {
            if (i == ROWS_NUM - 1) { //first or last row
                board.boardMatrix[i][j] = '*';
            } else { //other rows
                if (j == 0 || j == COLUMNS_NUM - 1) { //first or last column
                    board.boardMatrix[i][j] = '*';
                } else { //other columns
                    board.boardMatrix[i][j] = ' ';
                }
            }
        }
    }
}

/**
 * clear previous prints and print the current matrix
 */
void printBoard() {

    system("clear");
    emptyBoard();

    //place minuses in the matrix before printing
    int i;
    for (i = 0; i < MINUSES_NUM; i++) {
        board.boardMatrix[board.minuses[i].row][board.minuses[i].column] = '-';
    }

    for (i = 0; i < ROWS_NUM; i++) {
        int j;
        for (j = 0; j < COLUMNS_NUM; j++) {
            printf("%c", board.boardMatrix[i][j]);
        }
        printf("\n");
    }
}

/**
 * lower the minuses every second
 */
void alarmSigHandler() {
    //set this function as the signal handler for next time too.
    if (signal(SIGALRM, alarmSigHandler) == SIG_ERR) {
        write(STDERR, SYSCALL_ERR_MSG, strlen(SYSCALL_ERR_MSG));
        _exit(ERROR);
    }
    //set another alarm
    alarm(1);
    if (board.minuses[MINUSES_NUM - 1].row >= ROWS_NUM - 2) {
        initializeMinuses();
    } else {
        int i;
        for (i = 0; i < MINUSES_NUM; i++) {
            board.minuses[i].row++;
        }
    }
    printBoard();
}

/**
 * deal with incoming signal from user's input
 */
void inputSigHandler() {
    char input;
    if (signal(SIGUSR2, inputSigHandler) == SIG_ERR) {
        write(STDERR, SYSCALL_ERR_MSG, strlen(SYSCALL_ERR_MSG));
        _exit(ERROR);
    }
    testSysCall((int) read(0, &input, 1));
    int i;
    switch (input) {
        case 'w':
            wRotateKey();
            break;
        case 'a':
            if (board.minuses[0].column > 1) {
                for (i = 0; i < MINUSES_NUM; i++) {
                    board.minuses[i].column--;
                }
            }
            break;
        case 's':
            if (board.minuses[MINUSES_NUM - 1].row < ROWS_NUM - 2) {
                for (i = 0; i < MINUSES_NUM; i++) {
                    board.minuses[i].row++;
                }
            }
            break;
        case 'd':
            if (board.minuses[MINUSES_NUM - 1].column < COLUMNS_NUM - 2) {
                for (i = 0; i < MINUSES_NUM; i++) {
                    board.minuses[i].column++;
                }
            }
            break;
        case 'q':
            _exit(0);
        default:
            write(STDERR, INVALID_INPUT, strlen(INVALID_INPUT));
            break;
    }
    printBoard();
}

/**
 * Rotate the minuses from horizontal to vertical and vice versa.
 */
void wRotateKey() {

    if (board.horizontalMinuses == true) {

        //cannot rotate before the first row and close to the last
        if (board.minuses->row <= 0 || board.minuses->row > ROWS_NUM - 3) {
            return;
        }
        //first minus row up column right
        board.minuses[0].row--;
        board.minuses[0].column++;

        //last minus row down column left
        board.minuses[MINUSES_NUM - 1].row++;
        board.minuses[MINUSES_NUM - 1].column--;

        board.horizontalMinuses = false;

    } else { // minuses are vertical
        //cannot rotate close to the first row and close to the last
        if (board.minuses->column <= 1 || board.minuses->column > COLUMNS_NUM - 3) {
            return;
        }

        //first minus row down column right
        board.minuses[0].row++;
        board.minuses[0].column++;

        //last minus row up column left
        board.minuses[MINUSES_NUM - 1].row--;
        board.minuses[MINUSES_NUM - 1].column--;

        //after rotating twice the array's cells were reversed- reverse the minuses array back.
        int left, right;
        for (left = 0, right = MINUSES_NUM - 1; left < right; left++, right--) {
            CellPos temp = board.minuses[left];
            board.minuses[left] = board.minuses[right];
            board.minuses[right] = temp;
        }
        board.horizontalMinuses = true;
    }
}

/**
 * Check function's return value(which returns -1 or any other negative number on failure).
 * @param retVal return val
 */
void testSysCall(int retVal) {
    if (retVal < 0) {
        write(STDERR, SYSCALL_ERR_MSG, strlen(SYSCALL_ERR_MSG));
        _exit(ERROR);
    }
}