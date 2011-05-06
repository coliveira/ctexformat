/*
 * ctexformat: reads a C file and print it formatted as LaTeX input.
 * 2011 (c) Carlos A.S. Oliveira  http://coliveira.net 
 * - released under the GPL license.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TOKEN_SIZE 2047
char token[MAX_TOKEN_SIZE+1];
char tokencp[MAX_TOKEN_SIZE+1];

#define ASIZE(a_) sizeof(a_)/sizeof(a_[0])
#define FATAL(s_) fatal(s_, __FILE__, __LINE__)

void fatal(const char *emsg, const char *file, int line);

enum TokenType {
   IS_ERROR,
   IS_ID,
   IS_RESERVED,
   IS_OPERATOR,
   IS_STRING,
   IS_CHAR,
   IS_INT,
   IS_FLOAT,
   IS_NOTHING, // when EOF found
};

/* change this if adding single character operators */
#define MAX_ONECHAR_OPERATOR_POS 17

char *operators[] = {
   "{", "}", "#", "<", ">", "(", ")", "=", // 0 .. 7
   "+", "-", "*", "/", ",", ";", ":", "[", "]", "!",  // 8..17
   "/*", "*/", "&&", "||", "==", "!=", // 18..23
};

char *keywords[] = {
   "for", "if", "else", "while",
   "int", "char", "void", "long", "static", "const",
   "return", "case", "default", "switch", "break",
   "struct", "enum", "sizeof",
   0,
};

#define MAX_ID_SIZE 63
#define MAX_NUM_IDS 1024

char identifiers[MAX_NUM_IDS][MAX_ID_SIZE+1];
int firstviewed[MAX_NUM_IDS];
int idpos[MAX_NUM_IDS];

int nids = 0;
int curline = 0;

int isidpresent(const char *id, int add)
{
   int i;
   for (i=0; i<nids; ++i) {
      if (!strcmp(id, identifiers[i])) 
         return 1;
   }
   if (!add) return 0;
   if (nids >= MAX_NUM_IDS)
      FATAL("error: no space for new ids");
   if (strlen(id)> MAX_ID_SIZE)
      FATAL("errr: ID is too long");
   firstviewed[nids] = curline;
   strcpy(identifiers[nids++], id);
   return 1;
}

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
      if (c == '|') {
         int tlen = sizeof("{\\tt|}");
         if (tlen + pos > MAX_TOKEN_SIZE) FATAL("error: token is too long");
         strcpy(token + pos -1, "{\\tt|}");
         pos += tlen - 2;
      }
      if (c == '\'' || c == '"') {
         if (pos + 5 > MAX_TOKEN_SIZE) FATAL("error: token is too long");
         strcpy(token + pos -1, "{\\tt\"}");
         token[pos+3] = c;
         pos += sizeof("{\\tt\"}") - 2;
      }
      if (c == '~') {
         int tlen = sizeof("{\\tt\\~{}}");
         if (pos + tlen > MAX_TOKEN_SIZE) FATAL("error: token is too long");
         strcpy(token + pos -1, "{\\tt\\~{}}");
         pos += tlen - 2;
      }
      if (c == '+' || c == '-' || c == '<' || c == '>') {
         if (pos + 3 > MAX_TOKEN_SIZE) FATAL("error: token is too long");
         token[pos-1] = '$';
         token[pos  ] = c;
         token[pos+1] = '$';
         pos += 2;
      }
   }
   if (pos >= MAX_TOKEN_SIZE)  FATAL("error: token is too long"); 
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

int readnumber(FILE *f)
{
   int pos = 0;
   int c = fgetc(f);
   int type = IS_INT;
   while ('0' <= c && c <= '9') {
      token[pos++] = c;
      c = fgetc(f);
   }

   if (c == '.') {  // this is a float
      token[pos++] = c;
      c = fgetc(f);
      while ('0' <= c && c <= '9') {
         token[pos++] = c;
         c = fgetc(f);
      }
      type = IS_FLOAT;
   } 
   
   if (pos == 0)  // there is no number here
      FATAL("error reading number");

   token[pos] = '\0';
   ungetc(c, f);
   return type;
}

int readcomment(FILE *f)
{
   int blanktype;
   int c = fgetc(f);
   if (c == '/') { // this is a line comment
      printf("{\\tt//");
      while ((c = fgetc(f)) != EOF && c != '\n') {
         // just print the comment
         token[0] = c; texformat(1); printf("%s", token);
      }
      printf("}");
   } else if (c == '*') {
      /* There was a problem with alignment of first commented
       * line with the next. The reason is that before the comment,
       * blank characters "~" have the width of normal text. In the
       * comment, they have the width of \tt text. This is solved
       * by checking if there is a nonblank character. */
      printf("{\\tt/*");
      int nonblank = 1;
      while ((c = fgetc(f)) != EOF) {
         if (c == '*') {
            c = fgetc(f);
            if (c == '/') break;  /* end of comment */
            else { ungetc(c, f); c = '*'; }
         }
         if ((blanktype = isblank(c)) == 2) {
            printf("}\\\\\n\\rule{0cm}{0cm}"); curline++; nonblank = 0;
         } 
         else if (blanktype) printf("~");
         else {
            if (!nonblank) { printf("{\\tt "); nonblank=1; }
            token[0] = c; texformat(1); printf("%s", token);
         }
      }
      if (!nonblank) { printf("{\\tt"); }
      printf("*/}"); c = fgetc(f);
   } else {
      ungetc(c, f); c = '/';
   }
   return c;
}

