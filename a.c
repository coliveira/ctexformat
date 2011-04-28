/* read a file and print tokens on it */
#include <stdio.h>
#include <string.h>

#define MAX_TOKEN_SIZE 1023
char token[MAX_TOKEN_SIZE+1];

#define ASIZE(a_) sizeof(a_)/sizeof(a_[0])

enum {
   IS_ERROR,
   IS_ID,
   IS_RESERVED,
   IS_OPERATOR,
};

char *oneOperat[] = {
   "{", "}", "#", "<", ">", "(", ")",
   "=", ">", "<",
   "+", "-", "*", "/",
};

char *twoOperat[] = {
   "/*", "*/", "&&", "||", "==",
};

char *keywords[] = {
   "for",
   "if",
};

static int operatorId = 0;

int isblank(int c) 
{
   if (c == ' ' || c == '\t')
      return 1;
   if (c == '\n' || c == '\r') {
      printf("\\\\%c", c);
      return 1;
   }
   return 0;
}

int isOneOperator()
{
   int i;
   for (i=0; i<ASIZE(oneOperat); ++i) {
      if (token[0] == oneOperat[i][0]) {
         operatorId = i;
         return 1;
      }
   }
   return 0;
}

int isTwoOperator()
{
   int i;
   for (i=0; i<ASIZE(twoOperat); ++i) {
      if (token[0] == twoOperat[i][0] &&
          token[1] == twoOperat[i][1]) {
         operatorId = ASIZE(oneOperat) + i;
         return 1;
      }
   }
   return 0;
}

int nexttoken(FILE *f) 
{
   int c, i;
   int pos = 0;
   int ret = 0;
   while ((c = fgetc(f)) != EOF && isblank(c)) 
      ; /* advance over blanks */
   while (c != EOF &&  !isblank(c) && pos < MAX_TOKEN_SIZE) {
      token[pos++] = c;
      if (pos == 2) {
         if (isTwoOperator()) {
            ret = IS_OPERATOR;
            break;
         }
         else if (isOneOperator()) {
            ungetc(c, f);
            ret = IS_OPERATOR;
            pos--;
            break;
         }
      }
      if (c == '\\' || c == '_' || c == '#' || c == '&') {
         token[pos-1] = '\\';
         token[pos++] = c;
      }
      c = fgetc(f);
   }
   token[pos] = '\0';
   if (pos == 1 && isOneOperator()) return IS_OPERATOR;
   if (ret) return ret;

   for (i=0; i<sizeof(keywords)/sizeof(keywords[0]); ++i) {
      if (!strcmp(keywords[i], token)) {
         return IS_RESERVED;
      }
   }
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
         char op[8];
         opid = operatorId;
         switch (opid) {
            case 0: strcpy(op, "\\{"); break;
            case 1: strcpy(op, "\\}"); break;
            case 2: strcpy(op, "\\#"); break;
            case 3: strcpy(op, "$<$"); break;
            case 4: strcpy(op, "$>$"); break;
            case ASIZE(oneOperat): printf("{\\tt "); strcpy(op, "/*"); break;
            case ASIZE(oneOperat) + 1: printf("}"); strcpy(op, "*/"); break;
            default: strcpy(op, token);
         }
         printf("{\\it %s}", op);
      } else if (ret == IS_RESERVED) {
         printf("{\\bf %s}", token);
      } else {
         printf("%s ", token);
      }
   }
   printf("\\end{document}\n");
   return 0;
}
