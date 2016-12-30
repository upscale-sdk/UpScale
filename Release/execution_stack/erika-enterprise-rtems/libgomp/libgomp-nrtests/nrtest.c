/* Copyright 2014 DEI - Universita' di Bologna
   author       DEI - Universita' di Bologna
                Alessandro Capotondi - alessandro.capotondi@unibo.it
                Andrea Marongiu - a.marongiu@unibo.it
                Davide Rossi - davide.rossi@unibo.it
   info         NonRegression test Libgomp */

//NOTE[ALE] All the tests work fine ;)


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

int check_parallel(unsigned int nthreads)
{
    int ret = 0;
    
    switch(nthreads)
    {
        case 2:
            ret = 3;
            break;
        case 17:
            ret = 153;
            break;          
        case 33:
            ret = 561;
            break;          
        case 49:
            ret = 1225;
            break;
        case 16:
            ret = 136;
            break;
        case 32:
            ret = 528;
            break;          
        case 64:
            ret = 2080;
            break;          
        case 256:
            ret = 32896;
            break;          
        case 128:
            ret = 8256;
            break;          
        default:
            while(nthreads > 0)
                    ret += nthreads--;
                break;
    }
        
    return ret;
}

int check_nested_parallel(unsigned int nthreads1, unsigned int nthreads2)
{
        int ret = 0;
        int i,j, iterations;
    
    if (nthreads1 == 4 && nthreads2 == 4)
        ret = 136;
    else if (nthreads1 == 4 && nthreads2 == 16)
        ret = 2080;
    else
    {
            iterations = nthreads1 * nthreads2;
            while(iterations > 0)
            {
                ret += iterations;
                    iterations--;
            }
    }
    
    return ret;
}

#ifdef SINGLE_PARALLEL_TESTS
int test_single_parreg(unsigned int nthreads)
{
    int cnt = 0;
    int check = 0;
    int ret = 0;
    int i = 0;
    
    #pragma omp parallel num_threads(nthreads)
    {
        int proc_id = get_proc_id();

        #pragma omp critical
        cnt += proc_id;
    }

    check = check_parallel(nthreads);

#ifdef VERBOSE      
    printf("CNT %d\n", cnt);
    printf("CHECK %d\n", check);
#endif  

    if(cnt != check)
    {
        printf("[test_single_parreg] [CHECK] [ERROR]\n");
        ret = -1;
    }
    else
    {
        printf("[test_single_parreg] [CHECK] [CORRECT]\n");
        ret = 0;
    }

    return ret;
}
#endif

#ifdef NESTED_PARALLEL_TESTS
int test_nested_parreg(unsigned int nthreads_1, unsigned int nthreads_2)
{
    int cnt = 0;
    int check = 0;
    int ret = 0;
    
    #pragma omp parallel num_threads(nthreads_1)
    {   
        int tmp_cnt = 0;
        
        #pragma omp parallel num_threads(nthreads_2)
        {
            int proc_id = get_proc_id();
            
            // To be sure that same processor are used we need to perform some work
            _200_nop_block
            _200_nop_block
            _200_nop_block
            _200_nop_block
            _200_nop_block
        
            #pragma omp critical
            tmp_cnt += proc_id;

#ifdef VERBOSE
            printf("I am on proc %d\n", proc_id);
#endif
        }
        
        #pragma omp critical
        cnt += tmp_cnt;
    }
    
    check = check_nested_parallel (nthreads_1, nthreads_2);
    
#ifdef VERBOSE      
    printf("CNT %d\n",cnt);
    printf("CHECK %d\n", check);
#endif

    if(cnt != check)
    {
        printf("[test_nested_parreg] [CHECK] [ERROR]\n");
        ret = -1;
    }
    else
    {
         printf("[test_nested_parreg] [CHECK] [CORRECT]\n");
         ret = 0;
    }

    return ret;
}
#endif

