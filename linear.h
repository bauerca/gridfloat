#ifndef LINEAR_H
#define LINEAR_H

#include "gridfloat.h"

/**
 * The workhorse. Iterates over each point on a subgrid,
 * finds the 4 nearest points from the GridFloat dataset,
 * and calls a callback to do something with those 4 values.
 */
int grid_float_bilinear(
    const struct grid_float *gf,
    const struct gf_grid *grid,
    void *set_data_xtras,
    int (*set_data)(
        gf_float *quad,
        double cellsize,
        double *weights,
        double *latlng,
        void *xtras,
        void **data_ptr    /* Pointer to the current position in the buffer.
                              Position needs to be incremented by the callback. */
    ),
    int (*set_null)(void **data_ptr),
    void *data
);

int grid_float_bilinear_interpolate_kernel(gf_float *quad, double cellsize, double *w, double *latlng, void *xtras, void **data_ptr);

int grid_float_bilinear_interpolate(const struct grid_float *gf, const struct gf_grid *grid, gf_float *data);

int grid_float_bilinear_gradient_kernel(gf_float *quad, double cellsize, double *w, double *latlng, void *xtras, void **data_ptr);

int grid_float_bilinear_gradient(const struct grid_float *gf, const struct gf_grid *grid, double *gradient);

#endif
