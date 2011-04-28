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

// return 1 for blank, 2 for newlines
int isblank(int c) 
{
   if (c == ' ' || c == '\t')
      return 1;
   if (c == '\n' || c == '\r') {
      return 2;
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

int isidchar(char c, int afterfirst)
{
   int normalchar = ('a' <= c && c <= 'z') || ('A'<=c && c<='Z') || c=='_';
   return normalchar || (afterfirst && (('0' <= c && c <= '9') || c=='.'));
}

int nexttoken(FILE *f) 
{
   int c, i;
   int pos = 0;
   int ret = 0;
   int isid = 0;
   int blanktype;
   while ((c = fgetc(f)) != EOF && (blanktype = isblank(c))) {
      if (blanktype == 2) printf("\\\\\n\\rule{0cm}{0cm}");
      else printf("~");
   }
   while (c != EOF &&  !isblank(c) && pos < MAX_TOKEN_SIZE) {
      if (pos == 0 && isidchar(c, pos)) {
         isid = 1;
      } else if (isid && pos > 0 && !isidchar(c, pos)) {
         /* cannot be part of a token */
         ungetc(c, f);
/*       printf("I am ungetting %c for pos %d\n", c, pos); */
         break;
      }
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
      if (c == '\\' || c == '_' || (c == '#' && pos>1) ||  c == '&') {
         token[pos-1] = '\\';
         token[pos++] = c;
      }
      c = fgetc(f);
   }
   if (isblank(c)) ungetc(c, f); // lets deal with this the next time
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
         char op[16];
         opid = operatorId;
         switch (opid) {
            case 0: strcpy(op, "\\{"); break;
            case 1: strcpy(op, "\\}"); break;
            case 2: strcpy(op, "{\\tt \\#}"); break;
            case 3: strcpy(op, "$<$"); break;
            case 4: strcpy(op, "$>$"); break;
            case ASIZE(oneOperat): printf("{\\tt "); strcpy(op, "/*"); break;
            case ASIZE(oneOperat) + 1: printf("}"); strcpy(op, "*/"); break;
            default: strcpy(op, token);
         }
         printf("{\\tt %s}", op);
      } else if (ret == IS_RESERVED) {
         printf("{\\bf %s}", token);
      } else {
         printf("%s", token);
      }
   }
   printf("\\end{document}\n");
   return 0;
}
