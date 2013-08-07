#ifndef GRID_FLOAT_H
#define GRID_FLOAT_H

#include <stdio.h>

#define EQ_RADIUS 6378137. // meters
#define PI 3.14159265

#define GF_NULL_VAL -9999.0

typedef float gf_float;

struct gf_bounds {
    double left;
    double right;
    double bottom;
    double top;
};

struct grid_float_hdr {
    long nx;
    long ny;
    struct gf_bounds bounds;
    double cellsize;
    gf_float null_value;
    char byte_order[64];
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

int gf_get_line(long ii, long jj_start, long jj_end, const struct grid_float *gf, gf_float *line);

void grid_float_print(const struct gf_grid *grid, gf_float *data, int xy);

#endif
