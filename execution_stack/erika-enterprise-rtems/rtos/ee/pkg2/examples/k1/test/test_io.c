/** Application (I/O SIDE) **/

#include "offload_support.h"
#include <math.h>

/* USER-LEVEL CODE (with manual outline) */

#define SIZE 16

void omp_fn1(struct _omp_param *params)
{
  int *a = params[0].ptr;
  int *b = params[1].ptr;
  int *c = params[2].ptr;

  // START USER CODE
  int i, j, k;

  for (i = 0; i < SIZE*SIZE; ++i)
    c[i] = 0;

  for (j = 0; j < SIZE; ++j)
    for (k = 0; k < SIZE; ++k)
      for (i = 0; i < SIZE; ++i)
        c[j * SIZE + i] += a[k * SIZE + i] * b[j * SIZE + k];
  // END USER CODE
}

void omp_fn2(struct _omp_param *params)
{
  double *A = params[0].ptr;
  double *Anew = params[1].ptr;

  // START USER CODE
  int i,j;
  int iter_max = 100;
  double tol = 1.0e-6;
  double error = 1.0;
  int iter = 0;

  while(error > tol && iter < iter_max)  {
    error = 0.0;
    
    for(j = 1; j < SIZE-1; ++j) {
      for(i = 1; i < SIZE-1; ++i) {
        Anew[j*SIZE+i] = 0.25 * ( A[j*SIZE+i+1] + A[j*SIZE+i-1]
                            + A[(j-1)*SIZE+i] + A[(j+1)*SIZE+i]);
        error = fmax(error, fabs(Anew[j*SIZE+i] - A[j*SIZE+i]));
      }
    }
    
    for( j = 1; j < SIZE-1; j++) {
      for( i = 1; i < SIZE-1; i++ ) {
        A[j*SIZE+i] = Anew[j*SIZE+i];
      }
    }
    
    iter++;
  }
  // END USER CODE
}


/* DATA */ 

int a[SIZE*SIZE];
int b[SIZE*SIZE];
int c[SIZE*SIZE];

double A[SIZE*SIZE];
double Anew[SIZE*SIZE];


/* PROGRAM ENTRY FUNCTION */

int main (int argc, char** argv)
{
  int i;
  
  printf("STARTING... \n\n");

  GOMP_init(1 /* device id */);

  printf("MATRIX MULTIPLICATION\n");

  for (i = 0; i < SIZE*SIZE; ++i) {
    a[i] = 1, b[i] = 1;
  }


  struct _params3 data1;
  data1.n = 3;
  data1.params[0].ptr = a;
  data1.params[0].size = sizeof(int) * SIZE*SIZE;
  data1.params[0].type = 0; // IN
  data1.params[1].ptr = b;
  data1.params[1].size = sizeof(int) * SIZE*SIZE;
  data1.params[1].type = 0; // IN
  data1.params[2].ptr = c;
  data1.params[2].size = sizeof(int) * SIZE*SIZE;
  data1.params[2].type = 1; // OUT

  GOMP_target(1 /* device id */,
              (void (*) (void *)) omp_fn1 /* host function */,
              (void *) 0 /* Function executed on cluster side */,
              (char *) &data1 /* data struct*/);

  for(i=0; i<SIZE*SIZE; ++i)
    printf("c[%d] = %d\n", i, c[i]);


  printf("JACOBI RELAXATION FOR 2D LAPLACIAN\n");

  memset(A, 0, SIZE * SIZE * sizeof(double));
  memset(Anew, 0, SIZE * SIZE * sizeof(double));
  
  for (i = 0; i < SIZE; ++i)
  {
    A[i*SIZE]    = 1.0;
    Anew[i*SIZE] = 1.0;
  }
  

  struct _params2 data2;
  data2.n = 2;
  data2.params[0].ptr = A;
  data2.params[0].size = sizeof(double) * SIZE*SIZE;
  data2.params[0].type = 2; // INOUT
  data2.params[1].ptr = Anew;
  data2.params[1].size = sizeof(double) * SIZE*SIZE;
  data2.params[1].type = 0; // IN

  GOMP_target(1 /* device id */,
              (void (*) (void *)) omp_fn2 /* host function */,
              (void *) 1 /* Function executed on cluster side */,
              (char *) &data2 /* data struct*/);

  for(i=0; i<SIZE*SIZE; ++i)
    printf("A[%d] = %f\n", i, A[i]);

  GOMP_deinit(1 /* device id */);
  return 0;
}
