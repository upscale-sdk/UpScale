/* Copyright 2014 DEI - Universita' di Bologna
   author       DEI - Universita' di Bologna
                Alessandro Capotondi - alessandro.capotondi@unibo.it
                Andrea Marongiu - a.marongiu@unibo.it
                Davide Rossi - davide.rossi@unibo.it
   info         NonRegression test Libgomp */
//NOTE[ALE] All the tests work fine ;)
#define VERBOSE

#define __nop { \
  asm("l.nop"); \
}
#define _10_nop_block  { \
  asm("l.nop"); \
  asm("l.nop"); \
  asm("l.nop"); \
  asm("l.nop"); \
  asm("l.nop"); \
  asm("l.nop"); \
  asm("l.nop"); \
  asm("l.nop"); \
  asm("l.nop"); \
  asm("l.nop"); \
  }
#define _50_nop_block  { \
  _10_nop_block \
  _10_nop_block \
  _10_nop_block \
  _10_nop_block \
  _10_nop_block \
  }
#define _100_nop_block  { \
  _50_nop_block \
  _50_nop_block \
  }
#define _200_nop_block  { \
  _100_nop_block \
  _100_nop_block \
  }
#define LOOP_ITER 32 
#define NB_STRESS 2
//#define VERBOSE
#define ANSI_COLOR_BLUE
#define ANSI_COLOR_RED
#define ANSI_COLOR_GREEN
#define ANSI_COLOR_RESET
#define SINGLE_PARALLEL_TESTS
#define SINGLE_PARALLEL_SECTIONS_TESTS
#define SINGLE_PARALLEL_FOR_TESTS
#define PARALLEL_LOOP_DYNAMIC_TESTS
#ifndef __OMP_NEW_RT__
#define NESTED_PARALLEL_TESTS
#define NESTED_PARALLEL_FOR_TESTS
#define SECTIONS_LOOP_NESTED_PARALLEL_FOR_TESTS
#define SECTION_NESTED_PARALLEL
#endif
#include "libgomp.h" 

int check_parallel(unsigned int nthreads)
{
  int ret = 0;
  switch(nthreads){
    case 2:
{
      ret = 3;
      break; 
    }
    case 17:
{
      ret = 153;
      break; 
    }
    case 33:
{
      ret = 561;
      break; 
    }
    case 49:
{
      ret = 1225;
      break; 
    }
    case 16:
{
      ret = 136;
      break; 
    }
    case 32:
{
      ret = 528;
      break; 
    }
    case 64:
{
      ret = 2080;
      break; 
    }
    case 256:
{
      ret = 32896;
      break; 
    }
    case 128:
{
      ret = 8256;
      break; 
    }
    default:
{
      while(nthreads > 0)
        ret += nthreads--;
      break; 
    }
  }
  return ret;
}

int check_nested_parallel(unsigned int nthreads1,unsigned int nthreads2)
{
  int ret = 0;
  int i;
  int j;
  int iterations;
  if ((nthreads1 == 4) && (nthreads2 == 4)) 
    ret = 136;
  else if ((nthreads1 == 4) && (nthreads2 == 16)) 
    ret = 2080;
  else {
    iterations = (nthreads1 * nthreads2);
    while(iterations > 0){
      ret += iterations;
      iterations--;
    }
  }
  return ret;
}
#ifdef SINGLE_PARALLEL_TESTS

struct OUT__14__3827___data 
{
  void *cnt_p;
}
;
static void OUT__14__3827__(void *__out_argv);

