#ifndef GRID_FLOAT_H
#define GRID_FLOAT_H

#include <stdio.h>

#define EQ_RADIUS 6378137. // meters
#define PI 3.14159265

#define GF_NULL_VAL -9999.0

typedef float gf_float;

/**
 * Grid
 *
 * A grid of lat/lng points. The x-direction points along lines
 * of latitude; y along lines of longitude.
 *
 * @nx - Resolution along lines of latitude.
 * @ny - Resolution along lines of longitude.
 * @dx - Cell width in x.
 * @dy - Cell width in y.
 * @left - Longitude of left-most grid points (smallest
 *   in x). In the western hemisphere, this value is negative.
 * @right - Longitude of right-most grid points.
 * @bottom - Latitude of bottom-most grid points (smallest
 *   y-values).
 * @top - Latitude of top-most grid points.
 *
 */ 
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

/**
 * The master struct. Represents a gridfloat data/header
 * pair. Holds (most importantly) a grid and a pointer
 * to an open .flt file. Use gf_open(...) to initialize
 * the struct.
 *
 */
typedef struct gf_struct {
    gf_grid grid;
    gf_float null_value;
    char byte_order[64];
    FILE *flt;         /* Descriptor for .flt file */
} gf_struct;

/**
 * Create a grid based on a lat/lng point and a width/height
 * pair given in degrees. Width is along lines of latitude, height
 * is along lines of longitude.
 */
void gf_init_grid_point(gf_grid *grid, double lat, double lng, double width, double height, int nlat, int nlng);

void gf_init_grid_bounds(gf_grid *grid, double left, double right, double bottom, double top, int nlat, int nlng);

void gf_lengths(double lat, double lng, double dlat, double dlng, double ecc, double *dy, double *dx);

void gf_cellsize_meters(gf_grid *grid, double *dxm, double *dym);

int gf_parse_hdr(const char *hdr_file, gf_struct *gf);

int gf_open(const char *hdr_file, const char *flt_file, gf_struct *gf);

void gf_close(gf_struct *gf);

int gf_get_line(long ii, long jj_start, long jj_end, const gf_struct *gf, gf_float *line);

void gf_print(const gf_grid *grid, gf_float *data, int xy);

void gf_save(gf_grid *grid, gf_float *data, const char *prefix);

void gf_print_grid_info(gf_grid *grid);



#endif
