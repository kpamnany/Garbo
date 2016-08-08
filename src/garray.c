/* garbo

   global arrays

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


/*  garray_create()
 */
int garray_create(garbo_t *g, int64_t ndims, int64_t *dims, int64_t elem_size,
        int64_t *chunks, garray_t **ga_)
{
    /* only 1D for now */
    assert(ndims == 1);

    /* only regular distribution for now */
    assert(chunks == NULL);

    garray_t *ga;
    posix_memalign((void **)&ga, 64, sizeof(garray_t));

    ga->g = g;

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
    ga->dims = (int64_t *)malloc(ndims * sizeof(int64_t));
    for (int i = 0;  i < ndims;  ++i)
        ga->dims[i] = dims[i];
    ga->elem_size = elem_size;

    /* allocate the array */
    MPI_Win_allocate(ga->nlocal_elems*elem_size, 1, MPI_INFO_NULL,
            MPI_COMM_WORLD, &ga->buffer, &ga->win);
    MPI_Win_lock_all(MPI_MODE_NOCHECK, ga->win);

    *ga_ = ga;

    return 0;
}


/*  garray_destroy()
 */
void garray_destroy(garray_t *ga)
{
    free(ga->dims);
    MPI_Win_unlock_all(ga->win);
    MPI_Win_free(&ga->win);
    free(ga);
}


/*  garray_sync()
 */
void garray_sync(garray_t *ga)
{
    MPI_Win_flush_all(ga->win);
    MPI_Barrier(MPI_COMM_WORLD);
}


/*  calc_get_target()
 */
static div_t calc_get_target(garray_t *ga, int gidx)
{
    div_t res = div(gidx, ga->nlocal_elems);

    /* if the distribution is not perfectly even, we have to adjust
       the target nid+idx appropriately */
    if (ga->nextra_elems > 0) {
        int nid = res.quot, idx = res.rem;

        /* if i have an extra element... */
        if (ga->g->nid < ga->nextra_elems) {
            /* but the target does not */
            if (nid >= ga->nextra_elems) {
                /* then the target index has to be adjusted upwards */
                idx += (nid - ga->nextra_elems);
                /* which may mean that the target nid does too */
                if (idx >= (ga->nlocal_elems-1)) {
                    ++nid;
                    idx -= (ga->nlocal_elems-1);
                }
            }
        }

        /* i don't have an extra element... */
        else {
            /* so adjust the target index downwards */
            idx -= (nid < ga->nextra_elems ? nid : ga->nextra_elems);
            /* which may mean the target nid has to be adjusted too */
            while (idx < 0) {
                --nid;
                idx += ga->nelems_per_node + (nid < ga->nextra_elems ? 1 : 0);
            }
        }

        res.quot = nid; res.rem = idx;
    }

    return res;
}


/*  garray_get()
 */
int garray_get(garray_t *ga, int64_t *lo, int64_t *hi, void *buffer)
{
    int count = (hi[0]-lo[0])+1, length = count*ga->elem_size,
        tnid, tidx, n, oidx = 0;
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

    /* get the data in the lo nid */
    tnid = target_lo.quot;
    tidx = target_lo.rem;
    n = ga->nelems_per_node - tidx + (tnid < ga->nextra_elems ? 1 : 0);
    MPI_Get(buf, length, MPI_INT8_T, tnid, (tidx*ga->elem_size),
            (n*ga->elem_size), MPI_INT8_T, ga->win);
    oidx = (n*ga->elem_size);
    MPI_Win_flush_local(tnid, ga->win);

    /* get the data in the in-between nids */
    tidx = 0;
    for (tnid = target_lo.quot+1;  tnid < target_hi.quot;  ++tnid) {
        n = ga->nelems_per_node + (tnid < ga->nextra_elems ? 1 : 0);
        MPI_Get(&buf[oidx], length, MPI_INT8_T, tnid, 0,
                (n*ga->elem_size), MPI_INT8_T, ga->win);
        oidx += (n*ga->elem_size);
        MPI_Win_flush_local(tnid, ga->win);
    }

    /* get the data in the hi nid */
    tnid = target_hi.quot;
    tidx = target_hi.rem;
    n = ga->nelems_per_node - target_hi.rem
            + (target_hi.quot < ga->nextra_elems ? 1 : 0);
    MPI_Get(&buf[oidx], length, MPI_INT8_T, target_hi.quot,
            (target_hi.rem*ga->elem_size), (n*ga->elem_size),
            MPI_INT8_T, ga->win);
    MPI_Win_flush_local(target_hi.quot, ga->win);

    return 0;
}


/*  garray_put()
 */
int garray_put(garray_t *ga, int64_t *lo, int64_t *hi, void *buf)
{
    assert(0);

    return 0;
}


/*  garray_distribution()
 */
int garray_distribution(garray_t *ga, int64_t nid, int64_t *lo, int64_t *hi)
{
    if (nid >= ga->g->nnodes)
        return -1;

    int a1, a2;
    if (nid < ga->nextra_elems) {
        a1 = nid;
        a2 = 1;
    }
    else {
        a1 = ga->nextra_elems;
        a2 = 0;
    }
    lo[0] = nid * ga->nelems_per_node + a1;
    hi[0] = lo[0] + ga->nelems_per_node + a2 - 1; /* inclusive */

    return 0;
}


/*  garray_access()
 */
int garray_access(garray_t *ga, int64_t *lo, int64_t *hi, void **buf)
{
    /* TODO: validate lo and hi */

    *buf = ga->buffer;

    return 0;
}

