#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ConfigParser.h"

#define GROW_BY 16

static void ConfigParser_ParseBuffer(ConfigParser_T * parser);

void ConfigParser_Init(ConfigParser_T * parser)
{
   parser->buffer      = NULL;
   parser->buffer_size = 0;
   parser->token_root  = NULL;
   parser->root        = NULL;
}

void ConfigParser_Destory(ConfigParser_T * parser)
{
   CPToken_T * loop, * temp;
   loop = parser->token_root;
   while(loop != NULL)
   {
      temp = loop;
      loop = loop->next;
      free(temp);
   }
   parser->token_root = NULL;

   if(parser->buffer != NULL)
   {
      free(parser->buffer);
   }
}


void ConfigParser_LoadFile(ConfigParser_T * parser, const char * filename)
{
   FILE * file;

   file = fopen(filename, "rb");
   if(file == NULL)
   {
      printf("Error: ConfigParser_LoadFile: Cannot open file \"%s\"\n", filename);
   }
   else
   {
      fseek(file, 0, SEEK_END);
      parser->buffer_size = ftell(file);
      parser->buffer = malloc(sizeof(char) * parser->buffer_size);
      fseek(file, 0, SEEK_SET);
      fread(parser->buffer, parser->buffer_size, 1, file);
      fclose(file);
      ConfigParser_ParseBuffer(parser);

   }
}

#define IS_CHAR_NEWLINE(c)    ((c) == '\n' || (c) == '\r')
#define IS_CHAR_WHITESPACE(c) ((c) == ' ' || (c) == '\t' || IS_CHAR_NEWLINE(c))
#define IS_CHAR_DIGIT(c)      ((c) >= '0' || (c) <= '9')

static int ConfigParser_CheckToken(ConfigParser_T * parser, 
                                   CPToken_T * token, 
                                   size_t index, 
                                   const char * str)
{
   size_t str_size;
   size_t i;  
   int result;

   str_size = strlen(str);

   if(index + str_size <= parser->buffer_size)
   {
      result = 1;
      for(i = 0; i < str_size; i++)
      {
         if(parser->buffer[index + i] != str[i])
         {
            result = 0;
            break;
         }
      }

      if(result == 1)
      {
         token->size  = str_size;
         token->start = &parser->buffer[index];
      }
   }
   else
   {
      result = 0;
   }
   return result;
}

static size_t ConfigParser_FindEndCommentBlock(ConfigParser_T * parser,
                                               size_t index)
{
   size_t result;
   int state;
   char c;
   CPToken_T token;
   result = index;
   state = 0;
   result = parser->buffer_size;
   while(index < parser->buffer_size && state < 1)
   {
      
      if(ConfigParser_CheckToken(parser, &token, index, "*/"))
      {
         state = 1;
         result = index + token.size;
      }

      index++;
   }


   return result;
}

static size_t ConfigParser_FindStartOfNextLine(ConfigParser_T * parser,
                                               size_t index)
{
   size_t result;
   int state;
   char c;
   state = 0;
   result = parser->buffer_size;
   while(index < parser->buffer_size && state < 2)
   {
      c = parser->buffer[index];
      if(state == 0)
      {
         if(IS_CHAR_NEWLINE(c))
         {
            state = 1;
         }
      }
      else if(state == 1)
      {
         if(!IS_CHAR_NEWLINE(c))
         {
            state = 2;
            result = index;
         }
      }

      index++;
   }

   return result;
}

static size_t ConfigParser_ReadString(ConfigParser_T * parser, 
                                      CPToken_T * token, 
                                      size_t index)
{
   int quote, done;
   char c;
   size_t result;

   result = parser->buffer_size;

   c = parser->buffer[index];
   if(c == '"')
   {
      quote = 1;
      index ++;
   }
   else
   {
      quote = 0;
   }

   token->start = &parser->buffer[index];
   token->size = 0;

   done = 0;
   while(index < parser->buffer_size && done == 0)
   {
      c = parser->buffer[index];
      if(quote == 1)
      {
         if(c == '"')
         {
            done = 1;
            result = index + 1;
         }
         else
         {
            token->size ++;
         }
      }
      else if(!(IS_CHAR_WHITESPACE(c) || c == ',' || c == ':' || c == '}' || c == ']'))
      {
         token->size ++;
      }
      else
      {
         done = 1;
         result = index;
      }
      
      index ++;
   }

   return result;
}

