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
    int fd;
    int uid= atoi(argv[1]);
    int pid=atoi(argv[2]);
    int opt=atoi(argv[3]);
    int pipefd[2];
    char buf;
    


    char * gid_map;
    // Group mappings are not changed in the new namespace
    gid_map="0 0 4294967295";
    size_t gid_len=strlen(gid_map);
    char uid_map[100];
    if(opt==0){
    sprintf(uid_map,"0 %d 1\n%d %d %d\n%d %d %d",uid,1,1,uid-1,uid+1,uid+1,20000000);
    }
    else if(opt==1){
    sprintf(uid_map,"%d 0 1\n%d %d %d\n%d %d %d",uid,1,1,uid-1,uid+1,uid+1,20000000);
    }
    size_t uid_len=strlen(uid_map);
    char g_f[100];
    char id_f[100];
    char gi_f[100];
    int res;
    sprintf(g_f,"/proc/%d/setgroups",pid);
    sprintf(id_f,"/proc/%d/uid_map",pid);
    sprintf(gi_f,"/proc/%d/gid_map",pid);
        printf("Child file: %s\n ",g_f);  
        printf("Child file: %s\n ",id_f);  
        fd=open(g_f,O_WRONLY);
        if(fd==-1){
            printf("Failed to open file!:\n");
        }
        res=write(fd,"deny",4);
        if(res == -1){
            printf("Failed to write deny!\n");
        }
        close(fd);                             
        fd=open(id_f,O_WRONLY);
        res=write(fd,uid_map,uid_len);          
        if(res == -1){                            
            printf("Writing to uid map failed\n");
        }                                         
        close(fd);                             
        fd=open(gi_f,O_WRONLY);
        res=write(fd,gid_map,gid_len);          
        if(res == -1){                            
            printf("Writing to gid map failed\n");
        }                                         

    
}
