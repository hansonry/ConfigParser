#include <stdlib.h>
#include "ScannerWindow.h"

#define GROW_BY  16

void ScannerWindow_Init(ScannerWindow_T * window, Scanner_T * scanner)
{
   window->scanner    = scanner;
   window->list_size  = GROW_BY;
   window->list_count = 0;
   window->list       = malloc(sizeof(ScannerChar_T) * window->list_size);
}

void ScannerWindow_Destroy(ScannerWindow_T * window)
{
   free(window->list);
   window->scanner    = NULL;
   window->list       = NULL;
   window->list_size  = 0;
   window->list_count = 0;
}

size_t ScannerWindow_GetWindowSize(const ScannerWindow_T * window)
{
   return window->list_count;
}

static void ScannerWindow_GrowToSize(ScannerWindow_T * window, size_t size)
{
   int done;
   ScannerChar_T schar;
   // Check the buffer size
   if(size > window->list_size)
   {
      window->list_size += GROW_BY;
      window->list = realloc(window->list, sizeof(ScannerChar_T) * window->list_size);
   }

   while((window->list_count) < size && 
         (Scanner_GetNextChar(window->scanner, &schar)))
   {
      memcpy(&window->list[window->list_count], &schar, sizeof(ScannerChar_T));
      window->list_count ++;      
   }

}

int    ScannerWindow_GetIndex(ScannerWindow_T * window, size_t index, ScannerChar_T * schar)
{
   int result;
   ScannerWindow_GrowToSize(window, index + 1);
   if(index < window->list_count)
   {
      result = 1;
      memcpy(schar, window->list[index], sizeof(ScannerChar_T));
   }
   else
   {
      result = 0;
   }
   return result;
}

void ScannerWindow_ReleaseFirst(ScannerWindow_T * window, size_t count)
{
   size_t i, diff;
   // Make sure we have the characters to release
   ScannerWindow_GrowToSize(window, count);


   for(i = 0; i < count; i++)
   {
      memcpy(&window->list[i], 
             &window->list[i + count], 
             sizeof(ScannerChar_T));
   }

   window->list_count -= count;

}

int  ScannerWindow_Match(ScannerWindow_T * window, size_t start, const char * str)
{
   size_t str_len, i;
   int result;
   str_len = strlen(str);
   ScannerWindow_GrowToSize(window, str_len + start);

   if(str_len + start <= window->list_count)
   {
      result = 1;
      for(i = 0; i < str_len; i++)
      {
         if(window->list[start + i].c != str[i])
         {
            result = 0;
            break;
         }
      }
   }
   else
   {
      result = 0;
   }
   return result;
}

size_t ScannerWindow_CopyString(ScannerWindow_T * window, size_t start, char * buffer, size_t buffer_size)
{
   size_t result, i;
   ScannerWindow_GrowToSize(window, buffer_size);

   if(start + buffer_size <= window->list_count)
   {
      result = buffer_size;
   }
   else
   {
      result = window->list_count - start;
   }

   for(i = 0; i < result; i++)
   {
      buffer[i] = window->list[start + i].c;
   }

   return result;
}