static size_t ConfigParser_EatWhiteSpace(ConfigParser_T * parser, size_t index)
{
   char c;
   int done;
   size_t result;

   done = 0;
   result = parser->buffer_size;
   while(index < parser->buffer_size && done == 0)
   {
      c = parser->buffer[index];
      if(!IS_CHAR_WHITESPACE(c))
      {
         done = 1;
         result = index;
      }
      index ++;
   }
   return result;
}

static void ConfigParser_Tokenize(ConfigParser_T * parser)
{
   size_t index;
   CPToken_T token, *last_token, *new_token;
   int add_token_flag;

   last_token = NULL;
   token.next = NULL;
   index = ConfigParser_EatWhiteSpace(parser, 0);
   while(index < parser->buffer_size)
   {
      //printf("Looking at: %c\n", parser->buffer[index]);

      if(ConfigParser_CheckToken(parser, &token, index, ":"))
      {
         token.type = e_CPTT_Setter;
         index += token.size;
         add_token_flag = 1;
      }
      else if(ConfigParser_CheckToken(parser, &token, index, ","))
      {
         token.type = e_CPTT_Seperator;
         index += token.size;
         add_token_flag = 1;
      }
      else if(ConfigParser_CheckToken(parser, &token, index, "{"))
      {
         token.type = e_CPTT_StructBegin;
         index += token.size;
         add_token_flag = 1;
      }
      else if(ConfigParser_CheckToken(parser, &token, index, "}"))
      {
         token.type = e_CPTT_StructEnd;
         index += token.size;
         add_token_flag = 1;
      }
      else if(ConfigParser_CheckToken(parser, &token, index, "["))
      {
         token.type = e_CPTT_ArrayBegin;
         index += token.size;
         add_token_flag = 1;
      }
      else if(ConfigParser_CheckToken(parser, &token, index, "]"))
      {
         token.type = e_CPTT_ArrayEnd;
         index += token.size;
         add_token_flag = 1;
      }
      else if(ConfigParser_CheckToken(parser, &token, index, "//"))
      {
         index = ConfigParser_FindStartOfNextLine(parser, index + token.size);
         add_token_flag = 0;
      }
      else if(ConfigParser_CheckToken(parser, &token, index, "/*"))
      {
         index = ConfigParser_FindEndCommentBlock(parser, index + token.size);
         add_token_flag = 0;
      }
      else
      {
         index = ConfigParser_ReadString(parser, &token, index);
         token.type = e_CPTT_String;         
         add_token_flag = 1;
      }


      if(add_token_flag == 1)
      {
         new_token = malloc(sizeof(CPToken_T));
         memcpy(new_token, &token, sizeof(CPToken_T));
         new_token->next = NULL;
         if(last_token == NULL)
         {
            parser->token_root = new_token;
         }
         else
         {
            last_token->next = new_token;
         }
         last_token = new_token;

      }

      index = ConfigParser_EatWhiteSpace(parser, index);
   }

}

static CPValue_T * ConfigParser_ParseValue(ConfigParser_T * parser, CPToken_T * token);

static int ConfigParser_ParseObjectPair(ConfigParser_T * parser, CPToken_T * token, CPObjectPair_T * pair)
{
   int result;
   if(token->type == e_CPTT_String)
   {
      pair->key_token = token;
      token = token->next;
      if(token->type == e_CPTT_Setter)
      {
         token = token->next;
         pair->value = ConfigParser_ParseValue(parser, token);
         if(pair->value == NULL)
         {
            result = 0;
         }
         else
         {
            result = 1;
         }

      }
      else
      {
         printf("Error: ConfigParser_ParseObjectPair: Expected ':'. Got: \"%.*s\"\n", token->size, token->start);
         result = 0;
      }
   }
   else
   {
      printf("Error: ConfigParser_ParseObjectPair: Expected Object Key Name. Got: \"%.*s\"\n", token->size, token->start);
      result = 0;
   }
   return result;
}

