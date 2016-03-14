#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ConfigParser.h"

#define GROW_BY 16

static int ConfigParser_ParseBuffer(ConfigParser_T * parser);

void ConfigParser_Init(ConfigParser_T * parser, Scanner_T * scanner)
{
   size_t new_size;
   new_size = GROW_BY;
   parser->token_size  = new_size;
   parser->token_count = 0;
   parser->token_list  = malloc(sizeof(CPToken_T) * new_size);
   parser->root        = NULL;

   ScannerWindow_Init(&parser->window, scanner);
   StringHashTable_Init(&parser->strings, 0);

   ConfigParser_ParseBuffer(parser);
}

static void ConfigParser_DestroyValue(ConfigParser_T * parser, CPValue_T * value)
{
   size_t i;
   if(value != NULL)
   {
      if(value->type == e_CPVT_Array)
      {
         for(i = 0; i < value->data.array.size; i++)
         {
            ConfigParser_DestroyValue(parser, value->data.array.value_list[i]);
         }
      }
      else if(value->type == e_CPVT_Object)
      {
         for(i = 0; i < value->data.object.size; i++)
         {
            ConfigParser_DestroyValue(parser, value->data.object.pair_list[i].value);
         }
      }

      free(value);
   }
}

void ConfigParser_Destory(ConfigParser_T * parser)
{
   ConfigParser_DestroyValue(parser, parser->root);
   parser->root = NULL;

   free(parser->token_list);
   parser->token_list = NULL;

   StringHashTable_Destroy(&parser->strings);
   ScannerWindow_Destroy(&parser->window);

}


int ConfigParser_LoadFile(ConfigParser_T * parser, Scanner_T * scanner)
{
   ScannerWindow_Init(&parser->window, scanner);
   return 1;
}

int ConfigParser_GetIndexOfKey(const CPValue_T * value, const char * key)
{
   int result;
   int i;
   size_t str_size;
   const CPObjectPair_T * pair;
   if(value->type == e_CPVT_Object && key != NULL)
   {
      result = -1;
      str_size = strlen(key);
      for(i = 0; i < (int)value->data.object.size; i++)
      {
         pair = &value->data.object.pair_list[i];
         if(strcmp(pair->key_token->str, key) == 0)
         {
            result = i;
            break;
         }
      }
   }
   else
   {
      result = -2;
   }
   return result;
}

#define IS_CHAR_NEWLINE(c)    ((c) == '\n' || (c) == '\r')
#define IS_CHAR_WHITESPACE(c) ((c) == ' ' || (c) == '\t' || IS_CHAR_NEWLINE(c))
#define IS_CHAR_DIGIT(c)      ((c) >= '0' || (c) <= '9')

static int ConfigParser_CheckToken(ConfigParser_T * parser, 
                                   CPToken_T * token, 
                                   const char * str)
{
   int result;
   ScannerChar_T schar;
   size_t size;

   if(ScannerWindow_Match(&parser->window, 0, str))
   {
      (void)ScannerWindow_GetIndex(&parser->window, &schar, 0);
      token->line = schar.line;
      token->col  = schar.col;
      token->str  = StringHashTable_Put(&parser->strings, str);
      size = strlen(str);
      ScannerWindow_Release(&parser->window, size);
      result = 1;
   }
   else
   {
      result = 0;
   }

   return result;
}

static void ConfigParser_EatBlockComment(ConfigParser_T * parser)
{
   ScannerChar_T schar;
   int done;
   size_t size;
   done = 0;
   while(ScannerWindow_GetIndex(&parser->window, &schar, 0) && done == 0)
   {
      if(ScannerWindow_Match(&parser->window, 0, "*/"))
      {
         done = 1;      
      }
      else
      {
         ScannerWindow_Release(&parser->window, 1);
      }
   }

   size = ScannerWindow_GetWindowSize(&parser->window);
   ScannerWindow_Release(&parser->window, size);

}

static void ConfigParser_EatLineComment(ConfigParser_T * parser)
{
   ScannerChar_T schar;
   int done, state;
   state = 0;
   done = 0;
   while(ScannerWindow_GetIndex(&parser->window, &schar, 0) && done == 0)
   {
      if(state == 0)
      {
         if(IS_CHAR_NEWLINE(schar.c))
         {
            state = 1;
         }
         ScannerWindow_Release(&parser->window, 1);
      }
      else if(state == 1)
      {
         if(!IS_CHAR_NEWLINE(schar.c))
         {
            state = 2;
            done  = 1;
         }
         else
         {
            ScannerWindow_Release(&parser->window, 1);
         }
      }
   }
}

