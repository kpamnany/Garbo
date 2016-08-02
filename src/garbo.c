/* garbo -- global array toolbox

 */


#include <mpi.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <immintrin.h>

#include "garbo.h"


#ifndef UINT64_MAX
#define UINT64_MAX      18446744073709551615ULL
#endif


#ifdef PROFILE_GARBO

#define PROFILE_START(dt,w)                                                             \
    (dt)->times[(*(dt)->threadid)()][(w)].last = _rdtsc()

#define PROFILE_STAMP(dt,w)                                                             \
{                                                                                       \
    int t = (*(dt)->threadid)();                                                        \
    uint64_t l = (dt)->times[t][(w)].last = _rdtsc() - (dt)->times[t][(w)].last;        \
    if (l < (dt)->times[t][(w)].min) (dt)->times[t][(w)].min = l;                       \
    if (l > (dt)->times[t][(w)].max) (dt)->times[t][(w)].max = l;                       \
    (dt)->times[t][(w)].total += l;                                                     \
    ++(dt)->times[t][(w)].count;                                                        \
}

#else  /* !PROFILE_GARBO */

#define PROFILE_START(dt,w)
#define PROFILE_STAMP(dt,w)

#endif /* PROFILE_GARBO */


#ifdef PROFILE_GARBO

/*  parse_csv_str() -- parses a string of comma-separated numbers and returns
        an array containing the numbers in *vals, and the number of elements
        of the array. *vals must be freed by the caller!
 */
static int parse_csv_str(char *s, int **vals)
{
    int i, num = 0, *v;
    char *cp, *np;

    *vals = NULL;
    if (!s) return 0;

    cp = s;
    while (cp) {
        ++num;
        cp = strchr(cp, ',');
        if (cp) ++cp;
    }

    v = (int *)calloc(num, sizeof (int));

    cp = s;
    i = 0;
    while (cp) {
        np = strchr(cp, ',');
        if (np) *np++ = '\0';
        v[i++] = (int)strtol(cp, NULL, 10);
        cp = np;
    }

    *vals = v;
    return num;
}

#endif /* PROFILE_GARBO */


/*  garbo_init()
 */
int garbo_init(int ac, char **av, garbo_t **g_)
{
    int r = MPI_Init(&ac, &av);
    if (r != MPI_SUCCESS)
        return -1;

    //garbo_t *g = aligned_alloc(64, sizeof(garbo_t));
    garbo_t *g;
    posix_memalign((void **)&g, 64, sizeof(garbo_t));
    g->nnodes = garbo_nnodes();
    g->nid = garbo_nodeid();
    *g_ = g;

    return 0;
}


/*  garbo_shutdown()
 */
int garbo_shutdown(garbo_t *g)
{
    MPI_Finalize();
    free(g);

    return 0;
}


/*  garbo_create()
 */
int garbo_create(garbo_t *g, int ndims, int *dims, int elem_size, int *chunks,
        garray_t *ga)
{
    /* only 1D for now */
    assert(ndims == 1);

    /* only regular distribution for now */
    assert(chunks == NULL);

    /* distribution of elements */
    div_t res = div(dims[0], g->nnodes);
    ga->nextra_elems = res.rem;
    ga->nelems_per_node = ga->nlocal_elems = res.quot;
    if (g->nid < ga->nextra_elems)
        ++ga->nlocal_elems;

    /* every node must have at least one element */
    assert(res.quot > 0);

    /* fill in array info */
    ga->ndims = ndims;
    ga->dims = (int *)malloc(ndims * sizeof(int));
    for (int i = 0;  i < ndims;  ++i)
        ga->dims[i] = dims[i];
    ga->elem_size = elem_size;

    /* allocate the array */
    MPI_Win_allocate(ga->nlocal_elems*elem_size, 1, MPI_INFO_NULL,
            MPI_COMM_WORLD, &ga->buffer, &ga->win);
    MPI_Win_lock_all(MPI_MODE_NOCHECK, ga->win);

    return 0;
}


/*  garbo_destroy()
 */
int garbo_destroy(garray_t *ga)
{
    MPI_Win_unlock_all(ga->win);
    MPI_Win_free(&ga->win);

    return 0;
}


/*  garbo_sync()
 */
