#include <omp.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

static inline double getTime_us()
{
    /* Linux */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double ret = 0.0f;
    ret = (double) tv.tv_usec;
    /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
    //ret /= 1000;
    /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
    ret += (((double) (tv.tv_sec)) * 1000000);
    return ret;
}

#define CONF95 1.96

#define DEFAULT_NB_TESTS            12
#define DEFAULT_DELAY_LENGTH        -1      // -1 means the delay length should be auto generated
#define DEFAULT_OUTER_REPS          30      // Outer repetitions
#define DEFAULT_TEST_TARGET_TIME    10000.0 // Test Target time in microseconds.
#define DEFAULT_DELAY_TIME          100.0   // Default delaytime in microseconds
#ifndef DEFAULT_NTHREADS
#define DEFAULT_NTHREADS            16
#endif

int                __outerreps__      = DEFAULT_OUTER_REPS;
float              __targettesttime__ = DEFAULT_TEST_TARGET_TIME;
int                __nthreads__       = DEFAULT_NTHREADS;
float              __delaytime__      = DEFAULT_DELAY_TIME;
unsigned long      __innerreps__[DEFAULT_NB_TESTS];
float              __meantime__[DEFAULT_NB_TESTS];
float              __mintime__[DEFAULT_NB_TESTS];
float              __maxtime__[DEFAULT_NB_TESTS];
float              __sd__[DEFAULT_NB_TESTS];
int                __nr__[DEFAULT_NB_TESTS];
float              __referencetime__[DEFAULT_NB_TESTS];         
float              __referencesd__[DEFAULT_NB_TESTS];
float              __testtime__[DEFAULT_NB_TESTS];              
float              __testsd__[DEFAULT_NB_TESTS];                
unsigned long long __meancycles__[DEFAULT_NB_TESTS];
unsigned long long __refcycles__[DEFAULT_NB_TESTS];


typedef enum {
    REF1,
    PARALLEL,
    FOR,
    PARALLEL_FOR,
    BARRIER,
    SINGLE,
    CRITICAL,
    LOCKS,
    REF2,
    ATOMIC,
    REF3,
    REDUCTION
} BenchType_t;

static inline char *getName(BenchType_t type){
    switch(type)
    {
        case REF1:
            return "Reference 1";
        case PARALLEL:
            return "PARALLEL";
        case FOR:
            return "FOR";
        case PARALLEL_FOR:
            return "PARALLEL FOR";
        case BARRIER:
            return "BARRIER";
        case SINGLE:
            return "SINGLE";
        case CRITICAL:
            return "CRITICAL";
        case LOCKS:
            return "LOCK/UNLOCK";
        case REF2:
            return "Reference 2";
        case ATOMIC:
            return "ATOMIC";
        case REF3:
            return "Reference 3";
        case REDUCTION:
            return "REDUCTION";
    }
    return "";
    
}

int delaylength;        // The number of iterations to delay for
int nthreads;

// should run for.
unsigned long innerreps; // Inner repetitions
double *times; // Array of unisigned long longs storing benchmark cycle counts

float referencetime;    // The average reference time in microseconds to perform
// outerreps runs
float referencesd;      // The standard deviation in the reference time in
// microseconds for outerreps runs.
float testtime;         // The average test time in microseconds for
// outerreps runs
float testsd;       // The standard deviation in the test time in
// microseconds for outerreps runs.
unsigned long long meancycles; // mean cycle count

unsigned long long refcycles; // reference mean cycle count

void init(int bench);

void initreference(BenchType_t type);

void finalisereference(BenchType_t type);

void intitest(BenchType_t type);

void finalisetest(BenchType_t type);

unsigned long long getcycles();

static inline double gettime() {
    return getTime_us();
};

void delay(int delaylength);

void array_delay(int delaylength, float a[1]);

int getdelaylengthfromtime(float delaytime);

int returnfalse(void);

void finalise(void);

void benchmark(BenchType_t type, void (*test)(void));

void reference(BenchType_t type, void (*refer)(void));

void stats(BenchType_t type, float*, float*, unsigned long long*);

void refer(void);

void referatom(void);

void referred(void);

void testpr(void);

void testprn(void);

void testfor(void);

void testpfor(void);

void testbar(void);

void testsing(void);

void testcrit(void);

void testlock(void);

void testatom(void);

void testred(void);

omp_lock_t lock;