static void ConfigParser_ReadString(ConfigParser_T * parser, 
                                    CPToken_T * token)
{
   char * str;
   ScannerChar_T schar;
   size_t index, start, size;
   int quote, done;


   ScannerWindow_GetIndex(&parser->window, &schar, 0);
   token->line = schar.line;
   token->col  = schar.col;

   if(schar.c == '"')
   {
      quote = 1;
      start = 1;
   }
   else
   {
      quote = 0;
      start = 0;
   }

   
   index = start;
   size = 0;
   done = 0;
   while(ScannerWindow_GetIndex(&parser->window, &schar, index) && done == 0)
   {
      if(quote == 1)
      {
         if(schar.c == '"')
         {
            done = 1;
         }
         else
         {
            size ++;
         }
      }
      else if(!(IS_CHAR_WHITESPACE(schar.c) || schar.c == ',' || schar.c == ':' || schar.c == '}' || schar.c == ']'))
      {
         size ++;
      }
      else
      {
         done = 1;
      }
      
      index ++;
   }
   // Create String
   str = malloc(sizeof(char) * (size + 1));
   ScannerWindow_CopyString(&parser->window, start, str, size);
   str[size] = '\0';

   token->str = StringHashTable_Put(&parser->strings, str);
   free(str);

   ScannerWindow_Release(&parser->window, start + size + quote);
  

}

static void ConfigParser_EatWhiteSpace(ConfigParser_T * parser)
{
   ScannerChar_T schar;
   int done;
   done = 0;
   while(ScannerWindow_GetIndex(&parser->window, &schar, 0) && done == 0)
   {
      if(!IS_CHAR_WHITESPACE(schar.c))
      {
         done = 1;      
      }
      else
      {
         ScannerWindow_Release(&parser->window, 1);
      }
   }
}

static void ConfigParser_AddToken(ConfigParser_T * parser, CPToken_T * token)
{
   size_t new_size;
   if(parser->token_count >= parser->token_size)
   {
      new_size = parser->token_count + GROW_BY;
      parser->token_list = realloc(parser->token_list, sizeof(CPToken_T) * new_size);
      parser->token_size = new_size;
   }
   memcpy(&parser->token_list[parser->token_count], 
          token, sizeof(CPToken_T));
   parser->token_count ++;
}

static void ConfigParser_Tokenize(ConfigParser_T * parser)
{
   ScannerChar_T schar;
   CPToken_T token;

   ConfigParser_EatWhiteSpace(parser);
   while(ScannerWindow_GetIndex(&parser->window, &schar, 0))
   {
      //printf("Looking at: %c\n", parser->buffer[index]);

      if(ConfigParser_CheckToken(parser, &token, "//"))
      {
         ConfigParser_EatLineComment(parser);
      }
      else if(ConfigParser_CheckToken(parser, &token, "/*"))
      {
         ConfigParser_EatBlockComment(parser);
      }
      else
      {
         if(ConfigParser_CheckToken(parser, &token, ":"))
         {
            token.type = e_CPTT_Setter;
         }
         else if(ConfigParser_CheckToken(parser, &token, ","))
         {
            token.type = e_CPTT_Seperator;
         }
         else if(ConfigParser_CheckToken(parser, &token, "{"))
         {
            token.type = e_CPTT_StructBegin;
         }
         else if(ConfigParser_CheckToken(parser, &token, "}"))
         {
            token.type = e_CPTT_StructEnd;
         }
         else if(ConfigParser_CheckToken(parser, &token, "["))
         {
            token.type = e_CPTT_ArrayBegin;
         }
         else if(ConfigParser_CheckToken(parser, &token, "]"))
         {
            token.type = e_CPTT_ArrayEnd;
         }
         else
         {
            ConfigParser_ReadString(parser, &token);
            token.type = e_CPTT_String;         
         }

         ConfigParser_AddToken(parser, &token);
      }

      ConfigParser_EatWhiteSpace(parser);
   }

   token.type = e_CPTT_EndOfBuffer;
   token.str  = NULL;
   token.line = 0;
   token.col  = 0;
   ConfigParser_AddToken(parser, &token);
}

