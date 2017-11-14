#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include "signal_handlers.h"
#include "commands.h"
#include "built_in.h"

#define FILE_SERVER "/tmp/test_server"
#define SOCK_PATH "tpf_unix_sock.server"
#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands > 0) {
    if(n_commands==1){
      struct single_command* com = (*commands);
      return exec_com(com);
    }else{
      struct single_command* com = (*commands);
      struct single_command* next_com = (*commands+1);
      pthread_t p_thread;
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_create(&p_thread,&attr,socket_thread,(void*)next_com);
      if(fork()==0){
         int client_socket;
         struct sockaddr_un server_addr;
         client_socket = socket(PF_FILE,SOCK_STREAM,0);
         if(client_socket == -1){
            printf("socket creation fail!\n");
            exit(1);
         }
         memset(&server_addr,0,sizeof(server_addr));
         server_addr.sun_family = AF_UNIX;
         strcpy(server_addr.sun_path,FILE_SERVER);
         while(1)
         {
            if(connect(client_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))) continue;
            dup2(client_socket,1);
            exec_com(com);
            close(client_socket);
            exit(0);  
         }
        }
        wait(0);
      } 
    }
    return 0;
}
     
void *socket_thread(void* arg)
{
   struct single_command *com2 = (struct single_command *)arg;
   int server_socket;
   if(0 == access(FILE_SERVER,F_OK))
       unlink(FILE_SERVER);
   
   server_socket = socket(PF_FILE,SOCK_STREAM,0);
   struct sockaddr_un server_addr;
   memset(&server_addr,0,sizeof(server_addr));
   server_addr.sun_family=AF_UNIX;
   strcpy(server_addr.sun_path, FILE_SERVER);
   
   if(bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)))
   {
       printf("bind() error at server side!\n");
       exit(1);
   }
   while(1)
   {
      if(listen(server_socket,5)== -1)
        continue;
      struct sockaddr_un client_addr;
      int client_socket;
      int client_addr_size;
      client_addr_size = sizeof(client_addr);
      client_socket=accept(server_socket,(struct sockaddr*)&client_addr,&client_addr_size);
      if(client_socket==-1)
        continue;
      if(fork()==0)
      {
        dup2(client_socket,0);
        exec_com(com2);
        close(client_socket);
        close(server_socket);
        exit(0);
      }
      wait(0);
      close(client_socket);
      close(server_socket);
      break;
   }
   pthread_exit(0);
}

int exec_com(struct single_command (*com))
{
 assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else if(getFullPATH(com->argv)){
        do_exec(com->argv,com->argc);
        return 0;
    }else{
        printf("%s: command not found\n",com->argv[0]);
        return -1;
    }
    return 0;
}
void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}

int do_exec(char** argv,int argc)
{
    int pid;
    if((pid = fork())==0)
    {
        if(strcmp(argv[argc-1],"&")==0)
        {
           free(argv[argc]);
           argv[argc-1] = NULL;
           printf("%d\n",getpid());
        }
        execv(argv[0],argv);
    }
    if(strcmp(argv[argc-1],"&")==0)
    {
        bg_full_command = NULL;
        bgpid =0;
        bgpid =pid;
        pthread_t p_thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&p_thread,&attr,print_bg_terminate,(void*)NULL);
        
        int com_length = 0;
        for(int i=0; i<argc-1; i++)
            com_length += strlen(argv[i]+1);
        bg_full_command = (char*)malloc(com_length);
        strcpy(bg_full_command,argv[0]);
        for(int i=1; i<argc-1;i++)
        {
            strcat(bg_full_command,argv[i]);
            if(i != argc-2)
              strcat(bg_full_command," ");
        }
    }
    else if(strcmp(argv[argc-1], "&") !=0)
    {
      int  status;
      waitpid(pid,&status,0);
    }
}

void *print_bg_terminate(void* argv)
{
   int status;
   while(waitpid(bgpid,&status,WNOHANG)==0){}
   printf("%d\tDnoe\t\t%s\n",bgpid,bg_full_command);
   pthread_exit(0);
}

int getFullPATH(char** argv)
{
    if(!access(argv[0],X_OK))
       return 1;
    const char *path = getenv("PATH");
    char* p = malloc(strlen(path));
    strcpy(p,path);
    char *parsed_path =strtok(p,":");
    while(parsed_path != NULL)
    {
       char *dir=malloc(strlen(parsed_path)+strlen(argv[0]+1));
       strcpy(dir,parsed_path);
       strcat(dir, "/");
       strcat(dir,argv[0]);
       if(!access(dir,X_OK))
       {
          argv[0] = malloc(strlen(dir)+1);
          strcpy(argv[0],dir);
          return 1;
       }
       parsed_path =strtok(NULL,":");
    }
    return 0;
}

