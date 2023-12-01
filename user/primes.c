#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void prime_filter(int read_fd){
    int read_buf[40],write_fd[2],cnt,tmp_cnt,status;
    cnt=0;
    while((tmp_cnt=read(read_fd,read_buf+cnt,sizeof(read_buf)))>0){
        cnt+=tmp_cnt/sizeof(int);
    }
    if(cnt==0){
        exit(0);
    }else if(cnt==1){
        fprintf(1,"prime %d\n",read_buf[0]);
    }else{
        pipe(write_fd);
        if(fork()==0){
            close(write_fd[1]);
            prime_filter(write_fd[0]);
            close(write_fd[0]);
        }else{
            close(write_fd[0]);
            fprintf(1,"prime %d\n",read_buf[0]);
            for(int i=1;i<cnt;i++){
                if(read_buf[i]%read_buf[0]!=0){
                    write(write_fd[1],&read_buf[i],sizeof(int));
                }
            }
            close(write_fd[1]);
            wait(&status);
            //some error handling here
        }
    }
    exit(0);
}

int
main(int argc, char *argv[])
{
    int fd[2],status;
    pipe(fd);
    if(fork()==0){
        close(fd[1]);
        prime_filter(fd[0]);
        close(fd[0]);
    }else{
        close(fd[0]);
        for(int i=2;i<=35;i++){
            write(fd[1],&i,sizeof(int));
        }
        close(fd[1]);
        wait(&status);
        //some error handling here
    }
    exit(0);
}