int test_single_parreg(unsigned int nthreads)
{
  int cnt = 0;
  int check = 0;
  int ret = 0;
  int i = 0;
  struct OUT__14__3827___data __out_argv14__3827__;
  __out_argv14__3827__.cnt_p = ((void *)(&cnt));
  GOMP_parallel_start(OUT__14__3827__,&__out_argv14__3827__,nthreads);
  OUT__14__3827__(&__out_argv14__3827__);
  GOMP_parallel_end();
  check = check_parallel(nthreads);
#ifdef VERBOSE      
#endif  
  if (cnt != check) {
    printf("[test_single_parreg] [CHECK] [ERROR]\n");
    ret = -1;
  }
  else {
    printf("[test_single_parreg] [CHECK] [CORRECT]\n");
    ret = 0;
  }
  return ret;
}
#endif
#ifdef NESTED_PARALLEL_TESTS

struct OUT__12__3827___data 
{
  void *tmp_cnt_p;
}
;
static void OUT__12__3827__(void *__out_argv);

struct OUT__13__3827___data 
{
  void *nthreads_2_p;
  void *cnt_p;
}
;
static void OUT__13__3827__(void *__out_argv);

int test_nested_parreg(unsigned int nthreads_1,unsigned int nthreads_2)
{
  int cnt = 0;
  int check = 0;
  int ret = 0;
  struct OUT__13__3827___data __out_argv13__3827__;
  __out_argv13__3827__.cnt_p = ((void *)(&cnt));
  __out_argv13__3827__.nthreads_2_p = ((void *)(&nthreads_2));
  GOMP_parallel_start(OUT__13__3827__,&__out_argv13__3827__,nthreads_1);
  OUT__13__3827__(&__out_argv13__3827__);
  GOMP_parallel_end();
  check = check_nested_parallel(nthreads_1,nthreads_2);
#ifdef VERBOSE      
#endif
  if (cnt != check) {
    printf("[test_nested_parreg] [CHECK] [ERROR]\n");
    ret = -1;
  }
  else {
    printf("[test_nested_parreg] [CHECK] [CORRECT]\n");
    ret = 0;
  }
  return ret;
}
#endif
#ifdef SINGLE_PARALLEL_SECTIONS_TESTS

struct OUT__11__3827___data 
{
  void *iter_p;
  void *cnt_p;
}
;
static void OUT__11__3827__(void *__out_argv);

int test_sections_single_parreg(unsigned int nthreads,unsigned int iter)
{
  int cnt = 0;
  int check = 0;
  int ret = 0;
  struct OUT__11__3827___data __out_argv11__3827__;
  __out_argv11__3827__.cnt_p = ((void *)(&cnt));
  __out_argv11__3827__.iter_p = ((void *)(&iter));
  GOMP_parallel_start(OUT__11__3827__,&__out_argv11__3827__,nthreads);
  OUT__11__3827__(&__out_argv11__3827__);
  GOMP_parallel_end();
  check = check_parallel((4 * iter));
#ifdef VERBOSE      
#endif
  if (cnt != check) {
    printf("[test_sections_single_parreg] [CHECK] [ERROR]\n");
    ret = -1;
  }
  else {
    printf("[test_sections_single_parreg] [CHECK] [CORRECT]\n");
    ret = 0;
  }
  return ret;
}
#endif
#ifdef SECTION_NESTED_PARALLEL

struct OUT__9__3827___data 
{
  void *reg_id_p;
  void *tmp_cnt_p;
}
;
static void OUT__9__3827__(void *__out_argv);

struct OUT__10__3827___data 
{
  void *nthreads2_p;
  void *cnt_p;
}
;
static void OUT__10__3827__(void *__out_argv);

int test_sections_nested_parreg(unsigned int nthreads1,unsigned int nthreads2)
{
  int cnt = 0;
  int check = 0;
  int ret = 0;
  struct OUT__10__3827___data __out_argv10__3827__;
  __out_argv10__3827__.cnt_p = ((void *)(&cnt));
  __out_argv10__3827__.nthreads2_p = ((void *)(&nthreads2));
  GOMP_parallel_start(OUT__10__3827__,&__out_argv10__3827__,nthreads1);
  OUT__10__3827__(&__out_argv10__3827__);
  GOMP_parallel_end();
  check = check_parallel((4 * nthreads1));
#ifdef VERBOSE      
#endif
  if (cnt != check) {
    printf("[test_sections_nested_parreg] [CHECK] [ERROR]\n");
    ret = -1;
  }
  else {
    printf("[test_sections_nested_parreg] [CHECK] [CORRECT]\n");
    ret = 0;
  }
  return ret;
}
#endif
#ifdef SINGLE_PARALLEL_FOR_TESTS

