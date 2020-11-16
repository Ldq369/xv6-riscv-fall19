#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

inline int print(int , char*);
char *simple_tok(char*, char );
char *trim(char*);
void redirect(int, int pd[]);
void handle(char*);
void handle_cmd();

char cmd_buf[1024];
char *cmd_a, *n;

// a simple shell
int main(int argc, char *argv[])
{
    char *c;
    while (1)
    {
        print(1, "@ ");
        memset(cmd_buf, 0, 1024);
        gets(cmd_buf, 1024);

        if (cmd_buf[0] == 0) // EOF
            exit(0);

        c = strchr(cmd_buf, '\n'); 
        *c = '\0';

        if (fork())
        {//parent_proc
            wait(0);
        }
        else
        {//child_proc
            cmd_a = cmd_buf;
            n = simple_tok(cmd_a, '|');
            handle_cmd();
        }
    }

    exit(0);
}

int print(int fd, char *str)
{
    return write(fd, str, strlen(str));
}

// replace the left side "|" with "\0"
// then return the rest of the string or NULL
char *simple_tok(char *cmd_a, char c)
{
    while (*cmd_a != '\0' && *cmd_a != c)
        cmd_a++;
    if (*cmd_a == '\0')
        return 0;
    *cmd_a = '\0';
    return cmd_a + 1;
}

// trim spaces on both side
char *trim(char *cmd_a)
{
    char *e = cmd_a;
    while (*e)
        e++;
    while (*cmd_a == ' ')
        *(cmd_a++) = '\0';
    while (*(--e) == ' ')
        ;
    *(e + 1) = '\0';
    return cmd_a;
}

void redirect(int k, int pd[])
{
    close(k);
    dup(pd[k]);
    close(pd[0]);
    close(pd[1]);
}

void handle(char *cmd)
{
    char buf[32][32];
    char *pass[32];
    int argc = 0;

    cmd = trim(cmd);
    // fprintf(2, "cmd: %s\n", cmd);

    for (int i = 0; i < 32; i++)
        pass[i] = buf[i];

    char *c = buf[argc];
    int input_pos = 0, output_pos = 0;
    for (char *p = cmd; *p; p++)
    {
        if (*p == ' ' || *p == '\n')
        {
            *c = '\0';
            argc++;
            c = buf[argc];
        }
        else {
            if(*p == '<') {
                input_pos = argc + 1;
            } if(*p == '>') {
                output_pos = argc + 1;
            }
            *c++ = *p;
        }
    }
    *c = '\0';
    argc++;
    pass[argc] = 0;

    // fprintf(2, "inpos: %d, outpos: %d\n", input_pos, output_pos);

    if(input_pos) {
        close(0);
        open(pass[input_pos], O_RDONLY);
    }

    if(output_pos) {
        close(1);
        open(pass[output_pos], O_WRONLY | O_CREATE);
    }

    char *pass2[32];
    int argc2 = 0;
    for(int pos = 0; pos < argc; pos++) {
        if(pos == input_pos - 1) pos += 2;
        if(pos == output_pos - 1) pos += 2;
        pass2[argc2++] = pass[pos];
    }
    pass2[argc2] = 0;

    if (fork())
    {
        wait(0);
    }
    else
    {
        exec(pass2[0], pass2);
    }
}

void handle_cmd()
{
    if (cmd_a)
    {
        int pd[2];
        pipe(pd);
        // int parent_pid = getpid();
        // fprintf(2, "pid: %d, cmd: %s\n", parent_pid, cmd_a);

        if(!fork()){
            // fprintf(2, "%d -> %d source\n", parent_pid, getpid());
            if(n) redirect(1, pd);
            handle(cmd_a);
        } else if(!fork()) {
            // fprintf(2, "%d -> %d sink\n", parent_pid, getpid());
            if(n) {
                redirect(0, pd);
                cmd_a = n;
                n = simple_tok(cmd_a, '|');
                handle_cmd();
            }
        }

        close(pd[0]);
        close(pd[1]);
        wait(0);
        wait(0);
    }

    exit(0);

}
