#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" int foo() {
  printf("Hey Foo\n");
  return 0;
}