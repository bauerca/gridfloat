#ifndef GRID_FLOAT_H
#define GRID_FLOAT_H

#include <stdio.h>

#define EQ_RADIUS 6378137. // meters
#define PI 3.14159265

#define GF_NULL_VAL -9999.0

typedef float gf_float;

typedef struct gf_grid {
    int nx;
    int ny;
    double dx;
    double dy;
    double left;
    double right;
    double bottom;
    double top;
} gf_grid;

typedef struct gf_struct {
    gf_grid grid;
    gf_float null_value;
    char byte_order[64];
    FILE *flt;         /* Descriptor for .flt file */
} gf_struct;

void gf_init_grid_point(gf_grid *grid, double lat, double lng, double width, double height, int nlat, int nlng);

void gf_init_grid_bounds(gf_grid *grid, double left, double right, double bottom, double top, int nlat, int nlng);

void gf_lengths(double lat, double lng, double dlat, double dlng, double ecc, double *dy, double *dx);

int gf_parse_hdr(const char *hdr_file, gf_struct *gf);

int gf_open(const char *hdr_file, const char *flt_file, gf_struct *gf);

void gf_close(gf_struct *gf);

int gf_get_line(long ii, long jj_start, long jj_end, const gf_struct *gf, gf_float *line);

void gf_print(const gf_grid *grid, gf_float *data, int xy);

void gf_save(gf_grid *grid, gf_float *data, const char *prefix);

void gf_print_grid_info(gf_grid *grid);

#endif
