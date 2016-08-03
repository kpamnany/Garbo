/*  garbo -- global array toolbox
 *
 */

#ifndef _GARBO_H
#define _GARBO_H

#include <mpi.h>
#include <stdint.h>


/*  test-and-test-and-set lock macros
 */
#define lock_init(l)                    \
    (l) = 0

#define lock_acquire(l)                 \
    do {                                \
        while ((l))                     \
            cpupause();                 \
    } while (__sync_lock_test_and_set(&(l), 1))
#define lock_release(l)                 \
    (l) = 0


/*  thread yield and delay
 */
#if (__MIC__)
# define cpupause()     _mm_delay_64(100)
//# define waitcycles(c)  _mm_delay_64((c))
# define waitcycles(c) {                \
      uint64_t s=_rdtsc();              \
      while ((_rdtsc()-s)<(c))          \
          _mm_delay_64(100);            \
  }
#else
# define cpupause()     _mm_pause()
# define waitcycles(c) {                \
      uint64_t s=_rdtsc();              \
      while ((_rdtsc()-s)<(c))          \
          _mm_pause();                  \
  }
#endif


/*  debugging and profiling
 */
#ifdef DEBUG_GARBO
#ifndef TRACE_GARBO
#define TRACE_GARBO        1
#endif
#endif

#ifdef TRACE_GARBO

#if TRACE_GARBO == 1
#define TRACE(g,x...)           \
    if ((g)->my_rank == 0 || (g)->my_rank == (g)->num_ranks-1)  \
        fprintf(stderr, x)
#elif TRACE_GARBO == 2
#define TRACE(g,x...)           \
    if ((g)->my_rank < 18 || (g)->my_rank > (g)->num_ranks-19)  \
        fprintf(stderr, x)
#elif TRACE_GARBO == 3
#define TRACE(g,x...)           \
    fprintf(stderr, x)
#else
#define TRACE(g,x...)
#endif

#else

#define TRACE(x...)

#endif

#ifdef PROFILE_GARBO
enum {
    TIME_GET, TIME_PUT, TIME_SYNC, NTIMES
};

char *times_names[] = {
    "get", "put", "sync", ""
};

typedef struct thread_timing_tag {
    uint64_t last, min, max, total, count;
} thread_timing_t;
#endif


/* garbo handle, will contain OFI stuff */
typedef struct garbo_tag {
    int nnodes, nid;

#ifdef PROFILE_GARBO
    thread_timing_t     **times;
#endif
} garbo_t;


/* global array */
typedef struct garray_tag {
    garbo_t *g;
    int ndims, *dims, *chunks, elem_size, nextra_elems, nelems_per_node,
        nlocal_elems;
    int8_t *buffer;
    MPI_Win win;
} garray_t;


/* garbo interface
 */
int garbo_init(int ac, char **av, garbo_t **g);
int garbo_shutdown(garbo_t *g);
int garbo_create(garbo_t *g, int ndims, int *dims, int elem_size, int *chunks,
        garray_t *ga);
int garbo_destroy(garray_t *ga);
int garbo_sync(garray_t *ga);
int garbo_get(garray_t *ga, int *lo, int *hi, void *buf, int *ld);
int garbo_put(garray_t *ga, int *lo, int *hi, void *buf, int *ld);
int garbo_distribution(garray_t *ga, int nid, int *lo, int *hi);
int garbo_access(garray_t *ga, int *lo, int *hi, void **buf, int *ld);

/* number of participating nodes */
int     garbo_nnodes();

/* this node's unique identifier */
int     garbo_nodeid();

/* utility helpers */
uint64_t rdtsc();
void     cpu_pause();

#endif  /* _GARBO_H */