static size_t ConfigParser_ParseValue(ConfigParser_T * parser, size_t index, CPValue_T ** value_ptr, int * error_flag);

static CPToken_T * ConfigParser_GetToken(ConfigParser_T * parser, size_t index, int * error_flag)
{
   CPToken_T * result;
   
   if(index < parser->token_count)
   {
      result = &parser->token_list[index];
      if(result->type == e_CPTT_EndOfBuffer)
      {
         result = NULL;
      }
   }
   else
   {
      result = NULL;
   }

   if(result == NULL)
   {
      *error_flag = 1;
      printf("Error: ConfigParser_GetToken: Unexpected End of Buffer\n");
   }
   return result;
}


static size_t ConfigParser_ParseObjectPair(ConfigParser_T * parser, size_t index, CPObjectPair_T * pair, int * error_flag)
{
   CPToken_T * token, * last_known_good_token;
   pair->value = NULL;
   token = ConfigParser_GetToken(parser, index, error_flag);
   last_known_good_token = token;
   if(token->type == e_CPTT_String)
   {
      pair->key_token = token;
      index ++;
      token = ConfigParser_GetToken(parser, index, error_flag);
      if(token->type == e_CPTT_Setter)
      {
         index ++;
         index = ConfigParser_ParseValue(parser, index, &pair->value, error_flag);
      }
      else
      {
         printf("Error: ConfigParser_ParseObjectPair: Expected ':'. Got: \"%s\"\n", token->str);
         *error_flag = 1;
      }
   }
   else
   {
      printf("Error: ConfigParser_ParseObjectPair: Expected Object Key Name. Got: \"%s\"\n", token->str);
      *error_flag = 1;
   }

   // If we have an error, then put some valid data in
   if(*error_flag == 1)
   {
      pair->key_token = last_known_good_token;
      pair->value = malloc(sizeof(CPValue_T));
      pair->value->type = e_CPVT_String;
      pair->value->data.string.token = last_known_good_token;
   }
  
   return index;
}

static size_t ConfigParser_ParseObject(ConfigParser_T * parser, size_t index, CPValueObject_T * object, int * error_flag)
{
   int done;
   size_t size;
   int seperator_satified;
   CPToken_T * token;
   object->size = 0;
   size = GROW_BY;
   object->pair_list = malloc(sizeof(CPObjectPair_T) * size);
   token = ConfigParser_GetToken(parser, index, error_flag);
   if(token->type == e_CPTT_StructBegin)
   {
      done = 0;
      seperator_satified = 1;
      index ++;
      while(done == 0 && index < parser->token_count)
      {
         token = ConfigParser_GetToken(parser, index, error_flag);
         if(token->type == e_CPTT_StructEnd)
         {
            done = 1;
            index ++;
         }
         else if(seperator_satified == 1)
         {
            // Grow if nessary
            if(object->size >= size)
            {
               size = size + GROW_BY;
               object->pair_list = realloc(object->pair_list, sizeof(CPObjectPair_T) * size);
            }

            index = ConfigParser_ParseObjectPair(parser, index, &object->pair_list[object->size], error_flag);
            object->size ++;

            token = ConfigParser_GetToken(parser, index, error_flag);
            if(*error_flag == 1)
            {
               done = 1;
            }
            else if(token->type == e_CPTT_Seperator)
            {
               seperator_satified = 1;
               index ++;
            }
            else
            {
               seperator_satified = 0;
            }
         }
         else
         {
            printf("Error: ConfigParser_ParseObject: Expected ','. Got \"%s\"\n", token->str);
            done = 1;
            *error_flag = 1;
         }
      }
      if(done == 0)
      {
         printf("Error: ConfigParser_ParseObject: Unexpteded End of Buffer");
         *error_flag = 1;
      }
   }
   else
   {
      printf("Error: ConfigParser_ParserObject: Expected '{'. Got: \"%s\"\n", token->str);
      *error_flag = 1;
   }
   return index;
}

