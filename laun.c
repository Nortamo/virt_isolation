#define _GNU_SOURCE
#include <sched.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <error.h> 
#include <errno.h>
#include <asm-generic/errno-base.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <sys/stat.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int pipefd[2];
    pid_t cpid;
    char buf;
    int o_uid=getuid(); 
    char cmd[100];
    int p_pid=getpid();
    

    if (pipe(pipefd) == -1)          /* An error has occurred. */
  {
   fprintf(stderr, "%s", "The call to pipe() has failed.\n");           
   exit(EXIT_FAILURE);
  }
 cpid = fork();                   /* fork() returns the child process's PID  */
 if (cpid == -1)                  /* An error has occurred. */
  {
   fprintf(stderr, "%s", "The call to fork() has failed.\n");
   exit(EXIT_FAILURE);
  }
 if (cpid == 0){
    
    // Set mapping and the parent that we are done
    sprintf(cmd,"./set %d %d 0",o_uid,p_pid);
    system(cmd);
    close(pipefd[0]);              /* Close unused read end */
    write(pipefd[1], "H", 1);
    close(pipefd[1]);               
    
    _exit(EXIT_SUCCESS);
   }
 else {
    unshare(CLONE_NEWNS|CLONE_NEWUSER);
    
    // Wait for child to set first mappings
    close(pipefd[1]); 
    while (read(pipefd[0], &buf, 1) > 0){ 
    }
    close(pipefd[0]);
   
    pid_t c_cpid;
    int c_pipefd[2];
    char c_buf;
    char c_cmd[100];
    int c_p_pid=getpid();
    if (pipe(c_pipefd) == -1)          /* An error has occurred. */
  {
   fprintf(stderr, "%s", "The call to pipe() in child has failed.\n");           
   exit(EXIT_FAILURE);
  }
 c_cpid = fork();                   /* fork() returns the child process's PID  */
 if (c_cpid == -1)                  /* An error has occurred. */
  {
   fprintf(stderr, "%s", "The call to fork() in child has failed.\n");
   exit(EXIT_FAILURE);
  }
 if (c_cpid == 0){
    sprintf(c_cmd,"./set %d %d 1",o_uid,c_p_pid);
    system(c_cmd);
    close(c_pipefd[0]);              /* Close unused read end */
    write(c_pipefd[1], "H", 1);
    close(c_pipefd[1]);                
    _exit(EXIT_SUCCESS);
    }
 else{
    unshare(CLONE_NEWUSER); 
    close(pipefd[1]); 
    while (read(pipefd[0], &buf, 1) > 0){ 
    }
    execvp(argv[1],argv+1);
    }
 }
}
