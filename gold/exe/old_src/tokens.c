/* Tokens program
 *
 * retrieve tokens from 2 files indexing them via a token string plus an offset
 * then compare the values of the tokens within some epsilon difference
 * and return the difference and  < = >
 *
 * usage:
 *
 * tokens -f0 infile_0 -t0 target_token_0 -o0 offset_0
 *        -f1 infile_1 -t1 target_token_1 -o1 offset_1
 *        -e  epsilon -d (differnce) -c (compare)  -v (verbose) -q (quiet)
 *
 * for example: file fff contains "foo bar 123 456"
 *          and file ggg contains "goo 100 baz 200 300 400"
 * to retreive the "123" versus "300" by location run with an epsilon of 10:
 *
 *    ./tokens -f0 fff -t0 bar -o0 1  -f1 ggg -t1 baz -o1 2  -e 10  -q  -d -c
 *
 * the result would be  177 <
 *
 */

#include <stdio.h>
#include <stdlib.h>

main(int argc, char *argv[])
{
  
    int    vbose = 1;
    char  *file0 = "in0", *file1 = "in1";
    char  *targ0 = "",    *targ1 = "";
    int    offs0 = 0 ,     offs1  = 0;
    int    bdif  = 0;
    int    bval  = 0;
    int    bcmp  = 0;
    double epsi  = 0;

    int a = 0;
    for(a=0; a<argc; a++)
    {
        if(strcmp(argv[a], "-f0") == 0) file0 = argv[++a];
        if(strcmp(argv[a], "-f1") == 0) file1 = argv[++a];
        if(strcmp(argv[a], "-t0") == 0) targ0 = argv[++a];
        if(strcmp(argv[a], "-t1") == 0) targ1 = argv[++a];
        if(strcmp(argv[a], "-o0") == 0) offs0 = atoi(argv[++a]);
        if(strcmp(argv[a], "-o1") == 0) offs1 = atoi(argv[++a]);
        if(strcmp(argv[a], "-v" ) == 0) vbose = 1;
        if(strcmp(argv[a], "-q" ) == 0) vbose = 0;
        if(strcmp(argv[a], "-d" ) == 0) bdif  = 1;
        if(strcmp(argv[a], "-c" ) == 0) bcmp  = 1;
        if(strcmp(argv[a], "-v" ) == 0) bval  = 1;
        if(strcmp(argv[a], "-e" ) == 0) epsi  = atof(argv[a]);
    }

    if(vbose)
    {
        printf("Test file0=%s targ0=%s offs0=%d\n", file0, targ0, offs0);
        printf("     file1=%s targ1=%s offs1=%d\n", file1, targ1, offs1);
        printf("     epsi =%f\n");
    }


    FILE *fp0 = fopen(file0, "r");
    FILE *fp1 = fopen(file1, "r");
    if(vbose > 1)printf("fp0 = %d  fp1 = %d\n", fp0, fp1);
    if(fp0 == 0){ printf("ERROR fopen: %s\n", file0); return; }
//    if(fp1 == 0){ printf("ERROR fopen: %s\n", file1); return; }

    char tok0[100];
    int  got0 = 0;
    int  off0 = 0;
    int  idx0 = 0;

    for(idx0=0; idx0<10000; idx0++)
    {
        int r = fscanf(fp0, "%s", tok0);
        if(r == -1) return;
        if(argc > 1 && strcmp(tok0, targ0) == 0) got0 = 1;
        if(vbose)printf("%d: got0=%d off0=%d tok0 = \"%s\"\n", idx0, got0, off0, tok0);
        if(got0 && off0 == offs0) break;
        if(got0) off0++;
    }

    double tokd0 = atof(tok0);

    printf(" -->  retreived %dth tok0 = \"%s\" = %f\n", idx0, tok0, tokd0);

    char tok1[100];
    int  got1 = 0;
    int  off1 = 0;
    int  idx1 = 0;
/*
    for(idx1=0; idx1<10000; idx1++)
    {
        int r = fscanf(fp1, "%s", tok1);
        if(r == -1) return;
        if(argc > 1 && strcmp(tok1, targ1) == 0) got1 = 1;
        if(vbose)printf("%d: got1=%d off1=%d tok1 = \"%s\"\n", idx1, got1, off1, tok1);
        if(got1 && off1 == offs1) break;
        if(got1) off1++;
    }
*/

    double tokd1 = atof(tok1);

    printf(" -->  retreived %dth tok1 = \"%s\" = %f\n", idx1, tok1, tokd1);

    double difd  = tokd1 - tokd0;
    int    cmpd   = difd > epsi ? -1 : (difd < epsi ? 1 : 0);
    char   cmpc   = cmpd < 0 ? '<' : (cmpd > 0 ? '>' : '=');
    if(bdif) printf("  DIF = %f\n", difd);
    if(bval) printf("  VAL = %d\n", cmpd);
    if(bcmp) printf("  CMP = %c\n", cmpc); 
    
}

