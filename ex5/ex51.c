//Elad Israel 313448888

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>
#include <signal.h>

#define STDERR 2
#define ERROR -1
#define SYSCALL_ERR_MSG "Error in system call\n"
#define FILENAME "./draw.out"

char getch();

void testSysCall(int retVal);

int main() {

    int fd[2]; // fd[0] - pipe's input, fd[1] - pipe's output.
    testSysCall(pipe(fd));

    //create another process, than execvp the child, and wait/continue to the next command(with the father).
    pid_t forkRetVal = fork();

    testSysCall(forkRetVal); //check for error in syscall
    if (forkRetVal == 0) { //child
        /*switch standard input(0) to pipe's input(fd[0]),
         * so that when "write" in the father will write to the pipe's output(fd[1]),
         * the second process(exec in the forked child) will read from 0(changed to pipe's input)
         * and will get the pipe's output.
         * */
        dup2(fd[0], 0);
        //execute
        char *execInput[2] = {};
        execInput[0] = FILENAME;
        testSysCall(execvp(FILENAME, execInput));
    } else { //father- fork return value is the pid of the child
        //close(fd[1]);
        char input = getch();
        while (true) {
            if (input == 'w' || input == 'a' || input == 's' || input == 'd' || input == 'q') {
                //write received input to the pipe.
                testSysCall((int) write(fd[1], &input, 1));
            }
            //send sigusr2 to the other process
            testSysCall(kill(forkRetVal, SIGUSR2));
            if (input == 'q') {
                break;
            }
            input = getch();
        }
    }
    return 0;
}

/**
 * Gets char from console without the need to press enter
 * @return char
 */
char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return (buf);
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