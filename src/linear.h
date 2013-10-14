#ifndef LINEAR_H
#define LINEAR_H

#include "gridfloat.h"

typedef int (gf_bilinear_kernel)(gf_float *quad,
    const gf_grid *from_grid,
    double *weights,
    double *latlng,
    void *xtras,
    void **data_ptr
);

/**
 * The workhorse. Iterates over each point on a subgrid,
 * finds the 4 nearest points from the GridFloat dataset,
 * and calls a callback to do something with those 4 values.
 */
int gf_bilinear(
    const gf_struct *gf,
    const gf_grid *grid,
    void *set_data_xtras,
    gf_bilinear_kernel *set_data,
//    int (*set_data)(
//        gf_float *quad,
//        const gf_grid *from_grid,
//        double *weights,
//        double *latlng,
//        void *xtras,
//        void **data_ptr    /* Pointer to the current position in the buffer.
//                              Position needs to be incremented by the callback. */
//    ),
    int (*set_null)(void **data_ptr),
    void *data
);

int gf_bilinear_interpolate_kernel(gf_float *quad, const gf_grid *from_grid, double *w, double *latlng, void *xtras, void **data_ptr);

int gf_bilinear_interpolate(const gf_struct *gf, const gf_grid *to_grid, gf_float *data);

int gf_bilinear_gradient_kernel(gf_float *quad, const gf_grid *from_grid, double *w, double *latlng, void *xtras, void **data_ptr);

int gf_bilinear_gradient(const gf_struct *gf, const gf_grid *to_grid, double *gradient);

#endif
