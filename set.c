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


void check_file_ret(int ret_val,const char * action,const char * file_name){
    if(ret_val == -1){
        fprintf(stderr, "Failed to %s file: %s !\n", action,file_name); 
        exit(EXIT_FAILURE);
    }
} 

int main(int argc, char *argv[]) {
    int fd;
    int uid= atoi(argv[1]);
    int pid=atoi(argv[2]);
    int mode=atoi(argv[3]);
    
    const char * gid_map;
    // Group mappings are not changed in the new namespace
    gid_map="0 0 4294967295";
    size_t gid_len=strlen(gid_map);
    char uid_map[100];
    if(mode==0){
    sprintf(uid_map,"0 %d 1\n%d %d %d\n%d %d %d",uid,1,1,uid-1,uid+1,uid+1,20000000);
    }
    else if(mode==1){
    sprintf(uid_map,"%d 0 1\n%d %d %d\n%d %d %d",uid,1,1,uid-1,uid+1,uid+1,20000000);
    }
    else{
        fprintf(stderr,"Unknown mode %d.\n",mode);
        return -1;
    }
    size_t uid_len=strlen(uid_map);
    char setgroups_f[100];
    char uid_map_f[100];
    char gid_map_f[100];
    int res;
    sprintf(setgroups_f,"/proc/%d/setgroups",pid);
    sprintf(uid_map_f,"/proc/%d/uid_map",pid);
    sprintf(gid_map_f,"/proc/%d/gid_map",pid);
    fd=open(setgroups_f,O_WRONLY);
    check_file_ret(fd,"open",setgroups_f);
    res=write(fd,"deny",4);
    check_file_ret(res,"write to",setgroups_f);
    close(fd);                             
    fd=open(uid_map_f,O_WRONLY);
    check_file_ret(fd,"open",uid_map_f);
    res=write(fd,uid_map,uid_len);          
    check_file_ret(res,"write to",uid_map_f);
    close(fd);                             
    fd=open(gid_map_f,O_WRONLY);
    check_file_ret(fd,"open",gid_map_f);
    res=write(fd,gid_map,gid_len); 
    check_file_ret(res,"write to",gid_map_f);
    close(fd);
    return 0;    
}
