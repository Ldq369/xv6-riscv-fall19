#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char *argv[])
{
    int seconds;
    if(argc < 2)// If the user forgets to pass an argument, sleep should print an error message.
    {
        fprintf(2,"Usage: sleep + seconds\n");
        exit();
    }
    seconds = atoi(argv[1]);//The command-line argument is passed as a string; you can convert it to an integer using atoi.
    if(seconds > 0)
    {
        sleep(seconds);// Use the system call sleep.
    }
    else
    {
        fprintf(2,"Invalid interval %s\n",argv[1]);// If the user passes an invalid argument, sleep should print an error message.
    }    
    exit();// Make sure main calls exit() in order to exit your program.
}