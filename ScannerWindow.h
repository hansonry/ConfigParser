#ifndef __SCANNERWINDOW_H__
#define __SCANNERWINDOW_H__
#include "Scanner.h"

typedef struct ScannerWindow_S ScannerWindow_T;

struct ScannerWindow_S
{
   Scanner_T     * scanner;
   ScannerChar_T * list;
   size_t          list_size;
   size_t          list_count;

};

void ScannerWindow_Init(ScannerWindow_T * window, Scanner_T * scanner);
void ScannerWindow_Destroy(ScannerWindow_T * window);

size_t ScannerWindow_GetWindowSize(const ScannerWindow_T * window);
int    ScannerWindow_GetIndex(ScannerWindow_T * window, ScannerChar_T * schar, size_t index);

void ScannerWindow_Release(ScannerWindow_T * window, size_t count);
int  ScannerWindow_Match(ScannerWindow_T * window, size_t start, const char * str);
size_t ScannerWindow_CopyString(ScannerWindow_T * window, size_t start, char * buffer, size_t buffer_size);




#endif // __SCANNERWINDOW_H__

