
#include "db.h"

#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>


#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


#define FNAME_BYTES 32 //Characters allowed for filename, including null-char
#define MAX_DB_SIZE 8192 //Maximum bytes 
#define MAX_DB_ENTRIES 1024 

#define DEBUG 0
#define __STDC_FORMAT_MACROS

#define DEBUG_PRINT(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "DEBUG:: %s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, ##__VA_ARGS__); } while (0)

struct db
{
    int fd;
    size_t count;
    size_t max_entries; 
    char filename[FNAME_BYTES];       //includes '\0' char 
    uint64_t *db_map_file;            //memory mapped file ptr
};


//Prototyes for db.c
static void _db_info(db_t *); 
static size_t _fsize(const char*);
static void _set_db_name(db_t *);
static size_t _reduce_db(db_t *, size_t );
static int _internal_db_find(db_t *, uint64_t , size_t *);


/*******************************************
*
* db.c APIs
*
********************************************/

db_t * db_new()
{
    DEBUG_PRINT("allocating space for new database\n");
    db_t * db = malloc(sizeof(db_t));
    if( db == NULL){
        perror("db_new: malloc failed");
        return NULL;
    }

    db->fd = 0;
    db->max_entries = MAX_DB_ENTRIES; 
    db->count = 0;
    db->db_map_file = 0;
    DEBUG_PRINT("zero-ing db file name buffer\n");
    bzero(db->filename, FNAME_BYTES);
    DEBUG_PRINT("new db object created: %p\n", &db);
    return db;
}


void db_free(db_t * db)
{

    int return_code= 0;
    // create filename if unset 
    if(db->filename[0]==0){
        DEBUG_PRINT("DB has no filename\n"); 
        DEBUG_PRINT("Setting filename to date-time string: %s\n", db->filename);
        _set_db_name(db);
    }
  
    DEBUG_PRINT("freeing resource from db: %s\n", db->filename);
   
   
    DEBUG_PRINT("syncing db before closing... \n"); 
    if( msync( db->db_map_file, MAX_DB_SIZE, MS_ASYNC) < 0){
        perror("db_free: failed syncing update:: error code %d");
        fprintf(stderr, "errno value %d: \n", errno);
        return;
    }
    DEBUG_PRINT("syncing returned no errors... \n"); 
    
    DEBUG_PRINT("unmapping db memory file... \n"); 
    return_code = munmap(db->db_map_file, db->count); //TODO inspect value 
    DEBUG_PRINT("syncing returned no errors... \n"); 
    DEBUG_PRINT("truncating underlying file... \n");
    return_code = ftruncate(db->fd, db->count*sizeof(uint64_t)); //TODO: inspect valud
    DEBUG_PRINT("truncated underlying file... \n");
    DEBUG_PRINT("closing %d file descripter... \n", db->fd); 
    return_code = close(db->fd); //TODO: inspect value 
    DEBUG_PRINT("freeing db... \n"); 
    free(db);
    DEBUG_PRINT("freed db... done... \n"); 
}



int db_open(db_t * db, const char * filename)
{
    int fd = 0;
    struct stat sb; 
    int e = 0; //return value, possible error
    size_t num_entries = 0;
    size_t file_size = 0;
    
    DEBUG_PRINT("Checking if %s file ok...\n", filename);
    int access_status = access(filename, F_OK|R_OK|W_OK);
    
    DEBUG_PRINT("access_status: %d\n", access_status);
    if( access_status == 0){
        DEBUG_PRINT(" existing file okay...");
        fd = open(filename, O_RDWR, S_IRUSR|S_IWUSR);
        DEBUG_PRINT("file descriptor (existing file): %d\n", fd);
        if( fd < 0){
            DEBUG_PRINT("error opening file...\n");
            perror("db_open: error opening file\n");
            return -1;
        }

        
        e = fstat(fd, &sb);
        if( e == -1){
            DEBUG_PRINT("error getting file stats...\n");
            perror("error getting file stats\n");
        }
        file_size = sb.st_size;
        DEBUG_PRINT("file size: %zu", file_size);
        if( file_size > MAX_DB_SIZE){
            DEBUG_PRINT("underlying file size exceeds MAX_DB_SIZE...\n");
            DEBUG_PRINT("setting db size to MAX_DB_SIZE... db is full...\n");
            file_size = MAX_DB_SIZE;
        }
        if( (file_size % sizeof(uint64_t)) != 0){
            DEBUG_PRINT("db is not multile of 8 byte, will read only \
                            a multiple of 8 bytes...\n");
            file_size >>= 3; //divde by 8
            file_size <<= 3; ///mutliple
        } 
        DEBUG_PRINT("read return status:: %d", e); 
        db->count = (file_size>>3); 
    }
    else{ //create file; new database 
        fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        DEBUG_PRINT("file descriptor (new file): %d\n", fd);
        if( fd < 0){
            perror("db_open: error opening new file");
            return -1;
        }
        //REMARK: this should be the total bytes allotted to the db file
        file_size = MAX_DB_SIZE; 
    }
     
    db->fd = fd;
    DEBUG_PRINT(" db file descript set: %d\n", fd);
    e = ftruncate(fd, MAX_DB_SIZE);
    DEBUG_PRINT(" ftruncate return status:: %d\n", e);
    db->db_map_file = (uint64_t *)mmap(NULL, file_size, PROT_READ|PROT_WRITE, 
                                             MAP_SHARED, fd, 0);

    DEBUG_PRINT(" map file created...\n");
    if( db->db_map_file == MAP_FAILED){
        perror("db_open: failed making the map file\n");
        db->db_map_file = 0;
        return -1;
    }

    if(access_status==0){
        DEBUG_PRINT(" reading file into mmap file\n");
        e = read(fd, db->db_map_file, file_size);
        DEBUG_PRINT("read syscall result: %d\n", e);
        
        if(e == -1){
            perror("error reading from existing db file");
            return errno;
        }


        lseek(fd, 0, SEEK_SET);
        DEBUG_PRINT("reset file offset o origin\n"); 
    }

    strncpy(db->filename, filename, FNAME_BYTES-1);
    db->filename[FNAME_BYTES-1] = 0;
    if(DEBUG){
        _db_info(db);
    }
    return 0;
}



