#include <stdlib.h>
#include <string.h>
#include "StringHashTable.h"

#define GROW_BY                 16
#define DEFUALT_NUM_OF_BUCKETS  64

static unsigned long StringHashTable_Hash(const char * str)
{
   unsigned long hash;
   int c;
   hash = 5381;
   while(c = *str++) hash = ((hash << 5) + hash) + c;
   return hash;

}

static void StringHashTable_CreateBuckets(StringHashTable_T * table)
{
   size_t i;
   size_t new_size;
   table->bucket_list = malloc(sizeof(StringHashTableBucket_T) * table->num_of_buckets);

   for(i = 0; i < table->num_of_buckets; i++)
   {
      new_size = GROW_BY;
      table->bucket_list[i].size = new_size;
      table->bucket_list[i].count = 0;
      table->bucket_list[i].list = malloc(sizeof(char *) * new_size);
   }
}

void StringHashTable_Init(StringHashTable_T * table, size_t num_of_buckets)
{
   if(num_of_buckets == 0)
   {
      table->num_of_buckets = DEFUALT_NUM_OF_BUCKETS;
   }
   else
   {
      table->num_of_buckets = num_of_buckets;
   }
   table->bucket_list = NULL;
   StringHashTable_CreateBuckets(table);
}

void StringHashTable_Destroy(StringHashTable_T * table)
{
   size_t i, k;
   StringHashTableBucket_T * bucket;
   StringHashTable_Clear(table);

   for(i = 0; i < table->num_of_buckets; i++)
   {
      bucket = &table->bucket_list[i];
      free(bucket->list);
   }
   free(table->bucket_list);
}

const char * StringHashTable_Check(const StringHashTable_T * table, const char * str)
{
   size_t index, i;
   const char * result;
   StringHashTableBucket_T * bucket;
   index = StringHashTable_Hash(str) % table->num_of_buckets;
   bucket = &table->bucket_list[index];

   result = NULL;
   for(i = 0; i < bucket->count; i++)
   {
      if(strcmp(bucket->list[i], str) == 0)
      {
         result = bucket->list[i];
         break;
      }
   }

   return result;
}

const char * StringHashTable_Put(StringHashTable_T * table, const char * str)
{
   size_t index, i, str_size, new_size;
   const char * result;

   StringHashTableBucket_T * bucket;
   index = StringHashTable_Hash(str) % table->num_of_buckets;
   bucket = &table->bucket_list[index];

   result = NULL;
   for(i = 0; i < bucket->count; i++)
   {
      if(strcmp(bucket->list[i], str) == 0)
      {
         result = bucket->list[i];
         break;
      }
   }

   if(result == NULL)
   {
      // Grow list
      if(bucket->count >= bucket->size)
      {
         new_size = bucket->size + GROW_BY;
         bucket->size = new_size;
         bucket->list = realloc(bucket->list, sizeof(char *) * new_size);
      }

      str_size = strlen(str);
      bucket->list[bucket->count] = malloc(sizeof(char) * str_size + 1);
      strcpy(bucket->list[bucket->count], str);
      result = bucket->list[bucket->count];
      bucket->count ++;

   }

   return result;
}

void StringHashTable_Clear(StringHashTable_T * table)
{
   size_t i, k;
   StringHashTableBucket_T * bucket;

   for(i = 0; i < table->num_of_buckets; i++)
   {
      bucket = &table->bucket_list[i];
      for(k = 0; k < bucket->count; k ++)
      {
         if(bucket->list[k] != NULL)
         {
            free(bucket->list[k]);
         }         
      }
      bucket->count = 0;
   }
}

