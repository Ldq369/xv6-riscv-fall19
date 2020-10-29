//Your solution should be in the file user/pingpong.c.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char *argv[])
{
    int parent_fd[2];
    int child_fd[2];
    char buf[20];
    pipe(parent_fd);//The parent sends by writing a byte to parent_fd[1] and the child receives it by reading from parent_fd[0]
    pipe(child_fd);//After receiving a byte from parent, the child responds with its own byte by writing to child_fd[1], which the parent then reads.  
    int pid; 
    //parent proc
    if((pid = fork()) > 0)
    {
        close(parent_fd[0]);
        write(parent_fd[1],"ping",4);
        close(child_fd[1]);
        read(child_fd[0],buf,sizeof(buf));
        fprintf(2,"%d:recieved %s\n",getpid(),buf);//The number before ":" is the process id of the process printing the output. You can get the process id by calling the system call getpid. 
        exit();
    }
    //child proc
    else
    {
        close(parent_fd[1]);  
        read(parent_fd[0],buf,4);
        fprintf(2,"%d:recieved %s\n",getpid(),buf);
        close(child_fd[0]);
        write(child_fd[1],"pong",sizeof(buf));
        exit();
    }   
}