#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path,char *file)
{
  char buf[512], *p;//buf存储路径地址，p始终指向buf的尾
  int fd;//文件标识符
  struct dirent de;//不知道有啥用
  struct stat st;//保存打开的文件的状态信息

  if((fd = open(path, 0)) < 0){//open发生错误会返回-1
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0|| T_DIR != st.type){//fstat(): Return info about an open ﬁle
    fprintf(2, "find:the first arg must be dir path\n", path);
    close(fd);
    return;
  }

  while(read(fd, &de, sizeof(de)) == sizeof(de))
  {
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
    {
        printf("ls: path too long\n");
        break;
    }
    strcpy(buf, path);//把地址路径复制到buf
    p = buf+strlen(buf);//p指向buf的尾巴
    *p++ = '/';
    if(de.inum == 0)
        continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if(stat(buf, &st) < 0)
    {
        printf("find: cannot stat %s\n", buf);
        continue;
    }    
  }

  switch(st.type){
  case T_FILE:  //如果是文件?
    if(strcmp(file,de.name)==0)
    {
        printf("%s\n",buf);
    }
    break;
  case T_DIR: //如果是目录
    if(strcmp(de.name,".")!=0&&strcmp(de.name,"..")!=0){
      find(buf,file);
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[])
{
    if(argc <= 2||argc>3)
    {
        fprintf(2, "usage: find <path> <expression>\n");
        exit();
    }
    //fprintf(2,"%s\n%s",argv[1],argv[2]);
    find(argv[1],argv[2]);
    exit();
}