struct OUT__8__3827___data 
{
  void *iter_p;
  void *cnt_p;
}
;
static void OUT__8__3827__(void *__out_argv);

int test_loop_single_parreg(unsigned int nthreads,unsigned int iter)
{
  int cnt = 0;
  int check = 0;
  int ret = 0;
  struct OUT__8__3827___data __out_argv8__3827__;
  __out_argv8__3827__.cnt_p = ((void *)(&cnt));
  __out_argv8__3827__.iter_p = ((void *)(&iter));
  GOMP_parallel_start(OUT__8__3827__,&__out_argv8__3827__,nthreads);
  OUT__8__3827__(&__out_argv8__3827__);
  GOMP_parallel_end();
  check = check_parallel((iter * 32));
#ifdef VERBOSE      
#endif
  if (cnt != check) {
    printf("[test_loop_single_parreg] [CHECK] [ERROR]\n");
    ret = -1;
  }
  else {
    printf("[test_loop_single_parreg] [CHECK] [CORRECT]\n");
    ret = 0;
  }
  return ret;
}
#endif
#ifdef NESTED_PARALLEL_FOR_TESTS

struct OUT__6__3827___data 
{
  void *iter1_p;
  void *iter2_p;
  void *i_p;
  void *tmp_cnt_p;
}
;
static void OUT__6__3827__(void *__out_argv);

struct OUT__7__3827___data 
{
  void *nthreads2_p;
  void *iter1_p;
  void *iter2_p;
  void *cnt_p;
}
;
static void OUT__7__3827__(void *__out_argv);

int test_loop_nested_parreg(unsigned int nthreads1,unsigned int nthreads2,unsigned int iter1,unsigned int iter2,unsigned int stride_enable)
{
  int cnt = 0;
  int check = 0;
  int ret = 0;
  int i = 0;
  struct OUT__7__3827___data __out_argv7__3827__;
  __out_argv7__3827__.cnt_p = ((void *)(&cnt));
  __out_argv7__3827__.iter2_p = ((void *)(&iter2));
  __out_argv7__3827__.iter1_p = ((void *)(&iter1));
  __out_argv7__3827__.nthreads2_p = ((void *)(&nthreads2));
  GOMP_parallel_start(OUT__7__3827__,&__out_argv7__3827__,nthreads1);
  OUT__7__3827__(&__out_argv7__3827__);
  GOMP_parallel_end();
  check = check_parallel((iter1 * iter2));
#ifdef VERBOSE      
#endif
  if (cnt != check) {
    printf("[test_loop_nested_parreg] [CHECK] [ERROR]\n");
    ret = -1;
  }
  else {
    printf("[test_loop_nested_parreg] [CHECK] [CORRECT]\n");
    ret = 0;
  }
  return ret;
}
#endif
#ifdef SECTIONS_LOOP_NESTED_PARALLEL_FOR_TESTS

struct OUT__3__3827___data 
{
  void *tmp_cnt_p;
}
;
static void OUT__3__3827__(void *__out_argv);

struct OUT__4__3827___data 
{
  void *tmp_cnt_p;
}
;
static void OUT__4__3827__(void *__out_argv);

struct OUT__5__3827___data 
{
  void *nthreads2_p;
  void *cnt_p;
}
;
static void OUT__5__3827__(void *__out_argv);

