#ifndef MPPA_OMP_H
#define MPPA_OMP_H

#ifdef __cplusplus
extern "C" {
#endif

    // FIXME - Check exact values
    enum mppa_omp_datum_dir {
        OMP_TO_PARAM = 0,
        OMP_FROM_PARAM = 1,
        OMP_TOFROM_PARAM = 2,
    };

    typedef struct mppa_omp_datum {

        void *ptr;
        unsigned long size;
        char kind;

    } mppa_omp_datum_t;

    typedef struct mppa_omp_data {
        int n;
        mppa_omp_datum_t data[]; // C99-flexible member
    } mppa_omp_data_t;

    void GOMP_init(int cluster_id);
    void GOMP_deinit(int cluster_id);
    void GOMP_target(int cluster_id,
            void (*hostfn)(void*),
            const void *offload_id,
            char *offload_data);

#ifdef __cplusplus
}
#endif

#endif // MPPA_OMP_H
