#ifndef GRID_FLOAT_H
#define GRID_FLOAT_H

#include <stdio.h>

#define EQ_RADIUS 6378137. // meters
#define PI 3.14159265

#define GF_NULL_VAL -9999.0

typedef float gf_float;

struct grid_float_hdr {
    long nx;
    long ny;
    double left;
    double right;
    double top;
    double bottom;
    double cellsize;
    gf_float null_value;
    char byte_order[64];
};

struct gf_bounds {
    double left;
    double right;
    double bottom;
    double top;
};

struct gf_grid {
    int nx;
    int ny;
    struct gf_bounds bounds;
};

struct grid_float {
    struct grid_float_hdr hdr;
    FILE *flt;         /* Descriptor for .flt file */
};

void grid_float_lengths(double lat, double lng, double dlat, double dlng, double ecc, double *dx, double *dy);

int grid_float_parse_hdr(const char *hdr_file, struct grid_float_hdr *hdr);

int grid_float_open(const char *hdr_file, const char *flt_file, struct grid_float *gf);

void grid_float_close(struct grid_float *gf);

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

int grid_float_print(const struct gf_grid *grid, gf_float *data, int xy);

#endif