int test_sections_loop_nested_parreg(unsigned int nthreads1,unsigned int nthreads2,unsigned int stride_enable)
{
  int cnt = 0;
  int check = 0;
  int ret = 0;
  struct OUT__5__3827___data __out_argv5__3827__;
  __out_argv5__3827__.cnt_p = ((void *)(&cnt));
  __out_argv5__3827__.nthreads2_p = ((void *)(&nthreads2));
  GOMP_parallel_start(OUT__5__3827__,&__out_argv5__3827__,nthreads1);
  OUT__5__3827__(&__out_argv5__3827__);
  GOMP_parallel_end();
  check = check_parallel(32);
#ifdef VERBOSE      
#endif
  if (cnt != check) {
    printf("[test_sections_loop_nested_parreg] [CHECK] [ERROR]\n");
    ret = -1;
  }
  else {
    printf("[test_sections_loop_nested_parreg] [CHECK] [CORRECT]\n");
    ret = 0;
  }
  return ret;
}
#endif
#ifdef PARALLEL_LOOP_DYNAMIC_TESTS

struct OUT__2__3827___data 
{
  void *cnt_p;
  void *i_p;
}
;
static void OUT__2__3827__(void *__out_argv);

int test_parallel_loop_dynamic(unsigned int nthreads)
{
  int cnt = 0;
  int check = 0;
  int ret = 0;
  int i = 0;
  int j = 0;
  struct OUT__2__3827___data __out_argv2__3827__;
  __out_argv2__3827__.i_p = ((void *)(&i));
  __out_argv2__3827__.cnt_p = ((void *)(&cnt));
  GOMP_parallel_start(OUT__2__3827__,&__out_argv2__3827__,nthreads);
  OUT__2__3827__(&__out_argv2__3827__);
  GOMP_parallel_end();
  check = check_parallel(32);
#ifdef VERBOSE      
#endif
  if (cnt != check) {
    printf("[test_parallel_loop_dynamic] [CHECK] [ERROR]\n");
    ret = -1;
  }
  else {
    printf("[test_parallel_loop_dynamic] [CHECK] [CORRECT]\n");
    ret = 0;
  }
  return ret;
}

struct OUT__1__3827___data 
{
  void *cnt_p;
  void *i_p;
}
;
static void OUT__1__3827__(void *__out_argv);

int test_parallel_loop_dynamic_reduction(unsigned int nthreads)
{
  int cnt = 0;
  int check = 0;
  int ret = 0;
  int i = 0;
  int j = 0;
  struct OUT__1__3827___data __out_argv1__3827__;
  __out_argv1__3827__.i_p = ((void *)(&i));
  __out_argv1__3827__.cnt_p = ((void *)(&cnt));
  GOMP_parallel_start(OUT__1__3827__,&__out_argv1__3827__,nthreads);
  OUT__1__3827__(&__out_argv1__3827__);
  GOMP_parallel_end();
  check = check_parallel(32);
#ifdef VERBOSE      
#endif
  if (cnt != check) {
    printf("[test_parallel_loop_dynamic_reduction] [CHECK] [ERROR]\n");
    ret = -1;
  }
  else {
    printf("[test_parallel_loop_dynamic_reduction] [CHECK] [CORRECT]\n");
    ret = 0;
  }
  return ret;
}
#endif

