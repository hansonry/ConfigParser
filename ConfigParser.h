#ifndef __CONFIGPARSER_H__
#define __CONFIGPARSER_H__
#include <stddef.h>

typedef struct ConfigParser_S ConfigParser_T;


typedef struct CPToken_S CPToken_T;
typedef enum CPTokenType_E CPTokenType_T;

enum CPTokenType_E
{
   e_CPTT_Setter, // Colen for our case
   e_CPTT_Seperator, // Comma for our case
   e_CPTT_StructBegin,
   e_CPTT_StructEnd,
   e_CPTT_ArrayBegin,
   e_CPTT_ArrayEnd,
   e_CPTT_String
};

struct CPToken_S
{
   const char * start;
   size_t size;
   CPTokenType_T type;
   CPToken_T * next;
};



struct ConfigParser_S
{
   char * buffer;
   size_t buffer_size;
   CPToken_T * token_root;

};

void ConfigParser_Init(ConfigParser_T * parser);
void ConfigParser_LoadFile(ConfigParser_T * parser, const char * filename);




#endif // __CONFIGPARSER_H__


