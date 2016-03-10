#ifndef __STRINGHASHTABEL_H__
#define __STRINGHASHTABEL_H__
#include <stddef.h>

typedef struct StringHashTable_S       StringHashTable_T;
typedef struct StringHashTableBucket_S StringHashTableBucket_T;


struct StringHashTableBucket_S
{
   char ** list;
   size_t count;
   size_t size;
};

struct StringHashTable_S
{
   StringHashTableBucket_T * bucket_list;
   size_t num_of_buckets;
};

void StringHashTable_Init(StringHashTable_T * table, size_t num_of_buckets);
void StringHashTable_Destroy(StringHashTable_T * table);

const char * StringHashTable_Check(const StringHashTable_T * table, const char * str);

const char * StringHashTable_Put(StringHashTable_T * table, const char * str);

void StringHashTable_Clear(StringHashTable_T * table);




#endif // __STRINGHASHTABEL_H__