int omp_fn1(struct _omp_param *params)
{
  int i;
  for (i = 0; i < 2; ++i) {
#ifdef SINGLE_PARALLEL_TESTS
    printf("[test_single_parreg] THREADS 1\n");
    test_single_parreg(1);
    printf("[test_single_parreg] THREADS 2\n");
    test_single_parreg(2);
    printf("[test_single_parreg] THREADS 3\n");
    test_single_parreg(3);
    printf("[test_single_parreg] THREADS 4\n");
    test_single_parreg(4);
#endif
#ifdef NESTED_PARALLEL_TESTS    
    printf("[test_nested_parreg] THREADS 2-2\n");
    test_nested_parreg(2,2);
#endif
#ifdef SINGLE_PARALLEL_SECTIONS_TESTS       
    printf("[test_sections_single_parreg] THREADS 4 - SECTIONS 4\n");
    test_sections_single_parreg(4,1);
#endif
#ifdef SINGLE_PARALLEL_FOR_TESTS
    printf("[test_loop_single_parreg] THREADS 4\n");
    test_loop_single_parreg(4,1);
#endif
#ifdef NESTED_PARALLEL_FOR_TESTS
    printf("[test_loop_nested_parreg] THREADS 2-2\n");
    test_loop_nested_parreg(2,2,4,4,0);
#endif
#ifdef SECTIONS_LOOP_NESTED_PARALLEL_FOR_TESTS
    printf("[test_sections_loop_nested_parreg] THREADS 2-2\n");
    test_sections_loop_nested_parreg(2,2,0);
#endif
#ifdef SECTION_NESTED_PARALLEL
    printf("[test_sections_loop_nested_parreg] THREADS 2-2\n");
    test_sections_nested_parreg(2,2);
#endif
#ifdef PARALLEL_LOOP_DYNAMIC_TESTS
    printf("[test_parallel_loop_dynamic] THREADS 4");
    test_parallel_loop_dynamic(4);
    printf("[test_parallel_loop_dynamic_reduction] THREADS 4\n");
    test_parallel_loop_dynamic_reduction(4);
#endif
    printf("---------------------------------------------------\n");
  }
  return 0;
}

static void OUT__1__3827__(void *__out_argv)
{
  int *cnt = (int *)(((struct OUT__1__3827___data *)__out_argv) -> cnt_p);
  int *i = (int *)(((struct OUT__1__3827___data *)__out_argv) -> i_p);
  int _p_cnt;
  _p_cnt = 0;
  int _p_j;
  long p_index_;
  long p_lower_;
  long p_upper_;
  if (GOMP_loop_dynamic_start(0,31,1,1,&p_lower_,&p_upper_)) {
    do {
      for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
        int ID = ((1 + p_index_) + (32 *  *i));
#ifdef VERBOSE              
#endif
        _p_cnt += ID;
      }
    }while (GOMP_loop_dynamic_next(&p_lower_,&p_upper_));
  }
  GOMP_atomic_start();
   *cnt =  *cnt + _p_cnt;
  GOMP_atomic_end();
  GOMP_loop_end();
}

static void OUT__2__3827__(void *__out_argv)
{
  int *cnt = (int *)(((struct OUT__2__3827___data *)__out_argv) -> cnt_p);
  int *i = (int *)(((struct OUT__2__3827___data *)__out_argv) -> i_p);
  int _p_j;
  long p_index_;
  long p_lower_;
  long p_upper_;
  if (GOMP_loop_dynamic_start(0,31,1,1,&p_lower_,&p_upper_)) {
    do {
      for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
        int ID = ((1 + p_index_) + (32 *  *i));
#ifdef VERBOSE              
#endif      
        GOMP_critical_start();
         *cnt += ID;
        GOMP_critical_end();
      }
    }while (GOMP_loop_dynamic_next(&p_lower_,&p_upper_));
  }
  GOMP_loop_end();
}

static void OUT__3__3827__(void *__out_argv)
{
  int *tmp_cnt = (int *)(((struct OUT__3__3827___data *)__out_argv) -> tmp_cnt_p);
{
    int _p_i;
    long p_index_;
    long p_lower_;
    long p_upper_;
    if (GOMP_loop_dynamic_start(0,15,1,1,&p_lower_,&p_upper_)) {
      do {
        for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
          int ID = (17 + p_index_);
#ifdef VERBOSE      
#endif
          GOMP_critical_start();
           *tmp_cnt += ID;
          GOMP_critical_end();
        }
      }while (GOMP_loop_dynamic_next(&p_lower_,&p_upper_));
    }
    GOMP_loop_end();
  }
}

