/* read a file and print tokens on it */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TOKEN_SIZE 2047
char token[MAX_TOKEN_SIZE+1];
char tokencp[MAX_TOKEN_SIZE+1];

#define ASIZE(a_) sizeof(a_)/sizeof(a_[0])
#define FATAL(s_) fatal(s_, __FILE__, __LINE__)

enum TokenType {
   IS_ERROR,
   IS_ID,
   IS_RESERVED,
   IS_OPERATOR,
   IS_STRING,
   IS_CHAR,
};

/* change this if adding single character operators */
#define MAX_ONECHAR_OPERATOR_POS 11  

char *operators[] = {
   "{", "}", "#", "<", ">", "(", ")", "=", // 0 .. 7
   "+", "-", "*", "/", // 8..11
   "/*", "*/", "&&", "||", "==", // 12..16
};

char *keywords[] = {
   "for", "if", "else", "while",
   "int", "char", "void", "long", "static", "const",
   "return", "case", "default",
};

static int operatorId = 0;

void fatal(const char *emsg, const char *file, int line)
{
   printf("%s at %s:%d\n", emsg, file, line);
   exit(1);
}

/* return 1 for blank, 2 for newlines */
int isblank(int c) 
{
   if (c == ' '  || c == '\t')
      return 1;
   if (c == '\n' || c == '\r')
      return 2;
   return 0;
}

int isoperator()
{
   int i;
   for (i=ASIZE(operators)-1; i>=0;  --i) {
      if (operators[i][0] == token[0] &&
          (operators[i][1]=='\0' || operators[i][1] == token[1])) {
         operatorId = i;
         return 1;
      }
   }
   return 0;
}
      
int isidchar(char c, int afterfirst)
{
   int normalchar = ('a' <= c && c <= 'z') || ('A'<=c && c<='Z') || c=='_';
   return normalchar || (afterfirst && (('0' <= c && c <= '9') || c=='.'));
}


void texformat(int maxpos)
{
   int c, i, n, pos;

   for (i=0; i<maxpos; ++i) tokencp[i] = token[i]; // make a copy of what we have

   n = maxpos;
   pos = 0;
   for (i=0; i<n && pos < MAX_TOKEN_SIZE; ++i) {
      c = tokencp[i];
      token[pos++] = c;
      /* convert into valid TeX characters */
      if (c=='$' || c=='{' || c=='}' || c=='%' || c=='_' || c=='#' || c=='&') {
         if (pos + 2 > MAX_TOKEN_SIZE) FATAL("error: token is too long");
         token[pos-1] = '\\';
         token[pos++] = c;
      }
      if (c == '\\') {
         int tlen = sizeof("\\textbackslash{}");
         if (tlen + pos > MAX_TOKEN_SIZE) FATAL("error: token is too long");
         strcpy(token + pos -1, "\\textbackslash{}");
         pos += tlen - 2;
      }
      if (c == '\'' || c == '"') {
         if (pos + 5 > MAX_TOKEN_SIZE) FATAL("error: token is too long");
         strcpy(token + pos -1, "{\\tt\"}");
         token[pos+3] = c;
         pos += sizeof("{\\tt\"}") - 2;
      }
      if (c == '<' || c == '>') {
         if (pos + 3 > MAX_TOKEN_SIZE) FATAL("error: token is too long");
         token[pos-1] = '$';
         token[pos  ] = c;
         token[pos+1] = '$';
         pos += 2;
      }
   }
   if (pos >= MAX_TOKEN_SIZE) FATAL("error: token is too long");
   token[pos] = '\0';
}

int readcharlit(FILE *f)
{
   int c = fgetc(f);
   if (c == '\\') {
      c = fgetc(f);
      if (c!='\\' && c!='r' && c!='n' && c!='0' && c!='\'' && c!='t' && c!='a' && c!='b') {
         printf("char is %c\n", c);
         FATAL("error: invalid character literal");
      }
      token[0] = '\\';
      token[1] = c;
      texformat(2);
   } else {
      token[0] = c;
      texformat(1);
   }
   c = fgetc(f);
   if (c != '\'')
      FATAL("error: invalid character literal");
   return IS_CHAR;
}

