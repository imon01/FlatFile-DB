
#include "db.h"

#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <sys/stat.h>
#include <sys/types.h>


#define SET8_BYTES 0xFFFFFFFF
#define WORD 1
#define FNAME_BYTES 32 //Characters allowed for filename
#define MAX_DB_SIZE 8192 
#define MAX_DB_ENTRIES 1024 


/*
   Type definitions for db
*/


struct db
{
    size_t count;
    size_t max_entries; 
    char filename[FNAME_BYTES]; //Bytes allocated for filename, including '\0' character
    uint64_t id_array[MAX_DB_ENTRIES];//multiple of eight 
};


static size_t fsize(const char*);
static void set_dt_name(db_t *);
static size_t reduce_db(db_t *, size_t );
static int internal_db_find(db_t *, uint64_t , size_t *);


db_t * db_new()
{
    db_t * db = malloc(sizeof(db_t));
    if( db == NULL){
        perror("malloc failed");
    }

    db->max_entries = MAX_DB_ENTRIES; 
    db->count = 0;
    bzero(db->filename, FNAME_BYTES);
    bzero(db->id_array, MAX_DB_ENTRIES);
    
    return db;
}


void db_free(db_t * db){
    size_t dbsize = reduce_db(db, 0);


    //create filename if unset 
    if(db->filename[0]==0){
        set_dt_name(db);
    }


    FILE * fptr = fopen(db->filename, "w+");//check if file exist, otherwise create file with some name
    if(fptr==NULL){
        perror("error opeingin file for closing");
    }
    fwrite(db->id_array, sizeof(uint64_t), dbsize, fptr);
    fclose(fptr);
    free(db);
}



int db_open(db_t * db, const char * filename)
{
    
    FILE * fptr;
    int ret_value = 0;
    size_t num_entries = 0;
     
    if( access(filename, F_OK) == 0){
        fptr = fopen(filename, "r");
        num_entries = fsize(filename);
         
        if(db->id_array == NULL){
            perror("malloc failed on allocating id_array");
        }
        
        num_entries /=sizeof(uint64_t); //reuse variable 
        ret_value = fread(db->id_array,  sizeof(uint64_t), num_entries, fptr);
        db->count = num_entries; 
    }
    else{ //create file; new database 
        fptr = fopen(filename, "w+");
        if(db->id_array == NULL){
            perror("malloc failed on allocating id_array");
        }
    } 

    ret_value = fclose(fptr); 
    strncpy(db->filename, filename, FNAME_BYTES);
    return 0;
}



int db_insert(db_t *db, uint64_t id)
{
    //check if enough space
   
    size_t  index = db->count; 
    int ret_value = internal_db_find(db, id, &index);
    if( ret_value == 0){
        if( db->count == db->max_entries){
            return ENOMEM;
        }

        //Case: zero not inserted and id != 0; one remaining spot
 
        db->id_array[index] = id;
        ret_value = 0;
        db->count++;
    }
    else{ //value found in db
        ret_value = EEXIST;
    }
 
    return ret_value;
}


int db_delete(db_t *db, uint64_t id)//TODO error handling
{
    size_t index=0;
    int ret_value = 0;
    if( internal_db_find(db, id, &index)){
        db->id_array[index] = 0;
        ret_value = 1;
        reduce_db(db, index); 
    }
   
    return ret_value; //
}


int db_find(db_t *db, uint64_t id)//TODO error handling
{
    uint64_t index = 0;
    return internal_db_find(db, id, &index);
}


/**
*   Shifts keys to origin by copying, database doesn't include zeros. Unless added
*   
*/
static size_t reduce_db(db_t * db, size_t origin)
{
    size_t dst_index = origin;
    size_t src_index = origin;


    while(src_index < db->count){
        db->id_array[dst_index] = db->id_array[src_index];
        if( db->id_array[dst_index] != 0){
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
static int internal_db_find(db_t *db, uint64_t id, uint64_t *index)
{
    int ret_value = 0;
    size_t i = 0;
    while( i < db->count){
        if( db->id_array[i] == id){
            *index = i;
            ret_value = 1;
            break; 
        }
        i++; 
    }
    return ret_value;
}


/**
*   Sets filename to current date-time string 
*   @param db a database object 
*/
static void set_dt_name(db_t * db)
{
    time_t t = time(NULL);
    struct tm * lt = localtime( &t);
    strftime(db->filename, FNAME_BYTES, "%F+%T.db", lt);
}

static size_t fsize(const  char * fname)
{
    struct stat s;
    int sval = stat(fname, &s);
    return s.st_size; 
}