int processblanks(FILE *f)
{
   int c, blanktype;
   while ((c = fgetc(f)) != EOF && (blanktype = isblank(c))) {
      if (blanktype == 2) { printf("\\\\\n\\rule{0cm}{0cm}"); curline++; }
      else printf("~");
   }
   return c;
}

int nexttoken(FILE *f) 
{
   int pos = 0;
   int ret = 0;
   int isid = 0;
   int c;
   char **kws;

   c = processblanks(f);
   if (c == EOF) return IS_NOTHING;

   if (c == '/') {
      c = readcomment(f);
      ungetc(c, f);
      c = processblanks(f);
   }
   if (c == '"') {
      return readstringlit(f);
   } 
   if (c == '\'') {
      return readcharlit(f);
   }
   if (('0' <= c && c <= '9') || c == '.') {
      ungetc(c, f);
      return readnumber(f);
   }
   if (isidchar(c, pos)) {
      isid = 1;
   }

   /* read a general token */
   while (c != EOF &&  !isblank(c) && pos < MAX_TOKEN_SIZE) {

      /* is an id? then only [_A-Za-z] allowed */
      if (isid && pos > 0 && !isidchar(c, pos)) {
         ungetc(c, f);
         break;
      } 

      token[pos++] = c;

      /* is that an operator? return the appropriate code */
      if (pos == 2 && isoperator()) {
         ret = IS_OPERATOR;
         if (operatorId <= MAX_ONECHAR_OPERATOR_POS) {
            ungetc(c, f);
            pos--;
         }
         break;
      }
      c = fgetc(f);
   }
   token[pos] = '\0';

   if (isblank(c)) 
      ungetc(c, f); /* lets deal with this the next time */

   if (pos == 1 && isoperator()) 
      return IS_OPERATOR; /* maybe returned before testing...*/


   if (ret) {
      texformat(pos);
      return ret;
   }

   /* check for reserved keywords */
   for (kws = keywords; *kws; ++kws) {
      if (!strcmp(*kws, token)) {
         return IS_RESERVED;
      }
   }

   /* else it is just a common identifier */
   if (pos == 0) return IS_NOTHING;
   isidpresent(token, 1);
   texformat(pos);
   return IS_ID;
}

int cmpfunc(const void *pa, const void *pb)
{
   int a = *((int*)pa);
   int b = *((int*)pb);
   return strcasecmp(identifiers[a], identifiers[b]);
}

void sortids()
{
   int i;
   for (i=0; i<nids; ++i) idpos[i] = i;
   qsort(idpos, nids, sizeof(idpos[0]), cmpfunc);
}

int main(int argc, char **argv) 
{
   int i;
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
   while (!feof(f) && (ret = nexttoken(f)) != IS_NOTHING ) {
      if (ret == IS_OPERATOR) {
         char op[16];
         opid = operatorId;
         switch (opid) {
            case 0: strcpy(op, "\\{"); break;
            case 1: strcpy(op, "\\}"); break;
            case 2: strcpy(op, "{\\tt \\#}"); break;
            case 3: strcpy(op, "$<$"); break;
            case 4: strcpy(op, "$>$"); break;
            case MAX_ONECHAR_OPERATOR_POS + 1: printf("{\\tt "); strcpy(op, "/*"); break;
            case MAX_ONECHAR_OPERATOR_POS + 2: printf("}"); strcpy(op, "*/"); break;
            default: strcpy(op, token);
         }
         printf("{\\tt %s}", op);
      } else if (ret == IS_RESERVED) {
         printf("{\\bf %s}", token);
      } else if (ret == IS_STRING) {
         printf("{\\tt\"}%s{\\tt\"}", token);
      } else if (ret == IS_CHAR) {
         printf("{\\tt'%s'}", token);
      } else if (ret == IS_INT || ret == IS_FLOAT) {
         printf("{\\tt %s}", token);
      } else {
         printf("%s", token);
      } 
   }
   printf("\\section*{Index}\n");
   sortids();
   for (i=0; i<nids; ++i) {
      strcpy(token, identifiers[idpos[i]]);
      texformat(strlen(token));
      printf("%s, line %d\\\\\n", token, firstviewed[idpos[i]]);
   }
   printf("\\end{document}\n");
   return 0;
}
