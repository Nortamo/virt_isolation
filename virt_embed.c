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

static int received = 0;

void readUsual(int sig)
{
    if (sig == SIGUSR1)
    {
        received = 1;
    }
}

void check_file_ret(int ret_val,const char * action,const char * file_name){
    if(ret_val == -1){
        fprintf(stderr, "Failed to %s file: %s !\n", action,file_name); 
        perror("Error: ");
    }
} 


void send_exit_status(int *pfd, int status){
        int buf_s;
        buf_s=WEXITSTATUS(status);
        close(pfd[0]);              
        write(pfd[1],&buf_s, sizeof(int));
}


void check_ret_kill(int ret_val, const char * fun, pid_t parent_pid){
    if(ret_val != 0){
        fprintf(stderr, "The call to %s has failed.\n", fun); 
        fprintf(stderr, "Killing main process.\n"); 
        kill(parent_pid,9);
        exit(EXIT_FAILURE);
    }

}

inline int set_mapping(pid_t pid,int uid,int gid,int mode,int cap_enabled){
    char uid_map[100];
    char gid_map[100];
    const long max_uid=20000000;
    const long max_id=4294967295;
   
    // Mappings from default to first user namespace
    if(mode==0){
        if(cap_enabled){
            sprintf(gid_map,"%s","0 0 4294967295");
            sprintf(uid_map,"0 %d 1\n%d %d %d\n%d %d %ld",uid,1,1,uid-1,uid+1,uid+1,max_uid);
        }
        else{
            sprintf(gid_map,"%d %d 1",gid,gid);
            sprintf(uid_map,"0 %d 1",uid);
        }

    }
    // Mapping from first user namespace to second
    else if(mode==1){
        if(cap_enabled){
            sprintf(gid_map,"%s","0 0 4294967295");
            sprintf(uid_map,"%d 0 1\n%d %d %d\n%d %d %ld",uid,1,1,uid-1,uid+1,uid+1,max_uid);
        }
        else{
            sprintf(gid_map,"%d %d 1",gid,gid);
            sprintf(uid_map,"%d 0 1",uid);

        } 
    }
    else{
        fprintf(stderr,"Unknown mode\n");
        return 1;
    }
    
    size_t gid_len=strlen(gid_map);
    size_t uid_len=strlen(uid_map);
    char setgroups_f[100];
    char uid_map_f[100];
    char gid_map_f[100];
    int res;
    sprintf(setgroups_f,"/proc/%d/setgroups",pid);
    sprintf(uid_map_f,"/proc/%d/uid_map",pid);
    sprintf(gid_map_f,"/proc/%d/gid_map",pid);
    int fd;

    fd=open(setgroups_f,O_WRONLY);
    check_file_ret(fd,"open",setgroups_f);
    if(fd==-1){
        return 1;
    }
    res=write(fd,"deny",4);
    check_file_ret(res,"write to",setgroups_f);
    if(res==-1){
        return 1;
    }
    close(fd);                             
    fd=open(uid_map_f,O_WRONLY);
    if(fd==-1){
        return 1;
    }
    check_file_ret(fd,"open",uid_map_f);
    res=write(fd,uid_map,uid_len);          
    check_file_ret(res,"write to",uid_map_f);
    if(res==-1){
        return 1;
    }
    close(fd);                             
    fd=open(gid_map_f,O_WRONLY);
    if(fd==-1){
        return 1;
    }
    check_file_ret(fd,"open",gid_map_f);
    res=write(fd,gid_map,gid_len); 
    check_file_ret(res,"write to",gid_map_f);
    if(res==-1){
        return 1;
    }
    close(fd);

    return 0;
}
void check_ret(int ret_val, const char * fun){
    if(ret_val == -1){
        fprintf(stderr, "The call to %s has failed.\n", fun); 
        exit(EXIT_FAILURE);
    }
}

void get_set_exit_status(int *pfd,const char * cmd){
        int set_stat;
        close(pfd[1]);
        read(pfd[0],&set_stat,sizeof(int));
        
        // Set was sucessfull 
        if(set_stat!=0){
        exit(EXIT_FAILURE);
        }
}

int main(int argc, char *argv[]) {
    int p_to_c1[2];
    int c1_to_p[2];
    int msg_pipe[2];
    int wstat;
    char buf;
    signal(SIGUSR1,readUsual);
/*
    const char * sqfs_f=argv[1];
    const char * mount_p=argv[2];
    if( access( sqfs_f, F_OK ) == -1 ) {
        fprintf(stderr,"Can't open squashfs image %s \n",sqfs_f); 
        return -1;
    } 
    if( access( mount_p, F_OK ) == -1 ) {
        fprintf(stderr,"Mount point %s does not exist \n",mount_p); 
        return -1;
    } 
*/

    cap_value_t cap_values[] = {CAP_SETUID, CAP_SETGID};
    cap_t caps;
    caps = cap_get_proc();
    cap_flag_value_t v1,v2;
    cap_get_flag(caps, cap_values[0], CAP_EFFECTIVE, &v1);
    cap_get_flag(caps, cap_values[1], CAP_EFFECTIVE, &v2);
      
    int caps_set=(v1==CAP_SET && v2==CAP_SET);


    uid_t user_uid=getuid();
    pid_t parent_pid=getpid();
    uid_t default_gid=getgid();
    
    prctl(PR_SET_DUMPABLE, 1); 
   
    pid_t c1_pid;
    check_ret(pipe(p_to_c1),"pipe()");
    check_ret(pipe(c1_to_p),"pipe()");       
    check_ret(pipe(msg_pipe),"pipe()");        
    close(p_to_c1[0]);
    close(c1_to_p[1]);
    c1_pid = fork();                 
    check_ret(c1_pid,"fork()");

    if (c1_pid == 0){
        close(p_to_c1[1]);
        close(c1_to_p[0]);
        prctl(PR_SET_DUMPABLE, 1); 
        // Terminate if the parent exits
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        // Set mapping and tell the parent that we are done
        // gids are mapped 1 to 1
        // map starting uid to 0 and the rest 1 to 1
        while (!received);
        int status=set_mapping(parent_pid,user_uid,parent_pid,0,caps_set);
        send_exit_status(msg_pipe,status); 
        _exit(EXIT_SUCCESS);

    }
    
    // main process
    else {
        // Create new user namespace for users and mounts
        // This way we get uid 0 and mount are private to this process
        // Wait for the child to do the id mapping
        unshare(CLONE_NEWNS|CLONE_NEWUSER);
        kill(c1_pid,SIGUSR1);
        wait();
        get_set_exit_status(msg_pipe,"set_mappings");
        execvp(argv[1],argv+1);
    }
     //   pid_t c2_pid;

     //   c2_pid = fork();                   
     //   check_ret(c2_pid,"fork()");
        // second child process
        /*
        if(c2_pid==0){
            // Terminate if the parent exits
            prctl(PR_SET_PDEATHSIG, SIGHUP);
            // Mount a squashfs file image
            // Kill the parent process if we fail
            check_ret_kill(
                    execl("/usr/bin/squashfuse",
                        "/usr/bin/squashfuse",
                        "-f",
                        sqfs_f,
                        mount_p ,
                        (char*) NULL
                        )
                    ,"execl()",parent_pid);
        } 
        // main process
        else{   
            execvp(argv[1],argv+1);
        }
    }
            execvp(argv[1],argv+1);
    */
}