static void ConfigParser_ParseObject(ConfigParser_T * parser, CPToken_T * token, CPValueObject_T * object)
{
   int done;
   size_t size;
   int seperator_satified;
   object->size = 0;
   size = GROW_BY;
   object->pair_list = malloc(sizeof(CPObjectPair_T) * size);
   if(token->type == e_CPTT_StructBegin)
   {
      done = 0;
      seperator_satified = 1;
      while(done == 0)
      {
         token = token->next;
         if(token->type == e_CPTT_StructEnd)
         {
            done = 1;
         }
         else if(seperator_satified == 1)
         {
            // Grow if nessary
            if(object->size >= size)
            {
               size = size + GROW_BY;
               object->pair_list = realloc(object->pair_list, sizeof(CPObjectPair_T) * size);
            }

            if(!ConfigParser_ParseObjectPair(parser, token, &object->pair_list[object->size]))
            {
               done = 1;
            }
            object->size ++;

            // Check for comma
            token = token->next;
            if(token->type == e_CPTT_Seperator)
            {
               seperator_satified = 1;
            }
            else
            {
               seperator_satified = 0;
            }

         }
         else
         {
            printf("Error: ConfigParser_ParseObject: Expected ','. Got \"%.*s\"\n", token->size, token->start);
         }
      }
   }
   else
   {
      printf("Error: ConfigParser_ParserObject: Expected '{'. Got: \"%.*s\"\n", token->size, token->start);
   }
}

static void ConfigParser_ParseArray(ConfigParser_T * parser, CPToken_T * token, CPValueArray_T * array)
{
   int done;
   size_t size;
   int seperator_satified;
   array->size = 0;
   size = GROW_BY;
   array->value_list = malloc(sizeof(CPValue_T *) * size);
   if(token->type == e_CPTT_ArrayBegin)
   {
      done = 0;
      seperator_satified = 1;
      while(done == 0)
      {
         token = token->next;

         if(token->type == e_CPTT_ArrayEnd)
         {
            done = 1;
         }
         else if(seperator_satified == 1)
         {
            // Grow if nessary
            if(array->size >= size)
            {
               size = size + GROW_BY;
               array->value_list = realloc(array->value_list, sizeof(CPValue_T *) * size);
            }

            array->value_list[array->size] = ConfigParser_ParseValue(parser, token);
            if(array->value_list[array->size] == NULL)
            {
               done = 1;
            }
            array->size ++;

            // Check for comma
            token = token->next;
            if(token->type == e_CPTT_Seperator)
            {
               seperator_satified = 1;
            }
            else
            {
               seperator_satified = 0;
            }

            
         }
         else
         {
            printf("Error: ConfigParser_ParseArray: Expected ','. Got \"%.*s\"\n", token->size, token->start);
         }

      }

   }
   else
   {
      printf("Error: ConfigParser_ParseArray: Expected first token to be \"[\". Instead got: \"%.*s\"\n", token->size, token->start);
   }
}

static CPValue_T * ConfigParser_ParseValue(ConfigParser_T * parser, CPToken_T * token)
{
   CPValue_T * result;
   result = NULL;
   if(token->type == e_CPTT_String)
   {
      result = malloc(sizeof(CPValue_T));
      result->type = e_CPVT_String;
      result->data.string.token = token;
   }
   else if(token->type == e_CPTT_ArrayBegin)
   {
      result = malloc(sizeof(CPValue_T));
      result->type = e_CPVT_Array;
      ConfigParser_ParseArray(parser, token, &result->data.array);
      // TODO: Pass into parse array;
   }
   else if(token->type == e_CPTT_StructBegin)
   {
      result = malloc(sizeof(CPValue_T));
      result->type = e_CPVT_Object;
      // TODO: Pass into Parse Object
   }
   else
   {
      printf("Error: ConfigParser_ParseValue: Recived Unexpteded Token \"%*.s\"\n", token->size, token->start);
   }

   return result;
}

static void ConfigParser_ParseTokens(ConfigParser_T * parser)
{
   parser->root = ConfigParser_ParseValue(parser, parser->token_root);
}

static void ConfigParser_ParseBuffer(ConfigParser_T * parser)
{
   CPToken_T * loop;
   ConfigParser_Tokenize(parser);
   ConfigParser_ParseTokens(parser);

   loop = parser->token_root;
   while(loop != NULL)
   {
      printf("S: \"%.*s\"\n", loop->size, loop->start);
      loop = loop->next;
   }
}

