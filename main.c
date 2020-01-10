
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
 
int  process_args(int, char*[], char *, size_t);
 
int main(int argc, char * argv[]) 
{ 
    int fdpipe; 
    int ret_val; 
    const char * dbfifo = "/tmp/dbcli_pipe0"; 
    const char * debug = "/tmp/dbcli_debug"; 
    char const* which_pipe = dbfifo; 
    char buffer[1024];
    bzero(buffer, 1024); 
    //ret_val = mkfifo(dbfifo, 0666); 
    ret_val = process_args(argc, argv, buffer, 1024);

     
    if(argc == 2){
        which_pipe = debug;
    }
    
    fdpipe = open(which_pipe, O_RDWR); 
    write(fdpipe, buffer, ret_val); 
    close(fdpipe); 

    return 0; 
} 

/*
* Format argv as single string
*/
int process_args(int argc, char* argv[], char * buffer, size_t blen)
{
    
    size_t str_len;
    size_t total = 0, j = 0;
    for(int i =1; i < argc; i++){
        str_len = strlen(argv[i]);
        if( ( total + str_len+1)  > blen){
            break;
        }
        j = 0; 
        while( j < str_len){
             buffer[total++] = argv[i][j++];
        }
        buffer[total++] = '|';
    }
    return  total;
}

