/* CmpTab program for comparing two csv spreadsheet files for CMPE202 project
 * Jim Herriot, version 2013.05.21
 *
 * compare 2 csv spreadsheet files
 *
 * usage:
 *
 * cmptab -f0 file0 -f1 file1
 *        -t title
 *        -m num_metrics
 *        -e epsi_0 epsi_1 .. epsi_n
 *        -c expected_cmp_0 expected_cmp_1 ... expected_cmp_n
 *        -v (verbose)
 *
 * for example:
 *
 * cmptab -f0 file0.in -f1 file1.in -o file.out -t SLOW -m 5 -e 10 0.1 4 4.5 20 -c > >= = <= <
 *
 * the results will be written into file.out
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

char *gettok(char *tok);

int main(int argc, char *argv[])
{
    char  *file0  = "tab0.csv";
    char  *file1  = "tab1.csv";
    char  *file2  = "tab2.csv";
    char  *title  = "";
    int    nmet   = 5;
    int    vbose = 0;
    int    Vbose = 0;

    ////////////////////////////
    /////////// args ///////////
    ////////////////////////////
    

    int a = 0;
    for(a=0; a<argc; a++)
    {
        if(strcmp(argv[a], "-f0") == 0) file0 = argv[++a];
        if(strcmp(argv[a], "-f1") == 0) file1 = argv[++a];
        if(strcmp(argv[a], "-o" ) == 0) file2 = argv[++a];
        if(strcmp(argv[a], "-t" ) == 0) title = argv[++a];
	      if(strcmp(argv[a], "-m" ) == 0) nmet = atoi(argv[++a]);        
        if(strcmp(argv[a], "-v" ) == 0) vbose++;
        if(strcmp(argv[a], "-V" ) == 0) Vbose++;
    }

    double epsi[nmet];
    a = 0;
    for(a=0; a<argc; a++)
        if(strcmp(argv[a], "-e") == 0)
           for(int i=0; i<nmet; i++)
              epsi[i] = atof(argv[++a]);

    char *cmps[nmet];
    for(int i=0; i<nmet; i++) cmps[i] = "=";
    a = 0;
    for(a=0; a<argc; a++)
        if(strcmp(argv[a], "-c") == 0)
           for(int i=0; i<nmet; i++)
              cmps[i] = argv[++a];

    for(int i=0; i<nmet; i++) 
    {
        char *s = cmps[i];
        if(strcmp(s, "=" ) == 0) s = "eq";
        if(strcmp(s, "<" ) == 0) s = "lt";
        if(strcmp(s, ">" ) == 0) s = "gt";
        if(strcmp(s, "<=") == 0) s = "le";
        if(strcmp(s, ">=") == 0) s = "ge";
        cmps[i] = s;
    }

    //for(int i=0; i<nmet; i++) printf(" epsi[%d] = %f ", i, epsi[i]); printf("\n");
    //for(int i=0; i<nmet; i++) printf(" cmps[%d] = \"%s\" ", i, cmps[i]); printf("\n");

    if(vbose)
    {
        printf("Cmptab file0=%s file1=%s file2=%s title=%s nmet=%ds\n",
                       file0, file1, file2, title, nmet);
        printf("epsi= "); for(int i=0; i<nmet; i++) printf("%f ",     epsi[i]); printf("\n");
        printf("cmps= "); for(int i=0; i<nmet; i++) printf("\"%s\" ", cmps[i]); printf("\n");
    }


    ////////////////////////////
    ////////// input ///////////
    ////////////////////////////


    FILE *fp0 = fopen(file0, "r");
    if(fp0 == 0){ printf("ERROR fopen: %s\n", file0); return -1; }

    FILE *fp1 = fopen(file1, "r");
    if(fp1 == 0){ printf("ERROR fopen: %s\n", file1); return -1; }

    FILE *fp2 = fopen(file2, "w");
    if(fp2 == 0){ printf("ERROR fopen: %s\n", file2); return -1; }

    char   tok [1000];
    char  *tab0[1000];
    char  *tab1[1000];
    char  *tab2[1000];
    double val0[1000];
    double val1[1000];
    double diff[1000];
    double dpos[1000];
    double prob[1000];
    char  *comm[1000];
    long int    idx0 = 0;
    long int    idx1 = 0;

    for(int i=0; i<100000; i++) // i<100000
    {
        int r = fscanf(fp0, "%[^' '] ", tok);  // change was made here
      //  printf("%s %d %d\n", tok,i,r); // added to see tok contents
        //   int r = fscanf(fp0, "%s", tok);  // dont use
        if(r == -1) break;    
        char *s = gettok(tok);
	      //printf("%d: tok = \"%s\" gettok = \"%s\"\n", i, tok, s); // dont use
        if(strlen(s) == 0) continue;
        tab0[idx0] = s;
        val0[idx0] = atof(s);
        if(Vbose)printf("%d: tok = \"%s\" tab0[%d] = \"%s\"\n", i, tok, idx0, tab0[idx0]);
        idx0++;
    }

    for(int i=0; i<100000; i++)
    {
        int r = fscanf(fp1, "%[^' '] ", tok);
 //       printf("%s %d %d\n", tok,i,r); // added to see tok contents
        //int r = fscanf(fp1, "%s", tok);
        if(r == -1) break;
        char *s = gettok(tok);
        //printf("%d: tok = \"%s\" gettok = \"%s\"\n", i, tok, s);
        if(strlen(s) == 0) continue;
        tab1[idx1] = s;
        val1[idx1] = atof(s);
        diff[idx1] = val0[idx1] - val1[idx1];
//        if(diff[idx1]!=0) printf(diff[idx1], "%e\n");  //added
//        printf("Diff[%ld]: %f \n",idx1,diff[idx1]);//added
        dpos[idx1] = abs(diff[idx1]); // made this statement a conditional and changed abs to fabs//HB
        
        if(Vbose)printf("%d: tok = \"%s\" tab1[%d] = \"%s\"\n", i, tok, idx1, tab1[idx1]);
        
        idx1++;
    }

    if(vbose) printf(" -->  read %d %d tokens\n", idx0, idx1);

    int k = 0;


    ////////////////////////////
    ////////// output //////////
    ////////////////////////////


    fprintf(fp2, "%s\n\n", title);

    k=0;
    fprintf(fp2, "Input table 0\n");

    for(int i=0; i<10000; i++)
    {
        for(int j=0; j<nmet+1; j++)
        {
           if(i==0 || j==0) fprintf(fp2, "%s,", tab0[k]);
           else             fprintf(fp2, "%f,", val0[k]);
           //fprintf(fp2, "%s , ", tab0[k]);
           k++;
           if(k >= idx0) break;
        }
        fprintf(fp2, "\n");
        if(k >= idx0) break;
    }


    k=0;
    fprintf(fp2, "\n\nInput table 1\n");

    for(int i=0; i<10000; i++)
    {
        for(int j=0; j<nmet+1; j++)
        {
           if(i==0 || j==0) fprintf(fp2, "%s,", tab1[k]);
           else             fprintf(fp2, "%f,", val1[k]);
           //fprintf(fp2, "%s , ", tab1[k]);
           k++;
           if(k >= idx1) break;
        }
        fprintf(fp2, "\n");
        if(k >= idx1) break;
    }

    k=0;
    fprintf(fp2, "\n\nDifferences\n");

    for(int i=0; i<10000; i++)
    {
        for(int j=0; j<nmet+1; j++)
        {
           if     (i==0 && j==0) fprintf(fp2, "DIFFS,");
           else if(i==0 || j==0) fprintf(fp2, "%s,", tab0[k]);
	   else                  fprintf(fp2, "%f,", diff[k]);
           k++;
           if(k >= idx1) break;
        }
        fprintf(fp2, "\n");
        if(k >= idx1) break;
    }

    k=0;
    fprintf(fp2, "\n\nErrors\n");

    for(int i=0; i<10000; i++)
    {
        for(int j=0; j<nmet+1; j++)
        {
           if(i> 0 && j > 0)
           {
              double dif = diff[k];
              double pos = fabs(dif);
              double eps = epsi[j-1];
              char  *cmp = cmps[j-1];
              int    equ = pos <= eps + 0.00001;
              int    ok  = 0;
              if(strcmp(cmp, "eq") == 0 &&  equ            ) ok = 1;
              if(strcmp(cmp, "le") == 0 && (equ || dif < 0)) ok = 1;
              if(strcmp(cmp, "ge") == 0 && (equ || dif > 0)) ok = 1;
              if(strcmp(cmp, "lt") == 0 &&         dif < 0 ) ok = 1;
              if(strcmp(cmp, "gt") == 0 &&         dif > 0 ) ok = 1;
              comm[k] = ok ? "ok" : "ERROR";
             
              if(Vbose)
                 printf("i=%d j=%d k=%d tab0=%s tab1=%s dif=%f pos=%f eps=%f cmp=%s equ=%d ok=%d comm=%s\n",
                         i,   j,   k,   tab0[k], tab1[k], dif, pos,   eps,   cmp,   equ,   ok,   comm[k]);
           }

           if     (i==0 && j==0) fprintf(fp2, "CHECK,");
           else if(i==0 || j==0) fprintf(fp2, "%s,", tab0[k]);
	   else                  fprintf(fp2, "%s,", comm[k]);
           k++;
           if(k >= idx1) break;
        }
        fprintf(fp2, "\n");
        if(k >= idx1) break;
    }

    fprintf(fp2, "\n\nParameters\n");
    fprintf(fp2, "EPSI,");
    for(int i=0; i<nmet; i++) fprintf(fp2, "%f,", epsi[i]);
    fprintf(fp2, "\n");
    fprintf(fp2, "COMP,");
    for(int i=0; i<nmet; i++) fprintf(fp2, "%s,", cmps[i]);
    fprintf(fp2, "\n");

    fclose(fp2);
}


char *gettok(char *tok)
{
   int  z = strlen(tok);
   int  k = 0;
   char a[z];

   for(int i=0; i<z; i++)
   {
      char c = tok[i];
      if(c == '\0') break;
      if(c == ',' ) continue;
      if(c == ' ' ) continue;
      if(c == '\n') continue;
      if(c == '\t') continue;
      a[k++] = c;
   }
   char *s = malloc(k+1);
   strncpy(s, a, k);
   return s;
}