#ifdef SINGLE_PARALLEL_SECTIONS_TESTS
int test_sections_single_parreg(unsigned int nthreads, unsigned int iter)
{
    int cnt = 0;
    int check = 0;
    int ret = 0;

    #pragma omp parallel num_threads(nthreads)
    {
        int i = 0;
        
        for (i = 0; i < iter; i++)
        {
            #pragma omp sections //nowait
            {
                #pragma omp section
                {
                    int ID = 1 + 4*i;

#ifdef VERBOSE                  
                    printf("Execute Section ID %d\n", ID);
#endif

                    #pragma omp critical
                    cnt += ID;
                }

                #pragma omp section
                {
                    int ID = 2 + 4*i;

#ifdef VERBOSE                          
                    printf("Execute Section ID %d\n", ID);
#endif
                    
                    #pragma omp critical
                    cnt += ID;
                }
                
                #pragma omp section
                {
                    int ID = 3 + 4*i;

#ifdef VERBOSE                          
                    printf("Execute Section ID %d\n", ID);
#endif
                    
                    #pragma omp critical
                    cnt += ID;
                }                       
                
                #pragma omp section
                {
                    int ID = 4 + 4*i;

#ifdef VERBOSE                          
                    printf("Execute Section ID %d\n", ID);
#endif

                    #pragma omp critical
                    cnt += ID;
                }           
            }
        }
    }

    check = check_parallel(4*iter);
    
#ifdef VERBOSE      
    printf("CNT %d\n", cnt);
    printf("CHECK %d\n", check);
#endif

        if(cnt != check)
    {
        printf("[test_sections_single_parreg] [CHECK] [ERROR]\n");
        ret = -1;
    }
    else
    {
        printf("[test_sections_single_parreg] [CHECK] [CORRECT]\n");
         ret = 0;
    }
    
    return ret;
}
#endif

#ifdef SECTION_NESTED_PARALLEL
int test_sections_nested_parreg(unsigned int nthreads1, unsigned int nthreads2)
{
    int cnt = 0;
    int check = 0;
    int ret = 0;
        
    #pragma omp parallel num_threads(nthreads1)
    {       
        int reg_id  = omp_get_thread_num();
        int tmp_cnt = 0;
        
        #pragma omp parallel num_threads(nthreads2)
        {   
            #pragma omp sections //nowait
            {
                #pragma omp section
                {
                    int ID = 1 + 4*reg_id;

#ifdef VERBOSE                          
                    printf("Execute Section ID %d\n", ID);
#endif
                    
                    #pragma omp critical
                    tmp_cnt += ID;
                }

                #pragma omp section
                {
                    int ID = 2 + 4*reg_id;

#ifdef VERBOSE                          
                    printf("Execute Section ID %d\n", ID);
#endif
                    
                    #pragma omp critical
                    tmp_cnt += ID;
                }
                
                #pragma omp section
                {
                    int ID = 3 + 4*reg_id;

#ifdef VERBOSE                          
                    printf("Execute Section ID %d\n", ID);
#endif
                    
                    #pragma omp critical
                    tmp_cnt += ID;
                }                       
                
                #pragma omp section
                {
                    int ID = 4 + 4*reg_id;

#ifdef VERBOSE                          
                    printf("Execute Section ID %d\n", ID);
#endif

                    #pragma omp critical
                    tmp_cnt += ID;
                }           
            }
        }
        
        #pragma omp critical
        cnt += tmp_cnt;
    }
    
    check = check_parallel(4*nthreads1);
    
#ifdef VERBOSE      
    printf("CNT %d\n",cnt);
    printf("CHECK %d\n", check);
#endif

    if(cnt != check)
    {
        printf("[test_sections_nested_parreg] [CHECK] [ERROR]\n");
        ret = -1;
    }
    else
    {
        printf("[test_sections_nested_parreg] [CHECK] [CORRECT]\n");
        ret = 0;
    }

    return ret;
}
#endif

#ifdef SINGLE_PARALLEL_FOR_TESTS
int test_loop_single_parreg(unsigned int nthreads, unsigned int iter)
{
    int cnt = 0;
    int check = 0;
    int ret = 0;    
    
    #pragma omp parallel num_threads(nthreads)
    {   

        int i = 0;
        int j = 0;
        for (i = 0; i < iter; i++)
		{
            #pragma omp for schedule(dynamic, 1) //nowait 
            for (j = 0; j < LOOP_ITER; j++)
            {
                int ID = 1 + j + LOOP_ITER*i;

#ifdef VERBOSE                      
                printf("Execute Iteration ID %d - Processor %d\n", ID);
#endif

                #pragma omp critical
                {
                cnt += ID;
                }
            }
        }

    }
        
    check = check_parallel(iter*LOOP_ITER);
    
#ifdef VERBOSE      
    printf("CNT %d - Processor %d\n", cnt);
    printf("CHECK %d - Processor %d\n", check);
#endif

    if(cnt != check)
    {
        printf("[test_loop_single_parreg] [CHECK] [ERROR]\n");
        ret = -1;
    }
    else
    {
        printf("[test_loop_single_parreg] [CHECK] [CORRECT]\n");
        ret = 0;
    }
    
    return ret;
}
#endif

