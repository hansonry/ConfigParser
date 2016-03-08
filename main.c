#include <stdio.h>
#include "ConfigParser.h"


static void PrintValue(CPValue_T * value)
{
   size_t i;
   if(value->type == e_CPVT_String)
   {
      printf("%.*s\n", (int)value->data.string.token->size, value->data.string.token->start);
   }
   else if(value->type == e_CPVT_Array)
   {
      printf("[\n");
      for(i = 0; i < value->data.array.size; i++)
      {
         PrintValue(value->data.array.value_list[i]);
      }
      printf("]\n");
   }
   else if(value->type == e_CPVT_Object)
   {
      printf("{\n");
      for(i = 0; i < value->data.object.size; i++)
      {
        
         printf("%.*s :\n", (int)value->data.object.pair_list[i].key_token->size, value->data.object.pair_list[i].key_token->start);
         PrintValue(value->data.object.pair_list[i].value);
      }
      printf("}\n");
   }
}

int main(int args, char * argc[])
{
   ConfigParser_T parser;
   ConfigParser_Init(&parser);
   ConfigParser_LoadFile(&parser, "test.txt");


   PrintValue(parser.root);


   ConfigParser_Destory(&parser);
   printf("End\n");
   return 0;
}


