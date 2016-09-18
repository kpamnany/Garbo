/* garbo

 */


#include <mpi.h>
#include <stdint.h>
#include <stdlib.h>
#include <immintrin.h>

#include "garbo.h"


/*  garbo_init()
 */
int64_t garbo_init(int ac, char **av, garbo_t **g_)
{
    int provided;
    int r = MPI_Init_thread(&ac, &av, MPI_THREAD_MULTIPLE, &provided);
    if (r != MPI_SUCCESS)
        return -1;

    //garbo_t *g = aligned_alloc(64, sizeof(garbo_t));
    garbo_t *g;
    posix_memalign((void **)&g, 64, sizeof(garbo_t));
    g->nnodes = garbo_nnodes();
    g->nid = garbo_nodeid();
    log_init(&g->glog, "GARRAY_LOG_LEVEL");
    log_init(&g->dlog, "DTREE_LOG_LEVEL");
    *g_ = g;

    return 0;
}


/*  garbo_shutdown()
 */
void garbo_shutdown(garbo_t *g)
{
    MPI_Finalize();
    free(g);
}


/*  garbo_nnodes()
 */
int64_t garbo_nnodes()
{
    int num_ranks;
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
    return num_ranks;
}


/*  garbo_nodeid()
 */
int64_t garbo_nodeid()
{
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    return my_rank;
}


/*  garbo_sync_all()
 */
void garbo_sync()
{
    MPI_Barrier(MPI_COMM_WORLD);
}


/*  rdtsc()
 */
inline uint64_t __attribute__((always_inline)) rdtsc()
{
    uint32_t hi, lo;
    __asm__ __volatile__(
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo), "=d"(hi)
        : /* no inputs */
        : "rbx", "rcx");
    return ((uint64_t)hi << 32ull) | (uint64_t)lo;
}


/*  cpu_pause()
 */
inline void __attribute__((always_inline)) cpu_pause()
{
    _mm_pause();
}

