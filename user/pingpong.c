#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    uint8 byte=0;
    int p1[2];
    int p2[2];
    pipe(p1);
    pipe(p2);
    if(fork()==0)
    {//child process
    //first ping pong
    close(p1[1]);
    read(p1[0],&byte,1);
    fprintf(1,"%d: received ping\n",getpid());
    close(p1[0]);
    //second ping pong
    close(p2[0]);
    write(p2[1],&byte,1);
    close(p2[1]);
    }else{
    //parent process
    //first ping pong
    close(p1[0]);
    write(p1[1],&byte,1);
    close(p1[1]);
    //second ping pong
    close(p2[1]);
    read(p2[0],&byte,1);
    fprintf(1,"%d: received pong\n",getpid());
    close(p2[0]);
    }
    exit(0);
}
