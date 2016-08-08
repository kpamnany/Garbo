/*  garbo

    global array toolbox and distributed dynamic scheduler
*/

#ifndef _GARBO_H
#define _GARBO_H

#include <mpi.h>
#include <stdint.h>
#include "util.h"
#include "garray_debug.h"
#include "dtree_debug.h"

/* garbo handle */
typedef struct garbo_tag {
    int nnodes, nid;
} garbo_t;


/* global array */
typedef struct garray_tag {
    garbo_t     *g;
    int         ndims, *dims, *chunks, elem_size, nextra_elems, nelems_per_node,
                nlocal_elems;
    int8_t      *buffer;
    MPI_Win     win;

#ifdef PROFILE_GARRAY
    garray_thread_timing_t **times;
#endif
} garray_t;


/* dtree */
typedef struct dtree_tag {
    garbo_t             *g;

    /* tree structure */
    int8_t              num_levels, my_level;
    int                 parent, *children, num_children;
    double              tot_children;

    /* MPI info */
    int                 my_rank, num_ranks;
    int16_t             *children_req_bufs;
    MPI_Request         parent_req, *children_reqs;

    /* work distribution policy */
    int16_t             parents_work;
    double              first, rest;
    double              *distrib_fractions;
    int16_t             min_distrib;

    /* work items */
    int64_t             first_work_item, last_work_item, next_work_item;
    int64_t volatile    work_lock __attribute((aligned(8)));

    /* for heterogeneous clusters */
    double              node_mul;

    /* concurrent calls in from how many threads? */
    int                 num_threads;
    int                 (*threadid)();
#ifdef PROFILE_DTREE
    dtree_thread_timing_t **times;
#endif

} dtree_t;


/* garbo interface
 */

int garbo_init(int ac, char **av, garbo_t **g);
int garbo_shutdown(garbo_t *g);

/* number of participating nodes */
int garbo_nnodes();

/* this node's unique identifier */
int garbo_nodeid();

/* global array */
int garray_create(garbo_t *g, int ndims, int *dims, int elem_size, int *chunks,
        garray_t *ga);
int garray_destroy(garray_t *ga);

int garray_sync(garray_t *ga);

int garray_get(garray_t *ga, int *lo, int *hi, void *buf, int *ld);
int garray_put(garray_t *ga, int *lo, int *hi, void *buf, int *ld);

int garray_distribution(garray_t *ga, int nid, int *lo, int *hi);
int garray_access(garray_t *ga, int *lo, int *hi, void **buf, int *ld);

/* dtree */
int dtree_create(garbo_t *g, int fan_out, int64_t num_work_items,
        int can_parent, int parents_work, double node_mul,
        int num_threads, int (*threadid)(),
        double first, double rest, int16_t min_distrib, dtree_t **dt, int *is_parent);
void dtree_destroy(dtree_t *dt);

/* call to get initial work allocation; before dtree_run() is called */
int64_t dtree_initwork(dtree_t *dt, int64_t *first_item, int64_t *last_item);

/* get a block of work */
int64_t dtree_getwork(dtree_t *dt, int64_t *first_item, int64_t *last_item);

/* call from a thread repeatedly until it returns 0 */
int dtree_run(dtree_t *dt);

/* utility helpers */
uint64_t rdtsc();
void cpu_pause();

#endif  /* _GARBO_H */

