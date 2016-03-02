%{
#include <string.h>
#include <stdio.h>
#include <libgen.h>

#include "confparser.hpp"
	
int CFlineno=1;

#define MAXFILESTACK  32
typedef struct {
   char *filename;
   int line;
   YY_BUFFER_STATE state;
} fileStack_t;


fileStack_t  fileStack[MAXFILESTACK];
int fileStackPos=0;

const char *currentFile=0;
char *confDir=0;

%}

%option noyywrap
%option caseless
%option interactive
%option prefix="yyConf"

alpha   [a-zA-Z]
digit   [0-9]
alnum   [a-zA-Z0-9_]

Dnum1   {digit}+\.([eE][-+]?{digit}+)?
Dnum2   {digit}+\.?[eE][-+]?{digit}+
Dnum3   {digit}*\.{digit}+([eE][-+]?{digit}+)?
Double  {Dnum1}|{Dnum2}|{Dnum3}

Int     {digit}+

name    {alpha}({alnum})*

filename ({alnum}|[\./])*

nameref \$\({name}\)

strnameref \"\$\({name}\)\"

QString1 \"([^\$\"\n\r])*\"
QString2 \'([^\'\n\r])*\'

QString {QString1}|{QString2}

%%


 /***************** Double ***************/

{Double} { yyConflval.Double = atof(yyConftext); return CFDOUBLE; }

 /***************** Int ***************/

{Int}  { yyConflval.Int = strtol(yyConftext,0,10); return CFLONG; }

 /***************** Bool ***************/

"true"   { yyConflval.Bool = 1; return CFBOOL; }
"false"  { yyConflval.Bool = 0; return CFBOOL; }

 /***************** Misc ***************/

[\[]   return CFOB;  /* Open  Bracket */
[\]]   return CFCB;  /* Close Bracket */
[:][=] return CFEQUAL;  /* Assignment */
[=]    return CFEQUAL;  /* Assignment */
[:]    return CFDDOT;

[\(]   return CFOP;  /* Open   */
[\)]   return CFCP;  /* Close  */

[+]    return CFPLUS;
[-]    return CFMINUS;
[*]    return CFMULT;
[/]    return CFDIV;
[\\]   return CFDIVC;
[\^]   return CFEXP;

"<="   return LE;
"<"    return LT;
">="   return GE;
">"    return GT;

"?"    return TRICASE;



 /***************** Eat Comments and lines ***************/

\#[^\n\r]*

[\n\r] { CFlineno++; }

 /***************** CharPtr ***************/

{QString} { char *nstr = strdup(yyConftext+1); nstr[strlen(yyConftext)-2]=0; yyConflval.CharPtr = nstr; return CFQSTRING; }

{name}    { yyConflval.CharPtr = strdup(yyConftext); return CFNAME; }
	
{nameref} { char *nstr = strdup(yyConftext+2); nstr[strlen(yyConftext)-3]=0; yyConflval.CharPtr = nstr; return CFNAMEREF; }

{strnameref} { char *nstr = strdup(yyConftext+3); nstr[strlen(yyConftext)-5]=0; yyConflval.CharPtr = nstr; return CFSTRNAMEREF; }

[\t ]  /* Eat spaces */

[\<]{filename}[\>]  { 
                  FILE *nfp;
		  /* nextFile is the final name of the file to open */
		  char *nextFile;
		  /* Remove the starting < and ending > from the parser input */
		  char *inclFile  = strdup(yyConftext+1);
		  inclFile[strlen(inclFile)-1]='\0';
		  /* Is the parsed name a full (absolute) path */
		  if(*inclFile=='/'){
		    /* The name is a full path, just use it as file name */
		    nextFile=strdup(inclFile);
		  }else{
		    /* The name is a relative path assume that this is relative to the base conf file*/
		    /* The actual file name is a concatenation of that path, a slash, and the parsed name */
		    nextFile = (char *)malloc(strlen(confDir)+1+strlen(inclFile)+1);
		    strcpy(nextFile,confDir);
		    strcat(nextFile,"/");
		    strcat(nextFile,inclFile);
		  }
		  free(inclFile);
                  nfp=fopen(nextFile,"r");
                  if( nfp == 0 ){
	            fprintf(stderr,"Config:: Impossible to open the file [%s]\n",nextFile);
                    exit(-1);
                  }
                  fileStack[fileStackPos].line = CFlineno;
                  fileStack[fileStackPos].state= YY_CURRENT_BUFFER;
                  fileStack[fileStackPos].filename = (char*)currentFile;
                  fileStackPos++;

                  currentFile = nextFile;
                 
                  if( fileStackPos > MAXFILESTACK ){
                    fprintf(stderr,"Too many recursive includes configurations\n");
                    exit(-3);
                  }
                  
                  CFlineno =1;
                  yyConfin =nfp;

                  yy_switch_to_buffer(yy_create_buffer( yyConfin, YY_BUF_SIZE ) );
                 }

<<EOF>>    { 

            if( fileStackPos == 0 ){
              yyterminate();
            }else{ 
              fileStackPos--;
              
              yy_delete_buffer( YY_CURRENT_BUFFER );

              free((void *)currentFile);

              currentFile = fileStack[fileStackPos].filename;
              CFlineno = fileStack[fileStackPos].line;
              yy_switch_to_buffer(fileStack[fileStackPos].state);
            }
           }

.          { printf( "conflex::Line %d: Unrecognized character: [%s]\n", CFlineno,yyConftext ); }

%%
