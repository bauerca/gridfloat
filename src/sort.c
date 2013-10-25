#include "gridfloat.h"
#include "sort.h"

#include <stdint.h>

static
void gf_sift_down(gf_struct **gfs, int start, int end,
        int (*cmp)(gf_struct *gf1, gf_struct *gf2))
{
    int child, root, node_swap;
    gf_struct *gf_swap;

    root = start;
    child = 2 * root + 1; /* Left child. */

    while (child < end) {
        //printf("%d\n", child);
        node_swap = root;
        /* Swap root for first child? */
        if (cmp(gfs[root], gfs[child]) < 0)
            node_swap = child;
        /* Swap root for second child? */
        if (child + 1 < end && cmp(gfs[node_swap], gfs[child + 1]) < 0)
            node_swap = child + 1;
        /* Require a swap? */
        if (node_swap != root) {
            gf_swap = gfs[node_swap];
            gfs[node_swap] = gfs[root];
            gfs[root] = gf_swap;
            root = node_swap;
            child = 2 * root + 1;
        } else return;
    }
}

/* Heapsort of gf_structs. */
int gf_sort(gf_struct **gfs, int len, int (*cmp)(gf_struct *gf1, gf_struct *gf2)) {
    int start, end;
    gf_struct *gf_swap;
    printf("starting sort\n");

    for (start = (len - 2) / 2; start >= 0; start--)
        gf_sift_down(gfs, start, len, cmp);

    printf("built heap\n");

    for (end = len - 1; end > 0; end--) {
        gf_swap = gfs[end];
        gfs[end] = gfs[0];
        gfs[0] = gf_swap;
        
        gf_sift_down(gfs, 0, end, cmp);
    }

    return 0;
}


/* Provide the 2D Hilbert curve sorter. */

/* Dimension for Hilbert curve */
static int64_t hn = UINT32_MAX + 1;

//rotate/flip a quadrant appropriately
static
void rot(int64_t n, int64_t *x, int64_t *y, int rx, int ry) {
    if (ry == 0) {
        if (rx == 1) {
            *x = n-1 - *x;
            *y = n-1 - *y;
        }
 
        //Swap x and y
        int64_t t  = *x;
        *x = *y;
        *y = t;
    }
}

//convert (x,y) to d
static
int64_t xy2d(int64_t x, int64_t y) {
    int rx, ry;
    int64_t s, d=0;
    for (s=hn/2; s>0; s/=2) {
        rx = (x & s) > 0;
        ry = (y & s) > 0;
        d += s * s * ((3 * rx) ^ ry);
        rot(s, &x, &y, rx, ry);
    }
    return d;
}
 
//convert d to (x,y)
static
void d2xy(int64_t d, int64_t *x, int64_t *y) {
    int rx, ry;
    int64_t s, t=d;
    *x = *y = 0;
    for (s=1; s<hn; s*=2) {
        rx = (int)(1 & (t/2));
        ry = (int)(1 & (t ^ rx));
        rot(s, x, y, rx, ry);
        *x += s * rx;
        *y += s * ry;
        t /= 4;
    }
}
 

int hilbert_cmp(gf_struct *gf1, gf_struct *gf2) {
    int64_t x1, x2, y1, y2, d1, d2;
    double max = (double)UINT32_MAX;

    x1 = (int64_t)((0.5 * (gf1->grid.left + gf1->grid.right) + 180.0) * (max / 360.0));
    x2 = (int64_t)((0.5 * (gf2->grid.left + gf2->grid.right) + 180.0) * (max / 360.0));
    y1 = (int64_t)((0.5 * (gf1->grid.top + gf1->grid.bottom) + 90.0) * (max / 180.0));
    y2 = (int64_t)((0.5 * (gf2->grid.top + gf2->grid.bottom) + 90.0) * (max / 180.0));

    d1 = xy2d(x1, y1);
    d2 = xy2d(x2, y2);

    if (d1 < d2) return -1;
    else if (d1 == d2) return 0;
    else return 1;
}
