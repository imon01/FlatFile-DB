#include "db.h"

#include <poll.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <inttypes.h>

#define FNAME_BYTES 32
#define MAX_DB_ENTRIES 1024
#define FUNC_TABLE_SIZE 3
#define DEBUG_TABLE_SIZE 1
#define __STDC_FORMAT_MACROS


#define DEBUG 0 
#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "DEBUG:: %s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, ##__VA_ARGS__); } while (0)

/*
* Declarations 
*
*/

struct db{
    int fd;
    size_t count;
    size_t max_entries;
    char filename[FNAME_BYTES];
    uint64_t * db_map_file;
};


typedef struct dbnode{
    db_t *db;
    struct dbnode * next;
}dbnode_t;


typedef struct function_map{
    const char * fname;
    int (*func) (db_t*, uint64_t);
} function_map;

typedef struct function_map_debug{
    const char * fname;
    void (*func) ();
} function_map_debug;


static void _w_db_open(db_t *, char*);
static int _w_find(db_t * db, uint64_t v);
static int _w_insert(db_t * db, uint64_t v);
static int _w_delete(db_t * db, uint64_t v);

static dbnode_t * _dbnode_new();
static db_t * _search_db(char *);
static db_t * _insert_new_db(char*);
static void _close_system();
static void _show_db_list();

static int _process(char mode, char*, db_t *, uint64_t id);
static void _unpackage_arg(char *, size_t, char *, size_t, size_t *);


/*
*
* Definitions
*/
static volatile int exit_system = 0; 
static dbnode_t * db_list_head;
static dbnode_t * db_list_tail;

static const function_map_debug debug_table[] = {
    {"show_db_list", _show_db_list}
};


static const function_map func_table[] = {
    {"find", _w_find},
    {"delete", _w_delete},
    {"insert", _w_insert},
};



void signal_handle(int sig){
    exit_system = 1;
    printf("system exit interrupt!\n"); 
}

int main(int argc, char * argv[])
{

    DEBUG_PRINT("DEBUG mode set\n");
    const char * pipe_in = "/tmp/dbcli_pipe0"; 
    const char * pipe_out = "/tmp/dbcli_pipe1";
    const char * pipe_debug = "/tmp/dbcli_debug";
 
    int fd_write, fd_read, poll_value, fd_rdebug;
    size_t index = 0;
    char buffer[1024];
    char dbname[64];
    char command[32];
    char id[64];
    uint64_t value_id = 0;
    db_list_head = _dbnode_new(); //issues with db_list persisting

    db_t * db = 0;
    ssize_t read_value; 
    bzero(buffer, 1024);
    if( signal(SIGINT, signal_handle) == SIG_ERR){
        perror("error registering signal\n");
        exit(1);
    }
    
    mkfifo(pipe_in, S_IFIFO|0640);
    mkfifo(pipe_out, S_IFIFO|0640);
    mkfifo(pipe_debug, S_IFIFO|0640);
    fd_write = open(pipe_out, O_RDWR);
    fd_read = open(pipe_in, O_RDWR|O_NONBLOCK);
    fd_rdebug = open(pipe_debug, O_RDWR|O_NONBLOCK);
    //T
    
    struct pollfd poll_fds[2];
    poll_fds[0].fd = fd_read;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = fd_write;
    poll_fds[1].events = POLLOUT;
    poll_fds[2].fd = fd_rdebug;
    poll_fds[2].events = POLLIN;
     
    for(;;){
        if(exit_system){
            printf("shutting down...\n");
            break;
        }
        
        poll_value = poll(poll_fds, 3, -1);
        if( poll_value == -1){
            perror("dbmanger: polling failed\n");
            break;
        }
        if( poll_fds[2].revents & POLLIN){
            bzero(buffer, 1024); 
            DEBUG_PRINT(" debug instruction received...\n");
            read_value = read(poll_fds[2].fd, buffer, 1024);
            bzero(command, 32);
            index = 0;
            _unpackage_arg(buffer, 1024, command, 32, &index);
            _process(0, command, 0, 0);
        } 
 
        if( poll_fds[0].revents & POLLIN){
            bzero(buffer, 1024);
            DEBUG_PRINT(" new data...\n");
            read_value = read(poll_fds[0].fd, buffer, 1024); 
            
            bzero(dbname, 64);
            bzero(command, 32);
            bzero(id, 64);
             
            index = 0;
            _unpackage_arg(buffer, 1024, dbname, 64, &index);
            _unpackage_arg(buffer, 1024, command, 32, &index);
            _unpackage_arg(buffer, 1024, id, 64, &index);
            value_id = strtoull(id, NULL, 10);
            DEBUG_PRINT(" db-filename: %s\n", dbname);
            DEBUG_PRINT(" command: %s\n", command);
            DEBUG_PRINT(" id: %llu\n", strtoull(id, NULL, 10));
    
            db = _search_db( dbname);
            if( db == 0){
                db = _insert_new_db(dbname); 
            }
            _process(0, command, db, value_id);
        }
    }
    printf("closing dbmanger...\n");
    close(fd_read);
    close(fd_write);
    _close_system();
    
    return 0;
}

