#ifndef GF_PNG_H
#define GF_PNG_H

#include <png.h>

#include "gridfloat.h"

int gf_relief_shade(
    const struct grid_float *gf,
    const struct gf_grid *grid,
    double *n_sun,
    const char *filename);

int grid_float_png(int nx, int ny, png_byte **data, const char *filename);

#endif