static inline int timetuning(int nbAdds)
{
    int i;
    int ret = 0;
    for(i = 0; i < nbAdds; ++i)
        ret += i;
    
    return ret;
}
    
void omp_EPCC(struct _omp_param *params) {
    
    printf("Running OpenMP benchmark version 3.0\n"
    "\t%d thread(s)\n"
    "\t%d outer repetitions\n"
    "\t%0.2f test time (microseconds)\n"
    "\t%f delay time (microseconds)\n",
          __nthreads__,
          __outerreps__,
          __targettesttime__,
          __delaytime__);
    
    double start, stop;
    int adds;
    int ret;
    
    /* Intializes random number generator */
    srand((unsigned) time(NULL));
    adds = 1000000;
    
    start = gettime();
    ret = timetuning(adds);
    stop = gettime();
    printf("[TimeTuning_1] Execution Time %f nbAdds %d\t\tFreq %lld [ops/s] \t (ret %d)\n", stop-start, adds, (long long)(((double) adds / (double)(stop-start)) * (double) 1000000), ret);
    
    adds = ret/1000;
    start = gettime();
    ret = timetuning(adds);
    stop = gettime();
    printf("[TimeTuning_2] Execution Time %f nbAdds %d\t\tFreq %lld [ops/s] \t (ret %d)\n", stop-start, adds, (long long)(((double) adds / (double)(stop-start)) * (double) 1000000), ret);

    adds = ret/100;
    start = gettime();
    ret = timetuning(adds);
    stop = gettime();
    printf("[TimeTuning_3] Execution Time %f nbAdds %d\t\tFreq %lld [ops/s] \t (ret %d)\n", stop-start, adds, (long long)(((double) adds / (double)(stop-start)) * (double) 1000000), ret);

    
    init(0x0);
    omp_init_lock(&lock);
    
    /* GENERATE REFERENCE TIME */
    reference(REF1, &refer);
    
    /* TEST PARALLEL REGION */
    benchmark(PARALLEL, &testpr);
    
    /* TEST FOR */
    benchmark(FOR, &testfor);
    
    /* TEST PARALLEL FOR */
    benchmark(PARALLEL_FOR, &testpfor);
    
    /* TEST BARRIER */
    benchmark(BARRIER, &testbar);

    /* TEST SINGLE */
    benchmark(SINGLE, &testsing);
    
    /* TEST  CRITICAL*/
    benchmark(CRITICAL, &testcrit);
    
    /* TEST  LOCK/UNLOCK */
    benchmark(LOCKS, &testlock);
    
    /* GENERATE NEW REFERENCE TIME */
    reference(REF2, &referatom);
    
    /* TEST ATOMIC */
    benchmark(ATOMIC, &testatom);
    
    /* GENERATE NEW REFERENCE TIME */
    reference(REF3, &referred);
    
    /* TEST REDUCTION (1 var)  */
    benchmark(REDUCTION, &testred);
    
    finalise();
    
    return;
}

int getdelaylengthfromtime(float delaytime) {
    int i, reps;
    double lapsedtime; // seconds
    double start;

    reps = 1000;
    lapsedtime = 0.0;

    delaytime = delaytime/1.0E6; // convert from microseconds to seconds

    // Note: delaytime is local to this function and thus the conversion
    // does not propagate to the main code. 

    // Here we want to use the delaytime in microseconds to find the 
    // delaylength in iterations. We start with delaylength=0 and 
    // increase until we get a large enough delaytime, return delaylength 
    // in iterations. 

    delaylength = 0;
    delay(delaylength);

    while (lapsedtime < delaytime) {
        delaylength = delaylength * 1.1 + 1;
        start = gettime();
        for (i = 0; i < reps; i++) {
            delay(delaylength);
        }
        lapsedtime = (gettime() - start) / (double) reps;
    }
    return delaylength;
}

unsigned long getinnerreps(void (*test)(void)) {
    innerreps = 10L;  // some initial value
    double time = 0.0;
    double start;

    while (time < __targettesttime__) {
        start = gettime();
        test();
        time = (gettime() - start); 
        innerreps *=2;

        // Test to stop code if compiler is optimising reference time expressions away
        if (innerreps > (__targettesttime__*1.0e15)) {
            printf("Compiler has optimised reference loop away, STOP! \n");
            printf("Try recompiling with lower optimisation level \n");
            exit(0);
        }
    }
    return innerreps;
}