#ifdef NESTED_PARALLEL_FOR_TESTS
int test_loop_nested_parreg(unsigned int nthreads1, unsigned int nthreads2, unsigned int iter1, unsigned int iter2, unsigned int stride_enable)
{
    int cnt = 0;
    int check = 0;
    int ret = 0;
    int i = 0;
    
    #pragma omp parallel num_threads(nthreads1) private(i) shared(cnt)
    {
        #pragma omp for schedule(dynamic,1) //nowait
        for (i = 0; i < iter1; i++)
        {
            int tmp_cnt = 0;        
            #pragma omp parallel num_threads(nthreads2) shared(tmp_cnt)
            {
                int j = 0;
                
                #pragma omp for schedule(dynamic,1)
                for (j = 0; j < iter2; j++)
                {
                    int ID = i*iter1+j+1;

#ifdef VERBOSE                          
                    printf ("Execute Iteration ID %d\n", ID);
                    //printf("i %d - j %d\n",i,j);
#endif
                    
                    #pragma omp critical
                    tmp_cnt += ID;
                }
            }
            
            #pragma omp critical
            cnt += tmp_cnt;
        }
    }

    check = check_parallel(iter1*iter2);
    
#ifdef VERBOSE      
    printf("CNT %d - Processor %d\n",cnt);
    printf("CHECK %d - Processor %d\n", check);
#endif

    if(cnt != check)
    {
        printf("[test_loop_nested_parreg] [CHECK] [ERROR]\n");
        ret = -1;
    }
    else
    {
        printf("[test_loop_nested_parreg] [CHECK] [CORRECT]\n");
        ret = 0;
    }
    
    return ret;
}
#endif

#ifdef SECTIONS_LOOP_NESTED_PARALLEL_FOR_TESTS
int test_sections_loop_nested_parreg(unsigned int nthreads1, unsigned int nthreads2, unsigned int stride_enable)
{
    int cnt = 0;
    int check = 0;
    int ret = 0;

    #pragma omp parallel num_threads(nthreads1)
    {
        int tmp_cnt = 0;
        int i = 0;
        
        #pragma omp sections
        {
            #pragma omp section
            {
                #pragma omp parallel num_threads (nthreads2)
                { 
                    #pragma omp for schedule(dynamic,1)
                    for (i=0; i < 16; i++)
                    {
                        int ID = 1 + i;
                    
#ifdef VERBOSE                          
                        printf ("Iteration%d - Processor %d\n", ID);
#endif
                    
                        #pragma omp critical
                        tmp_cnt += ID;
                    }
                }
            }
            
            #pragma omp section
            {
                #pragma omp parallel num_threads (nthreads2)
                { 
                    #pragma omp for schedule(dynamic,1)
                    for (i=0; i < 16; i++)
                    {
                        int ID = 17 + i;

#ifdef VERBOSE      
                        printf ("Iteration%d - Processor %d\n", ID);
#endif
                    
                        #pragma omp critical
                        tmp_cnt += ID;
                    }
                }
            }
        }
        
        #pragma omp critical
        cnt += tmp_cnt;
    }
    check = check_parallel(32);
    
#ifdef VERBOSE      
    printf("CNT %d - Processor %d\n",cnt);
    printf("CHECK %d - Processor %d\n", check);
#endif

    if(cnt != check)
    {
        printf(ANSI_COLOR_BLUE"[test_sections_loop_nested_parreg] [CHECK] [" ANSI_COLOR_RED "ERROR" ANSI_COLOR_BLUE "]"ANSI_COLOR_RESET "\n");
        ret = -1;
    }
    else
    {
        printf(ANSI_COLOR_BLUE"[test_sections_loop_nested_parreg] [CHECK] [" ANSI_COLOR_GREEN "CORRECT" ANSI_COLOR_BLUE "]"ANSI_COLOR_RESET "\n");
         ret = 0;
    }
    
    return ret;
}
#endif

