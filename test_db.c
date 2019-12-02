#include "db.h"
#include "unity.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>


#include <sys/stat.h>


#define MAX_DB_ENTRIES 1024

const char * testdb = "testfile.db";

db_t * db;

void setUp(void){}
void tearDown(void){}

void test_insert_valid(void)
{
    size_t limit = MAX_DB_ENTRIES +1;
    for(size_t i = 1; i < limit; i++){
        TEST_ASSERT_EQUAL(0, db_insert(db, i));
    }
}


void test_insert_invalid_eexist(void)
{
    size_t limit = MAX_DB_ENTRIES +1;
    for(size_t i = 1; i < limit; i++){
        TEST_ASSERT_EQUAL(EEXIST, db_insert(db, i));
    }
}

void test_insert_invalid_nomem(void)
{
    TEST_ASSERT_EQUAL(ENOMEM, db_insert(db, MAX_DB_ENTRIES<<1));
    TEST_ASSERT_EQUAL(ENOMEM, db_insert(db, MAX_DB_ENTRIES<<2));
    TEST_ASSERT_EQUAL(ENOMEM, db_insert(db, MAX_DB_ENTRIES<<3));
}



void test_delete_valid(void)
{
    size_t i = MAX_DB_ENTRIES;
    for(; i > 0; i--){ 
        TEST_ASSERT_EQUAL(1, db_delete(db, i)); 
    }
}

void test_delete_invalid(void)
{
    TEST_ASSERT_EQUAL(0 , db_delete(db, 9)); 
    TEST_ASSERT_EQUAL(0 , db_delete(db, 1)); 
    TEST_ASSERT_EQUAL(0 , db_delete(db, 2)); 
}


void test_find_valid(void)
{
    size_t limit = MAX_DB_ENTRIES + 1;
    for(size_t i = 1; i < limit; i++){ 
        TEST_ASSERT_EQUAL(1, db_find(db, i)); 
    }
}

void test_find_invalid(void)
{
    size_t upper_limit = MAX_DB_ENTRIES<<2;
    for(size_t i = MAX_DB_ENTRIES + 1; i < upper_limit; i++){ 
        TEST_ASSERT_EQUAL(0, db_find(db, i)); 
    }
}


void test_db_open(void)
{
    TEST_ASSERT_EQUAL(0, db_open(db, testdb)); 
}



    

int main(void)
{
    db = db_new(); 
    UNITY_BEGIN();
    RUN_TEST(test_db_open); 
    RUN_TEST(test_insert_valid);
    RUN_TEST(test_insert_invalid_eexist);
    RUN_TEST(test_insert_invalid_nomem);
    //test opening existing file
    RUN_TEST(test_find_valid);
    RUN_TEST(test_find_invalid);
    RUN_TEST(test_delete_valid);
    RUN_TEST(test_delete_invalid);
    db_free(db);
    unlink(testdb);//remove underlying file
    return UNITY_END();
}
