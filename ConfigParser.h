#ifndef __CONFIGPARSER_H__
#define __CONFIGPARSER_H__
#include <stddef.h>

typedef struct ConfigParser_S ConfigParser_T;


typedef struct CPToken_S       CPToken_T;
typedef enum CPTokenType_E     CPTokenType_T;
typedef enum CPValueType_E     CPValueType_T;

typedef struct CPValue_S       CPValue_T;
typedef struct CPValueString_S CPValueString_T;
typedef struct CPValueArray_S  CPValueArray_T;
typedef struct CPValueObject_S CPValueObject_T;
typedef struct CPObjectPair_S  CPObjectPair_T;

enum CPValueType_E
{
   e_CPVT_String,
   e_CPVT_Array,
   e_CPVT_Object
};

enum CPTokenType_E
{
   e_CPTT_Setter, // Colen for our case
   e_CPTT_Seperator, // Comma for our case
   e_CPTT_StructBegin,
   e_CPTT_StructEnd,
   e_CPTT_ArrayBegin,
   e_CPTT_ArrayEnd,
   e_CPTT_String,
   e_CPTT_EndOfBuffer
};

struct CPToken_S
{
   const char * start;
   size_t size;
   CPTokenType_T type;
   CPToken_T * next;
};

struct CPValueString_S
{
   CPToken_T * token;
};

struct CPValueArray_S
{
   size_t size;
   CPValue_T ** value_list;
};

struct CPObjectPair_S
{
   CPToken_T * key_token;
   CPValue_T * value;
};

struct CPValueObject_S
{
   size_t size;
   CPObjectPair_T * pair_list;
};

struct CPValue_S
{
   CPValueType_T type;
   union 
   {
      CPValueString_T string;
      CPValueArray_T  array;
      CPValueObject_T object;
   } data;
};


struct ConfigParser_S
{
   char * buffer;
   size_t buffer_size;
   CPToken_T * token_root;
   CPValue_T * root;
};

void ConfigParser_Init(ConfigParser_T * parser);
void ConfigParser_Destory(ConfigParser_T * parser);

// Returns 1 if everything worked
int ConfigParser_LoadFile(ConfigParser_T * parser, const char * filename);

int ConfigParser_GetIndexOfKey(const CPValue_T * value, const char * key);


#endif // __CONFIGPARSER_H__