#ifdef PARALLEL_LOOP_DYNAMIC_TESTS
int test_parallel_loop_dynamic(unsigned int nthreads)
{
    int cnt = 0;
    int check = 0;
    int ret = 0;    
    int i = 0;
    int j = 0;
        
    #pragma omp parallel for schedule(dynamic, 1) num_threads(nthreads)
    for (j = 0; j < LOOP_ITER; j++)
    {
        int ID = 1 + j + LOOP_ITER*i;

#ifdef VERBOSE              
        printf("Execute Iteration ID %d - Processor %d\n", ID);
#endif      
        
        #pragma omp critical
        cnt += ID;
    }
        
    check = check_parallel(LOOP_ITER);
    
#ifdef VERBOSE      
    printf("CNT %d - Processor %d\n",cnt);
    printf("CHECK %d - Processor %d\n", check);
#endif

    if(cnt != check)
    {
        printf(ANSI_COLOR_BLUE"[test_parallel_loop_dynamic] [CHECK] [" ANSI_COLOR_RED "ERROR" ANSI_COLOR_BLUE "]"ANSI_COLOR_RESET "\n");
        ret = -1;
    }
    else
    {
        printf(ANSI_COLOR_BLUE"[test_parallel_loop_dynamic] [CHECK] [" ANSI_COLOR_GREEN "CORRECT" ANSI_COLOR_BLUE "]"ANSI_COLOR_RESET "\n");
         ret = 0;
    }
    
    return ret;
}

int test_parallel_loop_dynamic_reduction(unsigned int nthreads)
{
    int cnt = 0;
    int check = 0;
    int ret = 0;    
    int i = 0;
    int j = 0;
        
    #pragma omp parallel for schedule(dynamic, 1) reduction(+:cnt) num_threads(nthreads)
    for (j = 0; j < LOOP_ITER; j++)
    {
        int ID = 1 + j + LOOP_ITER*i;

#ifdef VERBOSE              
        printf("Execute Iteration ID %d - Processor %d\n", ID);
#endif

        cnt += ID;
    }
        
    check = check_parallel(LOOP_ITER);
    
#ifdef VERBOSE      
    printf("CNT %d - Processor %d\n",cnt);
    printf("CHECK %d - Processor %d\n", check);
#endif

    if(cnt != check)
    {
        printf(ANSI_COLOR_BLUE"[test_parallel_loop_dynamic_reduction] [CHECK] [" ANSI_COLOR_RED "ERROR" ANSI_COLOR_BLUE "]"ANSI_COLOR_RESET "\n");
        ret = -1;
    }
    else
    {
        printf(ANSI_COLOR_BLUE"[test_parallel_loop_dynamic_reduction] [CHECK] [" ANSI_COLOR_GREEN "CORRECT" ANSI_COLOR_BLUE "]"ANSI_COLOR_RESET "\n");
         ret = 0;
    }
    
    return ret;
}
#endif

int main()
{
int i;
for(i = 0; i < NB_STRESS; ++i){
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
    test_nested_parreg(2, 2);
#endif
    
#ifdef SINGLE_PARALLEL_SECTIONS_TESTS       
    printf("[test_sections_single_parreg] THREADS 4 - SECTIONS 4\n");
    test_sections_single_parreg(4, 1);
#endif

#ifdef SINGLE_PARALLEL_FOR_TESTS
    printf("[test_loop_single_parreg] THREADS 4\n");
    test_loop_single_parreg (4, 1);
#endif

#ifdef NESTED_PARALLEL_FOR_TESTS
    printf("[test_loop_nested_parreg] THREADS 2-2\n");
    test_loop_nested_parreg(2, 2, 4, 4, 0);
#endif

#ifdef SECTIONS_LOOP_NESTED_PARALLEL_FOR_TESTS
    printf("[test_sections_loop_nested_parreg] THREADS 2-2\n");
    test_sections_loop_nested_parreg (2, 2, 0);
#endif
    
#ifdef SECTION_NESTED_PARALLEL
    printf("[test_sections_loop_nested_parreg] THREADS 2-2\n");
    test_sections_nested_parreg(2,2);
#endif
#ifdef PARALLEL_LOOP_DYNAMIC_TESTS
    printf("[test_parallel_loop_dynamic] THREADS 4");
    test_parallel_loop_dynamic (4);
    printf("[test_parallel_loop_dynamic_reduction] THREADS 4\n");
    test_parallel_loop_dynamic_reduction (4);
#endif
    printf("---------------------------------------------------\n");
    
}
    
    return 0;
}
