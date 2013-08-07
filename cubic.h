#ifndef CUBIC_H
#define CUBIC_H

#include "gridfloat.h"


int gf_bicubic(
    const struct grid_float *gf,
    const struct gf_grid *grid,
    void *set_data_xtras,
    int (*set_data)(
        gf_float nine[][3],
        double cellsize,
        double *weights,
        double *latlng,
        void *xtras,
        void **data_ptr    /* Pointer to the current position in the buffer.
                              Position needs to be incremented by the callback. */
    ),
    int (*set_null)(void **),
    void *data
);

int gf_bicubic_gradient_kernel(gf_float nine[][3], double cellsize, double *w, double *latlng, void *xtras, void **data_ptr);

int gf_bicubic_gradient(const struct grid_float *gf, const struct gf_grid *grid, double *gradient);

#endif