static size_t ConfigParser_ParseArray(ConfigParser_T * parser, size_t index, CPValueArray_T * array, int * error_flag)
{
   int done;
   size_t size;
   int seperator_satified;
   CPToken_T * token;
   array->size = 0;
   size = GROW_BY;
   array->value_list = malloc(sizeof(CPValue_T *) * size);

   token = ConfigParser_GetToken(parser, index, error_flag);
   if(token->type == e_CPTT_ArrayBegin)
   {
      done = 0;
      seperator_satified = 1;
      index ++;
      while(done == 0 && index < parser->token_count)
      {
         token = ConfigParser_GetToken(parser, index, error_flag);
         if(token->type == e_CPTT_ArrayEnd)
         {
            done = 1;
            index ++;
         }
         else if(seperator_satified == 1)
         {
            // Grow if nessary
            if(array->size >= size)
            {
               size = size + GROW_BY;
               array->value_list = realloc(array->value_list, sizeof(CPValue_T *) * size);
            }

            index = ConfigParser_ParseValue(parser, index, &array->value_list[array->size], error_flag);
            array->size ++;
            token = ConfigParser_GetToken(parser, index, error_flag);
            if(*error_flag == 1)
            {
               done = 1;
            }
            else if(token->type == e_CPTT_Seperator)
            {
               index ++;
               seperator_satified = 1;
            }
            else
            {
               seperator_satified = 0;
            }
         }
         else
         {
            printf("Error: ConfigParser_ParseArray: Expected ','. Got \"%s\"\n", token->str);
            done = 1;
            *error_flag = 1;
         }
      }
      if(done == 0)
      {
         printf("Error: ConfigParser_ParseArray: Unexpteded End of Buffer");
         *error_flag = 1;
      }
   }
   else
   {
      printf("Error: ConfigParser_ParseArray: Expected first token to be \"[\". Instead got: \"%s\"\n", token->str);
      *error_flag = 1;
   }
   return index;
}

static size_t ConfigParser_ParseValue(ConfigParser_T * parser, size_t index, CPValue_T ** value_ptr, int *error_flag)
{
   CPValue_T * result;
   CPToken_T * token;
   token = ConfigParser_GetToken(parser, index, error_flag);
   if(index < parser->token_count)
   {
      if(token->type == e_CPTT_String)
      {
         result = malloc(sizeof(CPValue_T));
         result->type = e_CPVT_String;
         result->data.string.token = token;
         index ++;
      }
      else if(token->type == e_CPTT_ArrayBegin)
      {
         result = malloc(sizeof(CPValue_T));
         result->type = e_CPVT_Array;
         index = ConfigParser_ParseArray(parser, index, &result->data.array, error_flag);
      }
      else if(token->type == e_CPTT_StructBegin)
      {
         result = malloc(sizeof(CPValue_T));
         result->type = e_CPVT_Object;
         index = ConfigParser_ParseObject(parser, index, &result->data.object, error_flag);
      }
      else
      {
         result = NULL;
         *error_flag = 1;
         printf("Error: ConfigParser_ParseValue: Recived Unexpteded Token \"%s\"\n", token->str);
      }
   }
   else
   {
      printf("Error: ConfigParser_ParseValue: Unexpted End of Buffer\n");
      *error_flag = 1;
      result = NULL;
   }

   (*value_ptr) = result;

   return index;
}

static int ConfigParser_ParseTokens(ConfigParser_T * parser)
{
   size_t index;
   int result;
   int error_flag;
   error_flag = 0;
   index = ConfigParser_ParseValue(parser, 0, &parser->root, &error_flag);
   if(error_flag == 1)
   {
      printf("An Error Occured Durring Parsing\n");
      result = 0;
   }
   else if(parser->token_list[index].type != e_CPTT_EndOfBuffer)
   {
      printf("Error: ConfigParser_ParserTokens: Expected End of Buffer. Got \"%s\"\n", parser->token_list[index].str);
      result = 0;
   }
   else
   {
      result = 1;
   }
   return result;
}

static int ConfigParser_ParseBuffer(ConfigParser_T * parser)
{
   //size_t i;
   ConfigParser_Tokenize(parser);

   //for(i = 0; i < parser->token_count; i++)
   //{
   //   printf("T: %i  \"%s\"\n", i, parser->token_list[i].str);
   //}
   return ConfigParser_ParseTokens(parser);
}

