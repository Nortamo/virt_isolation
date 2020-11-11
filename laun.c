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
#include <signal.h>


// Wait until child/parent tells that it's ok to proceed.
void wait_for_action( int * pfd, char * t_buf){
    close(pfd[1]); 
    while (read(pfd[0], t_buf, 1) > 0){}
    close(pfd[0]);
}

// Use a pipe to tell the parent/child that they can proceed.
void tell_done(int *pfd){
    close(pfd[0]);              
    write(pfd[1], "D", 1);
    close(pfd[1]);               
}

void get_exit_status(int *pfd,const char * cmd){
        int set_stat;
        close(pfd[1]);
        read(pfd[0],&set_stat,sizeof(int));
        if(set_stat!=0){
            fprintf(stderr, "The command %s has failed.\n", cmd); 
            exit(EXIT_FAILURE);    
        }

}

void send_exit_status(int *pfd, int status){
        int buf_s;
        buf_s=WEXITSTATUS(status);
        close(pfd[0]);              
        write(pfd[1],&buf_s, sizeof(int));
}



// Check return value and exit on failure
void check_ret(int ret_val, const char * fun){
    if(ret_val == -1){
        fprintf(stderr, "The call to %s has failed.\n", fun); 
        exit(EXIT_FAILURE);
    }
}
void check_ret_kill(int ret_val, const char * fun, pid_t parent_pid){
    if(ret_val != 0){
        fprintf(stderr, "The call to %s has failed.\n", fun); 
        fprintf(stderr, "Killing main process.\n"); 
        kill(parent_pid,9);
        exit(EXIT_FAILURE);
    }

}


int main(int argc, char *argv[]) {
    // Used for blocking
    int pipefd[2];
    int msg_pipe[2];
    char buf;

    // The starting uid we wish to map back to.
    int o_uid=getuid(); 
    char cmd[100];
    // pid for the main process.
    int p_pid=getpid();


    pid_t c1_pid;
    check_ret(pipe(pipefd),"pipe()");       
    check_ret(pipe(msg_pipe),"pipe()");        
    c1_pid = fork();                 
    check_ret(c1_pid,"fork()");

    // First child process
    if (c1_pid == 0){
        // Terminate if the parent exits
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        // Set mapping and tell the parent that we are done
        // gids are mapped 1 to 1
        // map starting uid to 0 and the rest 1 to 1
        sprintf(cmd,"./set %d %d 0",o_uid,p_pid);
        int status=system(cmd);
        send_exit_status(msg_pipe,status); 
        tell_done(pipefd); 
        _exit(EXIT_SUCCESS);

    }
    // main process
    else {
        // Create new user namespace for users and mounts
        // This way we get uid 0 and mount are private to this process
        unshare(CLONE_NEWNS|CLONE_NEWUSER);
        // Wait for the child to do the id mapping
        wait_for_action(pipefd,&buf);
        get_exit_status(msg_pipe,"./set");
        pid_t c2_pid;

        c2_pid = fork();                   
        check_ret(c2_pid,"fork()");
        // second child process
        if(c2_pid==0){
            // Terminate if the parent exits
            prctl(PR_SET_PDEATHSIG, SIGHUP);
            // Mount a squashfs file image
            // Kill the parent process if we fail
            check_ret_kill(
                    execl("/usr/bin/squashfuse",
                        "/usr/bin/squashfuse",
                        "-f",
                        "tmd.sqfs",
                        "/mnt" ,
                        (char*) NULL
                        )
                    ,"execl()",p_pid);
        } 
        // main process
        else{   
            pid_t c3_pid;
            check_ret(pipe(pipefd),"pipe()");      
            check_ret(pipe(msg_pipe),"pipe()");        
            c3_pid = fork();                   
            check_ret(c3_pid,"fork()");
            // Third child process
            if (c3_pid == 0){
                // Terminate if the parent exits
                prctl(PR_SET_PDEATHSIG, SIGHUP);
                // Set mapping and tell the parent that we are done
                // gids are mapped 1 to 1
                // uid mapping is reversed so that we are back to our
                // original uid
                sprintf(cmd,"./set %d %d 1",o_uid,p_pid);
                int status=system(cmd);
                send_exit_status(msg_pipe,status); 
                tell_done(pipefd);
                _exit(EXIT_SUCCESS);
            }
            else{
                // create a nested user namespace so that we can
                // revert back to our original uid 
                unshare(CLONE_NEWUSER); 
                wait_for_action(pipefd,&buf);
                get_exit_status(msg_pipe,"./set");
                execvp(argv[1],argv+1);
            }

        }

    }
}