int garbo_sync(garray_t *ga)
{
    MPI_Win_flush_all(ga->win);
    MPI_Barrier(MPI_COMM_WORLD);

    return 0;
}


/*  calc_get_target()
 */
static div_t calc_get_target(garray_t *ga, int gidx)
{
    div_t res = div(gidx, ga->nlocal_elems);

    /* if the distribution is not perfectly even, we have to adjust
       the target rank+idx appropriately */
    if (ga->nextra_elems > 0) {
        int rank = res.quot, idx = res.rem;

        /* if this rank has an extra element; the target might need
           adjusting up */
        if (ga->g->nid < ga->nextra_elems) {
            if (rank >= ga->nextra_elems) {
                idx += (rank - ga->nextra_elems);
                if (idx >= (ga->nlocal_elems-1)) {
                    idx -= (ga->nlocal_elems-1);
                    ++rank;
                }
            }
        }

        /* this rank does not have an extra element; the target will
           need adjusting */
        else {
            idx -= (rank < ga->nextra_elems ? rank : ga->nextra_elems);
            if (idx < 0) {
                idx += (ga->nlocal_elems + (rank <= ga->nlocal_elems ? 1 : 0));
                --rank;
            }
        }

        res.quot = rank; res.rem = idx;
    }

    return res;
}


/*  garbo_get()
 */
int garbo_get(garray_t *ga, int *lo, int *hi, void *buffer, int *ld)
{
    int count = (hi[0]-lo[0])+1, length = count*ga->elem_size,
        trank, tidx, n, oidx = 0;
    int8_t *buf = (int8_t *)buffer;
    div_t target_lo = calc_get_target(ga, lo[0]);
    div_t target_hi = calc_get_target(ga, hi[0]);

    /* is all requested data on the same target? */
    if (target_lo.quot == target_hi.quot) {
        MPI_Get(buf, length, MPI_INT8_T, target_lo.quot,
                (target_lo.rem*ga->elem_size),
                length, MPI_INT8_T, ga->win);
        MPI_Win_flush_local(target_lo.quot, ga->win);

        return 0;
    }

    /* get the data in the lo rank */
    trank = target_lo.quot;
    tidx = target_lo.rem;
    n = ga->nelems_per_node - tidx + (trank < ga->nextra_elems ? 1 : 0);
    MPI_Get(buf, length, MPI_INT8_T, trank, (tidx*ga->elem_size),
            (n*ga->elem_size), MPI_INT8_T, ga->win);
    oidx = (n*ga->elem_size);
    MPI_Win_flush_local(trank, ga->win);

    /* get the data in the in-between ranks */
    tidx = 0;
    for (trank = target_lo.quot+1;  trank < target_hi.quot;  ++trank) {
        n = ga->nelems_per_node + (trank < ga->nextra_elems ? 1 : 0);
        MPI_Get(&buf[oidx], length, MPI_INT8_T, trank, 0,
                (n*ga->elem_size), MPI_INT8_T, ga->win);
        oidx += (n*ga->elem_size);
        MPI_Win_flush_local(trank, ga->win);
    }

    /* get the data in the hi rank */
    trank = target_hi.quot;
    tidx = target_hi.rem;
    n = ga->nelems_per_node - target_hi.rem
            + (target_hi.quot < ga->nextra_elems ? 1 : 0);
    MPI_Get(&buf[oidx], length, MPI_INT8_T, target_hi.quot,
            (target_hi.rem*ga->elem_size), (n*ga->elem_size),
            MPI_INT8_T, ga->win);
    MPI_Win_flush_local(target_hi.quot, ga->win);

    return 0;
}


/*  garbo_put()
 */
int garbo_put(garray_t *ga, int *lo, int *hi, void *buf, int *ld)
{
    assert(0);

    return 0;
}


/*  garbo_access()
 */
int garbo_access(garray_t *ga, int *lo, int *hi, void **buf, int *ld)
{
    *buf = ga->buffer;

    return 0;
}


/*  garbo_nnodes()
 */
int garbo_nnodes()
{
    int num_ranks;
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
    return num_ranks;
}


/*  garbo_nodeid()
 */
int garbo_nodeid()
{
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    return my_rank;
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

