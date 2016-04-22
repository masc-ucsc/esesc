%{
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#ifdef CYGWIN
// This assumes that the build directory is located is a subdirectory of esesc
#include <../../src/libsuc/Config.h>
#else
#include "Config.h"
#endif
#include <string>
  using std::string;

  extern int CFlineno; /* Defined in conflex.l */

  string blockName("");
  //  const char *blockName=""; /* No name by default */

  Config *configptr;
  bool errorFound = false;

  /* To avoid warnings at compile time */
  int  yyConflex();
  int  yyConfparse();
  void yyConferror(const char *txt);

%}

%union {
  bool Bool;
  int  Int;
  double Double;
  char *CharPtr;
}

%type <Bool>    CFBOOL
%type <Int>     CFLONG lexp
%type <Double>  CFDOUBLE dexp
%type <CharPtr> CFQSTRING CFNAME CFNAMEREF CFSTRNAMEREF texp

%token CFDOUBLE
%token CFLONG
%token CFBOOL
%token CFQSTRING
%token CFNAME
%token CFNAMEREF
%token CFSTRNAMEREF

%token CFOB
%token CFCB
%token CFEQUAL
%token CFDDOT
%token CFOP
%token CFCP
%token CFPLUS
%token CFMINUS
%token CFMULT
%token CFDIV
%token CFDIVC
%token CFEXP
%token LE
%token LT
%token GE
%token GT
%token TRICASE

%left CFMINUS CFPLUS
%left CFMULT CFDIV CFDIVC
%left LE LT GE GT
%right CFEXP    /* exponentiation        */

%%

initial: /* empty */
         | initial rules
        ;


rules:
CFOB CFNAME CFCB  { blockName=$2; free($2); }
|
 CFNAME CFEQUAL texp    { configptr->addRecord(blockName.c_str(),$1,$3); free($1); free($3); }
|
 CFNAME CFEQUAL CFQSTRING { configptr->addRecord(blockName.c_str(),$1,$3); free($1); free($3); }
|
 CFNAME CFEQUAL CFNAME    { configptr->addRecord(blockName.c_str(),$1,$3); free($1); free($3); }
|
 CFNAME CFEQUAL CFBOOL    { configptr->addRecord(blockName.c_str(),$1,$3); free($1); }
|
 CFNAME CFEQUAL CFNAMEREF { 
  if (configptr->checkInt("",$3)) {
    int val = configptr->getInt("",$3);
    configptr->addRecord(blockName.c_str(),$1,val);
  }else if (configptr->checkBool("",$3)) {
    bool val = configptr->getBool("",$3);
    configptr->addRecord(blockName.c_str(),$1,val);
  }else if (configptr->checkCharPtr("",$3)) {
    const char *val = configptr->getCharPtr("",$3);
    configptr->addRecord(blockName.c_str(),$1,val);
  }else if (configptr->checkDouble("",$3)) {
    double val = configptr->getDouble("",$3);
    configptr->addRecord(blockName.c_str(),$1,val);
  }else{
    yyConferror($1);
  }
  free($3);
}
|
 CFNAME CFEQUAL lexp      { configptr->addRecord(blockName.c_str(),$1,$3); free($1); }
|
 CFNAME CFEQUAL dexp      { configptr->addRecord(blockName.c_str(),$1,$3); free($1); }
|
 CFNAME CFOB lexp CFCB CFEQUAL CFQSTRING { configptr->addVRecord(blockName.c_str(),$1,$6,$3,$3); free($1); free($6); }
|
 CFNAME CFOB lexp CFCB CFEQUAL lexp      { configptr->addVRecord(blockName.c_str(),$1,$6,$3,$3); free($1); }
|
 CFNAME CFOB lexp CFCB CFEQUAL dexp      { configptr->addVRecord(blockName.c_str(),$1,$6,$3,$3); free($1); }
|
 CFNAME CFOB lexp CFCB CFEQUAL CFBOOL    { configptr->addVRecord(blockName.c_str(),$1,$6,$3,$3); free($1); }
|
 CFNAME CFOB lexp CFCB CFEQUAL texp      { configptr->addVRecord(blockName.c_str(),$1,$6,$3,$3); free($1); }
|
 CFNAME CFOB lexp CFDDOT lexp CFCB CFEQUAL CFQSTRING { configptr->addVRecord(blockName.c_str(),$1,$8,$3,$5); free($1); free($8); }