int readstringlit(FILE *f)
{
   int pos = 0;
   int c = fgetc(f);
   while (c != EOF && c != '"' && pos < MAX_TOKEN_SIZE) {
      if (c == '\\') {
         c = fgetc(f);
         if (c == '"') {
            token[pos++] = '\\';
         } else {
            ungetc(c, f);
            c = '\\';
         }
      }
      token[pos++] = c;
      c = fgetc(f);
   }
   if (pos >= MAX_TOKEN_SIZE) 
      FATAL("error: literal string is too long");
   if (c != '"') 
      FATAL("error: invalid string literal");
   texformat(pos);
   return IS_STRING;
}

int nexttoken(FILE *f) 
{
   int c, i;
   int pos = 0;
   int ret = 0;
   int isid = 0;
   int blanktype;

   /* write blanks */
   while ((c = fgetc(f)) != EOF && (blanktype = isblank(c))) {
      if (blanktype == 2) printf("\\\\\n\\rule{0cm}{0cm}");
      else printf("~");
   }

   /* read a token */
   while (c != EOF &&  !isblank(c) && pos < MAX_TOKEN_SIZE) {

      /* is an id? then only [_A-Za-z] allowed */
      if (pos == 0 && isidchar(c, pos)) {
         isid = 1;
      } else if (isid && pos > 0 && !isidchar(c, pos)) {
         ungetc(c, f);
         break;
      } 

      if (pos == 0 && c == '"') {
         return readstringlit(f);
      } 
      if (pos == 0 && c == '\'') {
         return readcharlit(f);
      }

      token[pos++] = c;

      /* is that an operator? return the appropriate code */
      if (pos == 2 && isoperator()) {
         ret = IS_OPERATOR;
         if (operatorId < MAX_ONECHAR_OPERATOR_POS) {
            ungetc(c, f);
            pos--;
         }
         break;
      }
      c = fgetc(f);
   }
   if (isblank(c)) ungetc(c, f); /* lets deal with this the next time */
   if (pos == 1) {
      token[pos] = '\0';
      if (isoperator()) return IS_OPERATOR; /* maybe returned before testing...*/
   }

   texformat(pos);

   if (ret) return ret;

   /* check for reserved keywords */
   for (i=0; i<sizeof(keywords)/sizeof(keywords[0]); ++i) {
      if (!strcmp(keywords[i], token)) {
         return IS_RESERVED;
      }
   }

   /* else it is just a common identifier */
   return IS_ID;
}

int main(int argc, char **argv) {
   FILE *f;
   int ret, opid;

   if (argc < 2) {
      printf("Usage: %s filename\n", argv[0]);
      return 0;
   }
   f = fopen(argv[1], "r");
   if (!f) {
      printf("error opening file %s\n", argv[1]);
      return 1;
   }

   printf("\\documentclass{article}\\input{pre}\\begin{document}\n");
   while (!feof(f) && (ret = nexttoken(f)) ) {
      if (ret == IS_OPERATOR) {
         char op[16];
         opid = operatorId;
         switch (opid) {
            case 0: strcpy(op, "\\{"); break;
            case 1: strcpy(op, "\\}"); break;
            case 2: strcpy(op, "{\\tt \\#}"); break;
            case 3: strcpy(op, "$<$"); break;
            case 4: strcpy(op, "$>$"); break;
            case 12: printf("{\\tt "); strcpy(op, "/*"); break;
            case 13: printf("}"); strcpy(op, "*/"); break;
            default: strcpy(op, token);
         }
         printf("{\\tt %s}", op);
      } else if (ret == IS_RESERVED) {
         printf("{\\bf %s}", token);
      } else if (ret == IS_STRING) {
         printf("{\\tt\"}%s{\\tt\"}", token);
      } else if (ret == IS_CHAR) {
         printf("{\\tt'%s'}", token);
      } else {
         printf("%s", token);
      } 
   }
   printf("\\end{document}\n");
   return 0;
}