int db_insert(db_t *db, uint64_t id)
{
    size_t  index = db->count; 
    int e = _internal_db_find(db, id, &index);
    if( e == 0){
        if( db->count == MAX_DB_ENTRIES){
            DEBUG_PRINT("db is full, db cout:: %zu\n", db->count);
            return ENOMEM;
        }
 
        db->db_map_file[index] = id;
        db->count++;
    }
    else{
        DEBUG_PRINT("value exist in db:: %"PRIu64"\n", id);
        return EEXIST;
    }

    if( msync( db->db_map_file, MAX_DB_SIZE, MS_ASYNC) < 0){
        perror("db_insert: failed syncing update");
        return errno;
    }
   
    if(DEBUG){ 
        _db_info(db);
    } 
    return e;
}


int db_delete(db_t *db, uint64_t id)
{
    size_t index=0;
    int e = 0;
    if( _internal_db_find(db, id, &index)){
        DEBUG_PRINT("id: %"PRIu64" found in db\n", id);
        db->db_map_file[index] = 0;
        e = 1;
        _reduce_db(db, index); 
    }
  
    if(DEBUG){ 
        _db_info(db);
    } 
    return e;
}


int db_find(db_t *db, uint64_t id)
{
    uint64_t index = 0;
    return _internal_db_find(db, id, &index);
}

/*******************************************
*
* db.c utility functions 
*
********************************************/

/**
*   Shifts keys to origin by copying, database doesn't include zeros. Unless added
*   @param db database object 
*   @param origin index where to begin overwrites 
*   
*/
static size_t _reduce_db(db_t * db, size_t origin)
{
    size_t dst_index = origin;
    size_t src_index = origin;
    DEBUG_PRINT("reducing db...\n");

    while(src_index < db->count){
        db->db_map_file[dst_index] = db->db_map_file[src_index];
        if( db->db_map_file[dst_index] != 0){
           dst_index++;
        }
        src_index++;
    }

    //TODO: verify logic
    db->count = dst_index;
    return dst_index; //will be number of id entries in db 
}


/**
*   Find function
*   @param db a database object 
*   @param id the key of interest
*   @param index a pointer where to save the id's index
*/
static int _internal_db_find(db_t *db, uint64_t id, uint64_t *index)
{
    int ret_value = 0;
    size_t i = 0;
    while( i < db->count){
        if( db->db_map_file[i] == id){
            *index = i;
            ret_value = 1;
            break; 
        }
        i++; 
    }
    return ret_value;
}


/*
*
*   Sets filename to current date-time string 
*   @param db a database object 
*
*/
static void _set_db_name(db_t * db)
{
    DEBUG_PRINT("setting datetime string name for db...\n");
    time_t t = time(NULL);
    struct tm * lt = localtime( &t);
    strftime(db->filename, FNAME_BYTES, "%F+%T.db", lt);
}


/*
*
*   Get file size 
*   @param fname file name 
*
*/
static size_t _fsize(const  char * fname)
{
    struct stat s;
    int sval = stat(fname, &s);
    return s.st_size; 
}


/*
*
*  Debug method to display db record
*  @param db a database object
*
*/
static void _db_info(db_t * db)
{
    char last_char = db->filename[FNAME_BYTES-1];
    db->filename[FNAME_BYTES-1] = 0;
    printf("\n***showing db object values***\n");
    printf("db name = %s\n", db->filename);
    printf("db address = %p\n", db);
    printf("db.fd = %d \n", db->fd);
    printf("db.count = %zu \n", db->count);
    printf("db.max_entries = %zu \n", db->max_entries);
    printf("db map address = %p\n", db->db_map_file);
    db->filename[FNAME_BYTES-1] = last_char;
}