static void OUT__4__3827__(void *__out_argv)
{
  int *tmp_cnt = (int *)(((struct OUT__4__3827___data *)__out_argv) -> tmp_cnt_p);
{
    int _p_i;
    long p_index_;
    long p_lower_;
    long p_upper_;
    if (GOMP_loop_dynamic_start(0,15,1,1,&p_lower_,&p_upper_)) {
      do {
        for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
          int ID = (1 + p_index_);
#ifdef VERBOSE                          
#endif
          GOMP_critical_start();
           *tmp_cnt += ID;
          GOMP_critical_end();
        }
      }while (GOMP_loop_dynamic_next(&p_lower_,&p_upper_));
    }
    GOMP_loop_end();
  }
}

static void OUT__5__3827__(void *__out_argv)
{
  unsigned int *nthreads2 = (unsigned int *)(((struct OUT__5__3827___data *)__out_argv) -> nthreads2_p);
  int *cnt = (int *)(((struct OUT__5__3827___data *)__out_argv) -> cnt_p);
  int tmp_cnt = 0;
  int i = 0;
{
    int gomp_section_1 = GOMP_sections_start(2);
    while(gomp_section_1 > 0){
      switch(gomp_section_1){
        case 1:
{
          struct OUT__4__3827___data __out_argv4__3827__;
          __out_argv4__3827__.tmp_cnt_p = ((void *)(&tmp_cnt));
          GOMP_parallel_start(OUT__4__3827__,&__out_argv4__3827__, *nthreads2);
          OUT__4__3827__(&__out_argv4__3827__);
          GOMP_parallel_end();
          break; 
        }
        case 2:
{
          struct OUT__3__3827___data __out_argv3__3827__;
          __out_argv3__3827__.tmp_cnt_p = ((void *)(&tmp_cnt));
          GOMP_parallel_start(OUT__3__3827__,&__out_argv3__3827__, *nthreads2);
          OUT__3__3827__(&__out_argv3__3827__);
          GOMP_parallel_end();
          break; 
        }
        default:
{
          abort();
        }
      }
      gomp_section_1 = GOMP_sections_next();
    }
    GOMP_sections_end();
  }
  GOMP_critical_start();
   *cnt += tmp_cnt;
  GOMP_critical_end();
}

static void OUT__6__3827__(void *__out_argv)
{
  unsigned int *iter1 = (unsigned int *)(((struct OUT__6__3827___data *)__out_argv) -> iter1_p);
  unsigned int *iter2 = (unsigned int *)(((struct OUT__6__3827___data *)__out_argv) -> iter2_p);
  int *i = (int *)(((struct OUT__6__3827___data *)__out_argv) -> i_p);
  int *tmp_cnt = (int *)(((struct OUT__6__3827___data *)__out_argv) -> tmp_cnt_p);
  int j = 0;
{
    int _p_j;
    long p_index_;
    long p_lower_;
    long p_upper_;
    if (GOMP_loop_dynamic_start(0, *iter2 - 1,1,1,&p_lower_,&p_upper_)) {
      do {
        for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
          int ID = (((( *i) *  *iter1) + p_index_) + 1);
#ifdef VERBOSE                          
#endif
          GOMP_critical_start();
           *tmp_cnt += ID;
          GOMP_critical_end();
        }
      }while (GOMP_loop_dynamic_next(&p_lower_,&p_upper_));
    }
    GOMP_loop_end();
  }
}

