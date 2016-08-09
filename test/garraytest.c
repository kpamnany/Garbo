#include <stdio.h>
#include <stdlib.h>
#include "garbo.h"

#define NUM_ELEMENTS    25

garbo_t *g;

int main(int argc, char **argv)
{
    int64_t dim1[1] = { NUM_ELEMENTS }, lo[1], hi[1], *val;
    int64_t nid, nnodes;
    garray_t *ga;

    garbo_init(argc, argv, &g);
    nid = garbo_nodeid();
    nnodes = garbo_nnodes();
    if (nid == 0)
        printf("garbotest -- %ld nodes\n", nnodes);

    garray_create(g, 1, dim1, sizeof(int), NULL, &ga);

    lo[0] = hi[0] = nid;
    garray_access(ga, lo, hi, (void **)&val);
    val[0] = nid+1;

    garray_sync(ga);

    val = malloc(NUM_ELEMENTS * sizeof(int));
    for (int i = 0;  i < NUM_ELEMENTS;  ++i)
        val[i] = -1;

    lo[0] = hi[0] = (nid + 1) % NUM_ELEMENTS;
    printf("[%ld] getting %ld-%ld\n", nid, lo[0], hi[0]);
    garray_get(ga, lo, hi, val);
    printf("[%ld] got %ld\n", nid, val[0]);

    free(val);
    garray_destroy(ga);
    garbo_shutdown(g);

    return 0;
}

