#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int getLine(int fd, char *buf, int max) {
    int i, status;
    char c;
    for (i = 0; i < max; i++) {
        status = read(fd, &c, 1);
        if (status < 1) break;
        if(c == '\n'){
            buf[i] = '\0';
            break;
        }
        buf[i] = c;
    }
    return i > 0; // Return 1 if line read, 0 otherwise
}

int
main(int argc, char *argv[])
{
    char *args[MAXARG],buf[512];
    int i = 1;

    if (argc < 2) {
        fprintf(2, "usage: xargs command\n");
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        args[i - 1] = argv[i];
    }

    while (getLine(0, buf, sizeof(buf))) {
        fprintf(2, "in while\n");
        int pid,status;

        args[i - 1] = buf; // Add line as the last argument
        args[i] = 0;       // Null terminate argument list
        // Fork and execute the command
        if ((pid = fork()) == 0) {

            exec(args[0], args);
            exit(0);
        }else{
            wait(&status);
        }
    }

    exit(0);
}