static void OUT__7__3827__(void *__out_argv)
{
  unsigned int *nthreads2 = (unsigned int *)(((struct OUT__7__3827___data *)__out_argv) -> nthreads2_p);
  unsigned int *iter1 = (unsigned int *)(((struct OUT__7__3827___data *)__out_argv) -> iter1_p);
  unsigned int *iter2 = (unsigned int *)(((struct OUT__7__3827___data *)__out_argv) -> iter2_p);
  int *cnt = (int *)(((struct OUT__7__3827___data *)__out_argv) -> cnt_p);
  int _p_i;
{
    long p_index_;
    long p_lower_;
    long p_upper_;
    if (GOMP_loop_dynamic_start(0, *iter1 - 1,1,1,&p_lower_,&p_upper_)) {
      do {
        for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
          int tmp_cnt = 0;
          struct OUT__6__3827___data __out_argv6__3827__;
          __out_argv6__3827__.tmp_cnt_p = ((void *)(&tmp_cnt));
          __out_argv6__3827__.i_p = ((void *)(&p_index_));
          __out_argv6__3827__.iter2_p = ((void *)(&( *iter2)));
          __out_argv6__3827__.iter1_p = ((void *)(&( *iter1)));
          GOMP_parallel_start(OUT__6__3827__,&__out_argv6__3827__, *nthreads2);
          OUT__6__3827__(&__out_argv6__3827__);
          GOMP_parallel_end();
          GOMP_critical_start();
           *cnt += tmp_cnt;
          GOMP_critical_end();
        }
      }while (GOMP_loop_dynamic_next(&p_lower_,&p_upper_));
    }
    GOMP_loop_end();
  }
}

static void OUT__8__3827__(void *__out_argv)
{
  unsigned int *iter = (unsigned int *)(((struct OUT__8__3827___data *)__out_argv) -> iter_p);
  int *cnt = (int *)(((struct OUT__8__3827___data *)__out_argv) -> cnt_p);
  int i = 0;
  int j = 0;
  for (i = 0; i <  *iter; i++) {{
      int _p_j;
      long p_index_;
      long p_lower_;
      long p_upper_;
      if (GOMP_loop_dynamic_start(0,32,1,1,&p_lower_,&p_upper_)) {
        do {
          for (p_index_ = p_lower_; p_index_ < p_upper_; p_index_ += 1) {
            int ID = ((1 + p_index_) + (32 * i));
#ifdef VERBOSE     
            printf ("Execute Iteration ID %d - Processor %d\n", ID);           
#endif
            GOMP_critical_start();
{
               *cnt += ID;
            }
            GOMP_critical_end();
          }
        }while (GOMP_loop_dynamic_next(&p_lower_,&p_upper_));
      }
      GOMP_loop_end();
    }
  }
}

static void OUT__9__3827__(void *__out_argv)
{
  int *reg_id = (int *)(((struct OUT__9__3827___data *)__out_argv) -> reg_id_p);
  int *tmp_cnt = (int *)(((struct OUT__9__3827___data *)__out_argv) -> tmp_cnt_p);
{
    int gomp_section_2 = GOMP_sections_start(4);
    while(gomp_section_2 > 0){
      switch(gomp_section_2){
        case 1:
{
          int ID = (1 + (4 *  *reg_id));
#ifdef VERBOSE                          
#endif
          GOMP_critical_start();
           *tmp_cnt += ID;
          GOMP_critical_end();
          break; 
        }
        case 2:
{
          int ID = (2 + (4 *  *reg_id));
#ifdef VERBOSE                          
#endif
          GOMP_critical_start();
           *tmp_cnt += ID;
          GOMP_critical_end();
          break; 
        }
        case 3:
{
          int ID = (3 + (4 *  *reg_id));
#ifdef VERBOSE                          
#endif
          GOMP_critical_start();
           *tmp_cnt += ID;
          GOMP_critical_end();
          break; 
        }
        case 4:
{
          int ID = (4 + (4 *  *reg_id));
#ifdef VERBOSE                          
#endif
          GOMP_critical_start();
           *tmp_cnt += ID;
          GOMP_critical_end();
          break; 
        }
        default:
{
          abort();
        }
      }
      gomp_section_2 = GOMP_sections_next();
    }
    GOMP_sections_end();
  }
}

