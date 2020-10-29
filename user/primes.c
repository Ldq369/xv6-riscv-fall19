#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void init_nums(int nums[34])
{
    int i = 0;
    for(int count=2;count<=35;count++)
    {
        nums[i]=count;
        i++;
    }
}

void send_primes(int pipeline[],int nums[],int numslen)
{
    int info;
    close(pipeline[0]);//close read
    for(int i =0;i<numslen;i++)
    {
        info = nums[i];
        write(pipeline[1],&info,sizeof(info));//为什么不会覆盖呢?
    }
}

void process(int pipeline[])
{
    int isprime;
    int right_neighbor;
    int len;
    int pid;
    int child_pd[2];
    int nums[34];
    int nums_id = 0;
    pipe(child_pd);

    close (pipeline[1]);
    len = read(pipeline[0],&isprime,sizeof(isprime));
    printf("prime %d\n",isprime);

    while(len!=0)
    {
        len = read(pipeline[0],&right_neighbor,sizeof(right_neighbor));
        if(right_neighbor % isprime !=0)
        {
            *(nums+nums_id)=right_neighbor;
            nums_id++;
        }
    }
    close(pipeline[0]);
    if(nums_id == 0)
    {
        exit();
    }
    if((pid = fork()) > 0)//父写
    {
        send_primes(child_pd,nums,nums_id);
        wait();
        exit();
    }
    else//子读
    {
        process(child_pd);
        exit();
    }
}

int main(int argc,char **argv)
{
    int pipeline[2];
    pipe (pipeline);
    int pid;
    //parent proc
    if((pid = fork()) > 0)
    {
        int nums[34];
        init_nums(nums);
        send_primes(pipeline,nums,34);
        exit();
    }
    else
    {
        process(pipeline);
        exit();
    }
    
}