/********************************************
*
* db.c API wrappers
*
*********************************************/

static int _w_find(db_t * db, uint64_t k){
    DEBUG_PRINT(" find value: %"PRIu64"\n", k); 
    int e = db_find(db, k);

    switch(e){
        case 1:
                fprintf(stderr, "value found\n");
                break;
        case 0:
                fprintf(stderr, "value NOT found\n");
                break;
        default:
            if( e == (-errno)){
                perror("db_open caused -errno...\n");
                fprintf(stderr, "value of errno: %d", errno);
            }
    }
 
    return 0;
}


static int _w_insert(db_t * db, uint64_t k){
    DEBUG_PRINT(" insert value: %"PRIu64"\n", k); 
    int e = db_insert(db, k);

    switch(e){
        case 0:
                fprintf(stderr, "db_insert success\n");
                break;
        case -EEXIST:
                fprintf(stderr, "value already in db\n");
                break;
        case ENOMEM:
                fprintf(stderr, "db out of memory\n");
                break;
    } 
    return 0;
}


static int _w_delete(db_t * db, uint64_t k){
    DEBUG_PRINT(" delete value: %"PRIu64"\n", k); 
    int e = db_delete(db, k);
    return 0;
}

static void _w_db_open(db_t *db, char * dbname)
{
    int return_code = db_open(db, dbname);
    
    if(return_code == 0){
        printf("db_open passed with success\n");
        return;
    }

    if( return_code == (-errno)){
        perror("db_open caused -errno...\n");
        fprintf(stderr, "value of errno: %d", errno);
    }
}


/********************************************
*
* Utility functions for dbmanager 
*
*********************************************/

static dbnode_t * _dbnode_new()
{
    dbnode_t * node = malloc(sizeof(dbnode_t));
    node->db = 0;
    node->next = 0;
    return node;
}


static db_t * _insert_new_db( char * dbname)
{
    DEBUG_PRINT(" inserting new db node\n");
    int return_code = 0;
    if( db_list_head->db == 0){
        db_list_head->db = db_new();
        return_code = db_open(db_list_head->db, dbname);
        db_list_tail = db_list_head; //VERIFY pointer updated 
        return db_list_head->db; 
    }

    db_list_tail->next  = _dbnode_new(); 
    db_list_tail = db_list_tail->next; 
    db_list_tail->db = db_new();
    db_list_tail->next = 0; 
    DEBUG_PRINT("db_list_tail updated, new dbnode insert at address: %p", &db_list_tail); 
    _w_db_open(db_list_tail->db, dbname);
    return db_list_tail->db;
}


static db_t * _search_db( char* dbname)
{
    DEBUG_PRINT(" searching for database: %s\n", dbname);
    dbnode_t * db_list = db_list_head;
    while( db_list != NULL && db_list->db != NULL ){ //NULL appears to
        if( strncmp(db_list->db->filename, dbname, FNAME_BYTES) == 0){
            DEBUG_PRINT(" database found: %s\n", dbname);
            return db_list->db;
        }
        db_list = db_list->next;
    }
    //ERROR 
    //WARNO 
    DEBUG_PRINT(" no mataching database found: %s\n", dbname);
    return 0;
}


static void _show_db_list()
{
    printf("existing databases...\n");
    dbnode_t * node = malloc(sizeof(dbnode_t));
    *node = *db_list_head;
    while( node){
        printf("%s\n", node->db->filename);
        node = node->next; 
    }
    free(node);
}


static void _close_system()
{
    DEBUG_PRINT(" closing db systems\n");
    dbnode_t * prev = db_list_head;
    dbnode_t * sto = db_list_head;
    db_list_head = db_list_head->next;
    while( db_list_head){
        db_free(prev->db);
        free(prev);
        prev = db_list_head;
        db_list_head = db_list_head->next;
    }
    db_free(prev->db);
    free(prev);
    DEBUG_PRINT(" freed db resources... address of db_list_start %p\n", &sto);
}

/********************************************
*
* Parser utility functions 
*
*********************************************/


static void _unpackage_arg(char * buffer, size_t blen,
                            char * dst, size_t dlen, size_t *index)
{
    size_t src_index =0;
    while( *index < blen && buffer[*index] != '|'){
    
        if( src_index > dlen){
            src_index--;
            break;
        }
       dst[src_index++] = buffer[ (*index)++];
    }
    (*index)++;
    dst[src_index] = '\0';
} 


static int _process(char mode, char * command, db_t * db, uint64_t id)
{
    switch(mode){
        case 0: 
            DEBUG_PRINT("_processing command %s\n", command); 
            int ret_value = 0; 
            for(int i =0; i < FUNC_TABLE_SIZE; i++){
                if( strcmp(command, func_table[i].fname) == 0){
                    ret_value = func_table[i].func(db, id);
                    break;
                } 
            }
            break;
        case -1:
            for(int i = 0; i < DEBUG_TABLE_SIZE; i++){
                if( strcmp(command, func_table[i].fname) == 0){
                    debug_table[i].func();
                    break;
                }
            } 
    }
    return 0;
}
