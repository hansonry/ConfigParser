#ifndef __SCANNER_H__
#define __SCANNER_H__
#include <stddef.h>
#include <stdio.h>

typedef struct Scanner_S     Scanner_T;
typedef struct ScannerChar_S ScannerChar_T;

struct ScannerChar_S
{
   int line;
   int col;
   char c;
};

struct Scanner_S
{
   int type;
   const char * buffer;
   size_t size;
   FILE * file;
   size_t index;
   int line;
   int col;
};

void Scanner_InitFromString(Scanner_T * scanner, const char * string);
void Scanner_InitFromBuffer(Scanner_T * scanner, const char * buffer, size_t size);
void Scanner_InitFromFile(Scanner_T * scanner, const char * filename);
void Scanner_Destroy(Scanner_T * scanner);



int Scanner_GetNextChar(Scanner_T * scanner, ScannerChar_T * schar);

#endif // __SCANNER_H__