static void OUT__10__3827__(void *__out_argv)
{
  unsigned int *nthreads2 = (unsigned int *)(((struct OUT__10__3827___data *)__out_argv) -> nthreads2_p);
  int *cnt = (int *)(((struct OUT__10__3827___data *)__out_argv) -> cnt_p);
  int reg_id = omp_get_thread_num();
  int tmp_cnt = 0;
  struct OUT__9__3827___data __out_argv9__3827__;
  __out_argv9__3827__.tmp_cnt_p = ((void *)(&tmp_cnt));
  __out_argv9__3827__.reg_id_p = ((void *)(&reg_id));
  GOMP_parallel_start(OUT__9__3827__,&__out_argv9__3827__, *nthreads2);
  OUT__9__3827__(&__out_argv9__3827__);
  GOMP_parallel_end();
  GOMP_critical_start();
   *cnt += tmp_cnt;
  GOMP_critical_end();
}

static void OUT__11__3827__(void *__out_argv)
{
  unsigned int *iter = (unsigned int *)(((struct OUT__11__3827___data *)__out_argv) -> iter_p);
  int *cnt = (int *)(((struct OUT__11__3827___data *)__out_argv) -> cnt_p);
  int i = 0;
  for (i = 0; i <  *iter; i++) {{
      int gomp_section_3 = GOMP_sections_start(4);
      while(gomp_section_3 > 0){
        switch(gomp_section_3){
          case 1:
{
            int ID = (1 + (4 * i));
#ifdef VERBOSE                  
#endif
            GOMP_critical_start();
             *cnt += ID;
            GOMP_critical_end();
            break; 
          }
          case 2:
{
            int ID = (2 + (4 * i));
#ifdef VERBOSE                          
#endif
            GOMP_critical_start();
             *cnt += ID;
            GOMP_critical_end();
            break; 
          }
          case 3:
{
            int ID = (3 + (4 * i));
#ifdef VERBOSE                          
#endif
            GOMP_critical_start();
             *cnt += ID;
            GOMP_critical_end();
            break; 
          }
          case 4:
{
            int ID = (4 + (4 * i));
#ifdef VERBOSE                          
#endif
            GOMP_critical_start();
             *cnt += ID;
            GOMP_critical_end();
            break; 
          }
          default:
{
            abort();
          }
        }
        gomp_section_3 = GOMP_sections_next();
      }
      GOMP_sections_end();
    }
  }
}

static void OUT__12__3827__(void *__out_argv)
{
  int *tmp_cnt = (int *)(((struct OUT__12__3827___data *)__out_argv) -> tmp_cnt_p);
  int proc_id = get_proc_id()+1;
// To be sure that same processor are used we need to perform some work
{
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
  }
{
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
  }
{
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
  }
{
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
  }
{
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
{
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
{
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
{
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
          __asm__ __volatile__ ("nop \n\t;;\n\t");
        }
      }
    }
  }
  GOMP_critical_start();
   *tmp_cnt += proc_id;
  GOMP_critical_end();
#ifdef VERBOSE
#endif
}

static void OUT__13__3827__(void *__out_argv)
{
  unsigned int *nthreads_2 = (unsigned int *)(((struct OUT__13__3827___data *)__out_argv) -> nthreads_2_p);
  int *cnt = (int *)(((struct OUT__13__3827___data *)__out_argv) -> cnt_p);
  int tmp_cnt = 0;
  struct OUT__12__3827___data __out_argv12__3827__;
  __out_argv12__3827__.tmp_cnt_p = ((void *)(&tmp_cnt));
  GOMP_parallel_start(OUT__12__3827__,&__out_argv12__3827__, *nthreads_2);
  OUT__12__3827__(&__out_argv12__3827__);
  GOMP_parallel_end();
  GOMP_critical_start();
   *cnt += tmp_cnt;
  GOMP_critical_end();
}

static void OUT__14__3827__(void *__out_argv)
{
  int *cnt = (int *)(((struct OUT__14__3827___data *)__out_argv) -> cnt_p);
  int proc_id = get_proc_id()+1;
  GOMP_critical_start();
   *cnt += proc_id;
  GOMP_critical_end();
}
