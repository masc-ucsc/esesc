
#include <stdio.h>

main() {
 if(fork())
   printf("Hello child\n");
 printf("Hello both\n");
}