void printheader(BenchType_t type) {
    __innerreps__[(int)type] = innerreps;
    
    printf("\n");
    printf("--------------------------------------------------------\n");
    printf("Threads: %d\n", nthreads);    
    printf("Computing %s time using %lu reps\n", getName(type), __innerreps__[(int)type]);
}

void stats(BenchType_t type, float *mtp, float *sdp, unsigned long long *mcp) {

    float meantime, totaltime, sumsq, mintime, maxtime, sd, cutoff;
    unsigned long long totalcycles;
    int i, nr;

    mintime = 1.0e10;
    maxtime = 0.;
    totaltime = 0.;
    totalcycles = 0;

    for (i = 1; i <= __outerreps__; i++) {
        mintime = (mintime < (float)times[i]) ? mintime : ((float)times[i]);
        maxtime = (maxtime > (float)times[i]) ? maxtime : ((float)times[i]);
        totaltime += (float)times[i];
    }
    
    meantime = totaltime / __outerreps__;
    sumsq = 0.0;

    for (i = 1; i <= __outerreps__; i++) {
        sumsq += ( (float)times[i] - meantime ) * ( (float)times[i] - meantime);
    }
    sd = sqrt(sumsq / ((float)(__outerreps__ - 1)));
    
    cutoff = 3.0 * sd;

    nr = 0;

    for (i = 1; i <= __outerreps__; i++) {
        if ( fabs( (float)times[i] - meantime ) > cutoff ) 
            nr ++;
    }

    printf("\n");
    printf("Sample_size       Average     Min         Max          S.D.          Outliers\n");
    printf(" %d                %f   %f   %f    %f      %d\n",
        __outerreps__, meantime, mintime, maxtime, sd, nr);
    printf("\n");
    
    *mtp = meantime;
    *mcp = totalcycles / (unsigned long long)__outerreps__;
    *sdp = sd;

    __meantime__[(int)type]= meantime;
    __mintime__[(int)type] = mintime;
    __maxtime__[(int)type] = maxtime;
    __sd__[(int)type]      = sd;
    __nr__[(int)type]      = nr;
}

void printreferencefooter(BenchType_t type, float referencetime, float referencesd, unsigned long long refcycles) {    
    __referencetime__[(int)type] = referencetime;
    __referencesd__[(int)type] = referencesd;
    __refcycles__[(int)type] = refcycles;  
    
    printf("%s time     = %f microseconds +/- %f mean times = %lld\n",
           getName(type), referencetime, CONF95 * referencesd, refcycles);
}

void printfooter(BenchType_t type, float testtime, float testsd,
         float referencetime, float refsd, unsigned long long refcycles, unsigned long long meancycles){
    
    __testtime__[(int)type] = testtime;
    __referencetime__[(int)type] = referencetime;
    __testsd__[(int)type] = testsd;
    __referencesd__[(int)type] = refsd;
    __meancycles__[(int)type] = meancycles;
    __refcycles__[(int)type] = refcycles;
    
    printf("%s time     = %f microseconds +/- %f, mean times = %lld\n",
           getName(type), testtime, CONF95*testsd, meancycles);
    printf("%s overhead = %f microseconds +/- %f, mean times = %lld\n",
           getName(type), testtime-referencetime, CONF95*(testsd+referencesd), meancycles-refcycles);
}

void init(int bench)
{
    nthreads = __nthreads__;
        
    delaylength = getdelaylengthfromtime(__delaytime__); // Always need to compute delaylength in iterations 

    times = (double *)malloc(((__outerreps__)+1) * sizeof(double));
}

void finalise(void) {
    free(times);
}

void initreference(BenchType_t type) {
    printheader(type);
}

/* Calculate the reference time. */
void reference(BenchType_t type, void (*refer)(void)) {
    
    int k;
    double start;

    // Calculate the required number of innerreps
    innerreps = getinnerreps(refer);

    initreference(type);

    for (k = 0; k <= __outerreps__; k++) {
        start = gettime();
        refer();
        times[k] = ( gettime() - start ) / (unsigned long long)innerreps;
    }

    finalisereference(type);

}

void finalisereference(BenchType_t type) {
    stats(type, &referencetime, &referencesd, &refcycles);
    printreferencefooter(type, referencetime, referencesd, refcycles);

}

void intitest(BenchType_t type) {
    printheader(type);

}

