#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    uint32 time;
    if(argc!=2){
        fprintf(2, "sleep: incorrect number of input %d\n", argc);
    }
    time = atoi(argv[1]);
    sleep(time);
    exit(0);
}
