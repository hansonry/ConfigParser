#include <stdio.h>
#include "ConfigParser.h"

int main(int args, char * argc[])
{
   ConfigParser_T parser;
   ConfigParser_Init(&parser);
   ConfigParser_LoadFile(&parser, "test.txt");
   printf("End\n");
   return 0;
}