void finalisetest(BenchType_t type) {
    stats(type, &testtime, &testsd, &meancycles);
    printfooter(type, testtime, testsd, referencetime, referencesd, refcycles, meancycles);

}

/* Function to run a microbenchmark test*/
void benchmark(BenchType_t type, void (*test)(void))
{
    int k;
    double start;

    // Calculate the required number of innerreps
    innerreps = getinnerreps(test);

    intitest(type);

    for (k=0; k<=__outerreps__; k++) {
        start = gettime();
        test();
        times[k] = ( gettime() - start ) / innerreps;
    }

    finalisetest(type);

}

// For the Cray compiler on HECToR we need to turn off optimisation 
// for the delay and array_delay functions. Other compilers should
// not be afffected. 
//#pragma _CRI noopt
void delay(int delaylength) {

    int i;
    float a = 0.;

    for (i = 0; i < delaylength; i++)
        a += i;
    if (a < 0)
        printf("%f \n", a);

}

void array_delay(int delaylength, float a[1]) {

    int i;
    a[0] = 1.0;
    for (i = 0; i < delaylength; i++)
        a[0] += i;
    if (a[0] < 0)
        printf("%f \n", a[0]);

}

unsigned long long getcycles()
{
    return 0;
}

int returnfalse() {
    return 0;
}

void refer() {
    int j;
    for (j = 0; j < innerreps; j++) {
        delay(delaylength);
    }
}

void referatom(){
    int j;
    float aaaa = 0.0;
    float epsilon = 1.0e-15;
    float b, c;
    b = 1.0;
    c = (1.0 + epsilon);
    for (j = 0; j < innerreps; j++) {
        aaaa += b;
        b *= c;
    }
    if (aaaa < 0.0)
        printf("%f\n", aaaa);
}

void referred() {
    int j;
    int aaaa = 0;
    for (j = 0; j < innerreps; j++) {
        delay(delaylength);
        aaaa += 1;
    }
}

void testpr() {
    int j;
    for (j = 0; j < innerreps; j++) {
        #pragma omp parallel num_threads(nthreads)
        {
            delay(delaylength);
        }
    }
}

void testfor() {
    int i, j;
    #pragma omp parallel private(j) num_threads(nthreads)
    {
        for (j = 0; j < innerreps; j++) {
            #pragma omp for
            for (i = 0; i < nthreads; i++) {
                delay(delaylength);
            }
        }
    }
}

void testpfor() {
    int i, j;
    for (j = 0; j < innerreps; j++) {
        #pragma omp parallel for num_threads(nthreads)
        for (i = 0; i < nthreads; i++) {
            delay(delaylength);
        }
    }
}

void testbar() {
    int j;
    #pragma omp parallel private(j) num_threads(nthreads)
    {
        for (j = 0; j < innerreps; j++) {
            delay(delaylength);
            #pragma omp barrier
        }
    }
}

void testsing() {
    int j;
    #pragma omp parallel private(j) num_threads(nthreads)
    {
        for (j = 0; j < innerreps; j++) {
            #pragma omp single
            delay(delaylength);
        }
    }
}

void testcrit() {
    int j;
    #pragma omp parallel private(j) num_threads(nthreads)
    {
        for (j = 0; j < innerreps / nthreads; j++) {
            #pragma omp critical
            {
                delay(delaylength);
            }
        }
    }
}

void testlock() {
    int j;
    
    #pragma omp parallel private(j) num_threads(nthreads)
    {
        for (j = 0; j < innerreps / nthreads; j++) {
            omp_set_lock(&lock);
            delay(delaylength);
            omp_unset_lock(&lock);
        }
    }
}

void testatom() {
    int j;
    float aaaa = 0.0;
    float epsilon = 1.0e-15;
    float b,c;
    b = 1.0;
    c = (1.0 + epsilon);
    #pragma omp parallel private(j) firstprivate(b) num_threads(nthreads)
    {
        for (j = 0; j < innerreps / nthreads; j++) {
            #pragma omp atomic  
            aaaa += b;
            b *= c;
        }
    }
    if (aaaa < 0.0)
        printf("%f\n", aaaa);
}

void testred() {
    int j;
    int aaaa = 0;
    for (j = 0; j < innerreps; j++) {
        #pragma omp parallel reduction(+:aaaa) num_threads(nthreads)
        {
            delay(delaylength);
            aaaa += 1;
        }
    }
}
