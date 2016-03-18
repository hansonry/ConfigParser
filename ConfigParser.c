#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ConfigParser.h"

typedef struct ParserData_S ParserData_T;

struct ParserData_S
{
   int error;
   size_t index;
};

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
            ConfigParser_DestroyValue(parser, &value->data.array.value_list[i]);
         }
         free(value->data.array.value_list);
      }
      else if(value->type == e_CPVT_Object)
      {
         for(i = 0; i < value->data.object.size; i++)
         {
            ConfigParser_DestroyValue(parser, value->data.object.pair_list[i].value);
         }
         free(value->data.object.pair_list);
      }
      

   }
}

void ConfigParser_Destory(ConfigParser_T * parser)
{
   ConfigParser_DestroyValue(parser, parser->root);
   free(parser->root);
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

static int ConfigParser_ParseValue(ConfigParser_T * parser, ParserData_T * data, CPValue_T * value);

static CPToken_T * ConfigParser_GetToken(ConfigParser_T * parser, ParserData_T * data)
{
   CPToken_T * result;
   
   if(data->index < parser->token_count)
   {
      result = &parser->token_list[data->index];
   }
   else
   {
      result = NULL;
   }
   return result;
}

static int ConfigParser_ParseBasicToken(ConfigParser_T * parser, ParserData_T * data, CPTokenType_T type)
{
   int is_type;
   CPToken_T * token;
   token = ConfigParser_GetToken(parser, data);
   if(token != NULL && token->type == type)
   {
      is_type = 1;
      data->index ++;
   }
   else
   {
      is_type = 0;
   }

   return is_type;
}


static int ConfigParser_ParseObjectKey(ConfigParser_T * parser, ParserData_T * data, CPObjectPair_T * pair)
{
   int is_key;   
   CPToken_T * token;
   token = ConfigParser_GetToken(parser, data);
   if(token != NULL && token->type == e_CPTT_String)
   {
      is_key = 1;
      data->index ++;
      pair->key_token = token;
   }
   else
   {
      is_key = 0;
   }

   return is_key;
}

static int ConfigParser_ParseObjectPair(ConfigParser_T * parser, ParserData_T * data, CPObjectPair_T * pair)
{
   CPValue_T value;
   int is_pair;
   if(ConfigParser_ParseObjectKey(parser, data, pair))
   {
      is_pair = 1;
      if(ConfigParser_ParseBasicToken(parser, data, e_CPTT_Setter))
      {
         if(ConfigParser_ParseValue(parser, data, & value))
         {
            pair->value = malloc(sizeof(CPValue_T));
            memcpy(pair->value, &value, sizeof(CPValue_T));
         }
         else
         {
            data->error ++;
            pair->value = NULL;
            printf("Error: ConfigParser_ParserObjectPair: Ecpected Value\n");
         }
      }
      else
      {
         data->error ++;
         pair->value = NULL;
         printf("Error: ConfigParser_ParseObjectPair: Expected ':'.\n");
      }
   }
   else
   {
      is_pair = 0;
   }

   return is_pair;
}


static int ConfigParser_ParseObject(ConfigParser_T * parser, ParserData_T * data, CPValueObject_T * object)
{
   int is_object;
   int done;
   size_t size;
   int last_error;
   CPToken_T * token;
   CPObjectPair_T pair;
   object->size = 0;
   size = GROW_BY;
   object->pair_list = malloc(sizeof(CPObjectPair_T) * size);
   if(ConfigParser_ParseBasicToken(parser, data, e_CPTT_StructBegin))
   {
      is_object = 1;
      done = 0;
      while(done == 0)
      {
         last_error = data->error;
         if(ConfigParser_ParseObjectPair(parser, data, &pair))
         {
            // Grow if nessary
            if(object->size >= size)
            {
               size = size + GROW_BY;
               object->pair_list = realloc(object->pair_list, sizeof(CPObjectPair_T) * size);
            }
            memcpy(&object->pair_list[object->size], &pair, sizeof(CPObjectPair_T));
            object->size ++;

            if(last_error == data->error) // No error
            {

               if(ConfigParser_ParseBasicToken(parser, data, e_CPTT_StructEnd))
               {
                  done = 1;
               }
               else if(!ConfigParser_ParseBasicToken(parser, data, e_CPTT_Seperator))
               {
                  done = 1;
                  data->error ++;
                  printf("Error: ConfigParser_ParseObject: Expected ','\n");
               }
            }
            else
            {
               done = 1;
            }
         }
         else
         {
            printf("Error: ConfigParser_ParseObject: Expected Object Pair\n");
            data->error ++;
            done = 1;
         }


      }

   }
   else
   {
      is_object = 0;
   }
   
   return is_object;
}

static int ConfigParser_ParseArray(ConfigParser_T * parser, ParserData_T * data, CPValueArray_T * array)
{
   int done;
   int is_array;
   size_t size;
   int last_error;
   CPValue_T value;
   array->size = 0;
   size = GROW_BY;
   array->value_list = malloc(sizeof(CPValue_T) * size);

   if(ConfigParser_ParseBasicToken(parser, data, e_CPTT_ArrayBegin))
   {
      is_array = 1;
      done = 0;
      while(done == 0)
      {
         last_error = data->error;
         if(ConfigParser_ParseValue(parser, data, &value))
         {
            // Grow if nessary
            if(array->size >= size)
            {
               size = size + GROW_BY;
               array->value_list = realloc(array->value_list, sizeof(CPValue_T) * size);
            }
            memcpy(&array->value_list[array->size], &value, sizeof(CPValue_T));
            array->size ++;

            if(last_error == data->error) // No errors
            {


               if(ConfigParser_ParseBasicToken(parser, data, e_CPTT_ArrayEnd))
               {
                  done = 1;
               }
               else if(!ConfigParser_ParseBasicToken(parser, data, e_CPTT_Seperator))
               {
                  done = 1;
                  data->error ++;
                  printf("Error: ConfigParser_ParseArray: Expected ','.\n");

               }

            }
            else
            {
               done = 1;
            }
         }
         else
         {
            printf("Error: ConfigParser_ParseArray: Expected Value\n");
            done = 1;
            data->error ++;
         }
      }
   }
   else
   {
      is_array = 0;
   }
   return is_array;
}

static int ConfigParser_ParseStringValue(ConfigParser_T * parser, ParserData_T * data, CPValueString_T * str)
{
   int is_string;
   CPToken_T * token;
   token = ConfigParser_GetToken(parser, data);
   if(token != NULL && token->type == e_CPTT_String)
   {
      is_string = 1;
      data->index ++;
      str->token = token;
   }
   else
   {
      is_string = 0;
   }
   return is_string;
}

static int ConfigParser_ParseValue(ConfigParser_T * parser, ParserData_T * data, CPValue_T * value)
{
   CPToken_T * token;
   int is_value;
   if(ConfigParser_ParseStringValue(parser, data, &value->data.string))
   {
      value->type = e_CPVT_String;
      is_value = 1;
   }
   else if(ConfigParser_ParseArray(parser, data, &value->data.array))
   {
      value->type = e_CPVT_Array;
      is_value = 1;
   }
   else if(ConfigParser_ParseObject(parser, data, &value->data.object))
   {
      value->type = e_CPVT_Object;
      is_value = 1;
   }
   else
   {
      is_value = 0;
   }
   return is_value;
}

static int ConfigParser_ParseTokens(ConfigParser_T * parser)
{
   ParserData_T data;
   int result;
   CPValue_T value;
   data.error = 0;
   data.index = 0;
   if(ConfigParser_ParseValue(parser, &data, &value))
   {
      parser->root = malloc(sizeof(CPValue_T));
      memcpy(parser->root, &value, sizeof(CPValue_T));
      if(data.error == 0)
      {
         if(data.index < parser->token_count && parser->token_list[data.index].type == e_CPTT_EndOfBuffer)
         {
            result = 1;
         }
         else
         {
            result = 0;
            data.error ++;
            printf("Error: ConfigParser_ParserTokens: Expected End of Buffer. Got \"%s\"\n", parser->token_list[data.index].str);
         }
      }
      else
      {
         result = 0;
      }

   }
   else 
   {
      result = 0;
      data.error ++;
      printf("Error: ConfigParser_ParseRokens: Expected Value\n");
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

