#include "db.h"
#include<stdio.h>
#include<stdlib.h>

#include<unistd.h>

/* Definitions for database bookkeeping

*/


db_t *db;
int main(void)
{

    db_t * db = db_new();
    db_open(db, "test.db"); 
    db_insert(db, 12);
    db_insert(db, 13);
    db->count++;
    db->count--;

    db_free(db); 
    return 0;
}
