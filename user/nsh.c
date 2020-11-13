//Simple Shell

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
//#define LIST  4
//#define BACK  5

#define MAXARGS 10

struct cmd
{
    int type;
};

struct execcmd
{
    int type;
    char *argv[MAXARGS];
    char *eargv[MAXARGS];
};

struct redircmd
{
    int type;
    struct cmd *cmd;
    char *file;
    char *efile;
    int mode;
    int fd;
};

struct pipecmd
{
    struct cmd *left;
    struct cmd *right;
    int type;    
};

int fork1(void);//Fork 除非 panics 显示失败.
void panic(char*);
struct cmd *parsecmd(char*);

//Execute cmd. never returns.
void runcmd(struct cmd *cmd)
{
    int p[2];
    struct execcmd *ecmd;
    struct redircmd *rcmd;
    struct pipecmd *pcmd;

    if(cmd == 0){
        exit(-1);
    }
    switch (cmd->type)
    {
    case EXEC:/* constant-expression */
        ecmd =(struct execcmd*)cmd;/*强制格式转换*/
        if(ecmd->argv[0]==0)
        {
            exit(-1);
        }
        exec(ecmd->argv[0],ecmd->argv);
        fprintf(2,"exec %s failed\n",ecmd->argv[0]);
        break;
    
    case REDIR:
        rcmd = (struct redircmd*)cmd;/*强制格式转换*/
        close(rcmd->fd);
        if(open(rcmd->file,rcmd->mode)<0){
            fprintf(2,"open %s failed\n",rcmd->file);
            exit(-1);
        }
        runcmd(rcmd->cmd);
        break;
    
    case PIPE:
        pcmd = (struct pipecmd*)cmd;
        if(pipe(p) < 0)
            panic("pipe");
        if(fork1() == 0){
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->left);
        }
        if(fork1() == 0){
            close(0);
            dup(p[0]);//由dup返回的新文件描述符一定是当前可用文件描述中的最小数值。
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->right);
        }
        close(p[0]);//关闭父进程的读和写端口
        close(p[1]);
        wait(0);//等第一个结束的子进程
        wait(0);//等第二个结束的子进程
        break;
    default:
        panic("runcmd");
        break;
    }
    exit(0);
}

int getcmd(char *buf, int max)
{
    fprintf(2,"@");
    memset(buf,0,max);//分配一个内存空间，初始化为0
    gets(buf,max);//从标准输入读取一行命令到buf
    if(buf[0]==0)
    {//什么都没读到
        return -1;
    }
    return 0;
}
int main(void)
{
    static char buf[100];
    int fd;

    //Ensure that three file descriptors are open.
    //0:标准输入，1标准输出，2标准错误输出。三者一般默认打开
    while ((fd = open("console",O_RDWR))>=0)
    {
        if(fd>=3)
        {
            close(fd);
            break;
        }
    }

    //Read and run input commands
    while(getcmd(buf,sizeof(buf))>=0)
    {
        if(buf[0]=='c'&&buf[1]=='d'&&buf[2]==' ')
        {//
            buf[strlen(buf)-1]=0;//为什么是0不是10？ascall码里面，10才是换行
            if(chdir(buf+3)<0)//掠过'cd '
            {
                fprintf(2,"cannot cd %s\n",buf+3);
            }
            continue;
        }
        if(fork1()==0)
        {//子进程调用系统命令
            runcmd(parsecmd(buf));
        }
        wait(0);//父进程等待子进程的结束
    }
    exit(0);
}

void panic(char *s)
{
    fprintf(2,"%s\n",s);
    exit(-1);
}

int fork1(void)
{
    int pid;

    pid = fork();
    if(pid == -1)
        panic("fork");
    return pid;
}

//without malloc!!!

struct cmd* ececcmd(void)
{
    struct execcmd *cmd;
    memset(cmd,0,sizeof(*cmd));
    cmd->type = EXEC;
    return (struct cmd*)cmd;
}
/*
struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}*/


//Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int gettoken(char **ps,char *es,char **q, char *eq)
{
    char *s;
    int ret;

    s = *ps;
    while(s<es && strchr(whitespace,*s))//strchr返回一个指向该字符串中第一次出现的字符的指针，如果字符串中不包含该字符则返回NULL空指针。
    {
        s++;
    }
    if(q)
    {
        *q=s;
    }
    ret = *s;//初始化
    switch (*s)
    {
    case 0:/* constant-expression */
        /* code */
        break;
    case '|':
    case '(':
    case ')':
    case ';':
    case '&':
    case '<'://重定向输入
        s++;
        break;
    case '>'://重定向输出
        s++;
        if(*s == '>'){
            ret = '+';
            s++;
        }
        break;
    default:
        ret = 'a';
        while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
            s++;
        break;
    }
    if(eq)
        *eq = s;

    while(s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}


struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd* parsecmd(char *s)
{
    char *es;
    struct cmd *cmd;

    es = s +strlen(s);//指向s的尾巴
    cmd = parsepipe(&s,es);
    peek(&s,es,"");
}

struct cmd* parsepipe(char** ps, char*es)
{
    struct cmd *cmd;

    cmd = parseexec(ps, es);
    if(peek(ps, es, "|")){
        gettoken(ps, es, 0, 0);
        cmd = pipecmd(cmd, parsepipe(ps, es));
    }
    return cmd;
}

struct cmd* parseredirs(struct cmd *cmd, char **ps, char *es)
{
    int tok;
    char *q, *eq;

    while(peek(ps, es, "<>"))
    {
        tok = gettoken(ps, es, 0, 0);
        if(gettoken(ps, es, &q, &eq) != 'a')
            panic("missing file for redirection");
        switch(tok){
        case '<':
            cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
            break;
        case '>':
            cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
            break;
        case '+':  // >>
            cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
            break;
        }
    }
    return cmd;
}

struct cmd* parseexec(char **ps, char *es)
{
    char *q, *eq;
    int tok, argc;
    struct execcmd *cmd;
    struct cmd *ret;

    if(peek(ps, es, "("))
        return parseblock(ps, es);

    ret = execcmd();
    cmd = (struct execcmd*)ret;

    argc = 0;
    ret = parseredirs(ret, ps, es);
    while(!peek(ps, es, "|)&;"))
    {
        if((tok=gettoken(ps, es, &q, &eq)) == 0)
            break;
        if(tok != 'a')
            panic("syntax");
        cmd->argv[argc] = q;
        cmd->eargv[argc] = eq;
        argc++;
        if(argc >= MAXARGS)
            panic("too many args");
        ret = parseredirs(ret, ps, es);
    }
    cmd->argv[argc] = 0;
    cmd->eargv[argc] = 0;
    return ret;
}

// NUL-terminate all the counted strings.
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;

  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;
  }
  return cmd;
}
