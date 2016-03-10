#include "Scanner.h"
#include <string.h>

#define TYPE_BUFFER  0
#define TYPE_FILE    1

void Scanner_InitFromString(Scanner_T * scanner, const char * string)
{
   scanner->type = TYPE_BUFFER;
   scanner->file = NULL;
   scanner->index = 0;
   scanner->buffer = string;
   scanner->size = strlen(string);
   scanner->line = 1;
   scanner->col  = 1;
}

void Scanner_InitFromBuffer(Scanner_T * scanner, const char * buffer, size_t size)
{
   scanner->type = TYPE_BUFFER;
   scanner->file = NULL;
   scanner->index = 0;
   scanner->buffer = buffer;
   scanner->size = size;
   scanner->line = 1;
   scanner->col  = 1;
}

void Scanner_InitFromFile(Scanner_T * scanner, const char * filename)
{
   scanner->type = TYPE_FILE;
   scanner->file = fopen(filename, "r");
   scanner->index = 0;
   scanner->buffer = NULL;
   scanner->size = 0;
   scanner->line = 1;
   scanner->col  = 1;

   if(scanner->file == NULL)
   {
      printf("Error: Scanner_InitFromFile: Cant Open File: \"%s\"\n", filename);
   }
}

void Scanner_Destroy(Scanner_T * scanner)
{
   if(scanner->file != NULL)
   {
      fclose(scanner->file);
      scanner->file = NULL;
   }
   scanner->buffer = NULL;
}


static void Scanner_SetChar(Scanner_T * scanner, ScannerChar_T * schar, char c)
{

   if(scanner->col == 0 && c != '\r')
   {
      scanner->col = 1;
   }
   schar->line = scanner->line;
   schar->col  = scanner->col;
   schar->c    = c;

   if(c == '\n')
   {
      scanner->line ++;
      scanner->col = 0;
   }
   else if(scanner->col != 0);
   {
      scanner->col ++;
   }
}

static void Scanner_SetBadChar(ScannerChar_T * schar)
{
   schar->line = 0;
   schar->col  = 0;
   schar->c    = '\0';
}


int Scanner_GetNextChar(Scanner_T * scanner, ScannerChar_T * schar)
{
   int result;
   int f_char;
   if(scanner->type == TYPE_FILE)
   {
      if(scanner->file == NULL)
      {
         result = 0;
         Scanner_SetBadChar(schar);
      }
      else
      {
         f_char = fgetc(scanner->file);
         if(f_char == EOF)
         {
            fclose(scanner->file);
            scanner->file = NULL;
            Scanner_SetBadChar(schar);
            result = 0;
         }
         else
         {
            Scanner_SetChar(scanner, schar, f_char);
            result = 1;
         }

      }
   }
   else if(scanner->type == TYPE_BUFFER)
   {
      if(scanner->index < scanner->size)
      {
         Scanner_SetChar(scanner, schar, scanner->buffer[scanner->index]);
         scanner->index ++;
         result = 1;
      }
      else
      {
         Scanner_SetBadChar(schar);
         result = 0;
      }
   }
   else
   {
      result = 0;
   }
   return result;
}