|
 CFNAME CFOB lexp CFDDOT lexp CFCB CFEQUAL lexp  { configptr->addVRecord(blockName.c_str(),$1,$8,$3,$5); free($1); }
|
 CFNAME CFOB lexp CFDDOT lexp CFCB CFEQUAL dexp  { configptr->addVRecord(blockName.c_str(),$1,$8,$3,$5); free($1); }
|
CFNAME CFOB lexp CFDDOT lexp CFCB CFEQUAL CFBOOL { configptr->addVRecord(blockName.c_str(),$1,$8,$3,$5); free($1); }
|
CFNAME CFOB lexp CFDDOT lexp CFCB CFEQUAL texp   { configptr->addVRecord(blockName.c_str(),$1,$8,$3,$5); free($1); }
;


texp:  CFSTRNAMEREF            { $$ = strdup(configptr->getCharPtr("",$1)); configptr->isCharPtr("",$1); free($1); }
;


lexp:     CFLONG               { $$ = $1; }
        | CFNAMEREF            { 
  if (configptr->isInt("",$1)) {
    $$ = configptr->getInt("",$1);
  }else{
    yyConferror($1);
  }
  free($1);
}
        | lexp CFPLUS  lexp    { $$ = $1 + $3;    }
        | lexp CFMINUS lexp    { $$ = $1 - $3;    }
        | lexp CFMULT  lexp    { $$ = $1 * $3;    }
        | lexp CFDIV   lexp    { $$ = $1 / $3;    }
        | lexp CFDIVC  lexp    { $$ = (int)ceil((double)$1 / (double)$3); }
        | lexp CFEXP   lexp    { $$ = static_cast<int>(pow((double)$1, (double)$3)); }
        | CFOP lexp CFCP       { $$ = $2; }
        | CFMINUS lexp  %prec CFMINUS { $$ = -$2; }
        | lexp LE lexp TRICASE lexp CFDDOT lexp { $1<=$3 ? $$=$5 : $$=$7; }
        | lexp LT lexp TRICASE lexp CFDDOT lexp { $1< $3 ? $$=$5 : $$=$7; }
        | lexp GE lexp TRICASE lexp CFDDOT lexp { $1>=$3 ? $$=$5 : $$=$7; }
        | lexp GT lexp TRICASE lexp CFDDOT lexp { $1> $3 ? $$=$5 : $$=$7; }
;

dexp:      CFDOUBLE        { $$ = $1; }
/*        | CFNAMEREF              { $$ = configptr->getInt("",$1); configptr->isInt("",$1); } */
        | dexp CFPLUS dexp       { $$ = $1 + $3; }
        | lexp CFPLUS dexp       { $$ = $1   + $3; }
        | dexp CFPLUS lexp       { $$ = $1 + $3;   }

        | dexp CFMINUS dexp      { $$ = $1 - $3; }
        | lexp CFMINUS dexp      { $$ = $1   - $3; }
        | dexp CFMINUS lexp      { $$ = $1 - $3;   }

        | dexp CFMULT dexp       { $$ = $1 * $3; }
        | lexp CFMULT dexp       { $$ = $1   * $3; }
        | dexp CFMULT lexp       { $$ = $1 * $3;   }

        | dexp CFDIV dexp        { $$ = $1 / $3; }
        | lexp CFDIV dexp        { $$ = $1   / $3; }
        | dexp CFDIV lexp        { $$ = $1 / $3;   }

        | dexp CFEXP dexp        { $$ = pow ($1, $3); }
        | lexp CFEXP dexp        { $$ = pow ((double)$1, $3); }
        | dexp CFEXP lexp        { $$ = pow ($1, (double)$3);   }

        | CFOP dexp CFCP         { $$ = $2; }

        | CFMINUS dexp  %prec CFMINUS { $$ = -$2;  }
;


%%

/* Used by flex "conflex.l" */
extern FILE *yyConfin, *yyConfout;
extern const char *currentFile;
extern char *confDir;

void yyConferror(const char *txt) {

  fprintf(stderr,"conflex:<%s> Line %d error:%s\n",currentFile,CFlineno,txt);
  errorFound=true;
}

bool readConfigFile(Config *ptr, FILE *fp, const char *fpname) {

  yyConfin = fp;
  
  configptr  = ptr;
  errorFound = false;
  
  currentFile = fpname;
  char *confFile = strdup(currentFile);
  confDir = strdup(dirname(confFile));
  free(confFile);
  
  yyConfparse();
 
  free(confDir);
  return !errorFound;
}
