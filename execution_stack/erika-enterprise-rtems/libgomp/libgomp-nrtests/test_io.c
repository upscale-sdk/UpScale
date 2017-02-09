/** Application (I/O SIDE) **/

#include "offload_support.h"
#include <math.h>

/* USER-LEVEL CODE (with manual outline) */



/* DATA */

#define SIZE 10

int a[SIZE*SIZE];
int b[SIZE*SIZE];
int c[SIZE*SIZE];

/* PROGRAM ENTRY FUNCTION */

int main (int argc, char** argv)
{
  int i;

  printf("STARTING... \n\n");

  GOMP_init(1 /* device id */);

  printf("NON-REGRESSION TESTS\n");

  struct _params2 data1;
  data1.n = 2;
  data1.params[0].ptr = a;
  data1.params[0].size = sizeof(int);
  data1.params[0].type = 0; // IN
  data1.params[1].ptr = b;
  data1.params[1].size = sizeof(int);
  data1.params[1].type = 1; // OUT

  GOMP_target(1 /* device id */,
              (void (*) (void *)) NULL /* host function */,
              (void *) 0 /* Function executed on cluster side */,
              (char *) &data1 /* data struct*/);


  GOMP_deinit(1 /* device id */);
  return 0;
}
