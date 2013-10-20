#ifndef GF_PNG_H
#define GF_PNG_H

#include <png.h>

#include "gridfloat.h"

int gf_relief_shade_kernel(
    gf_float nine[][3],
    const gf_grid *from_grid,
    double *w, double *latlng, void *xtras, void **data_ptr);

void gf_relief_shade(
    const gf_struct *gf,
    const gf_grid *to_grid,
    double *n_sun,
    const char *filename);

int gf_save_png(int nx, int ny, png_byte **data, const char *filename);

#endif
