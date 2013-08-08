#include "gridfloat.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

const int LINE_BUF = 256;
const char * TOKEN_DELIM = " \t";

void gf_init_grid_point(gf_grid *grid, double lat, double lng, double width, double height, int nlat, int nlng) {
    grid->ny = nlat;
    grid->nx = nlng;

    grid->left = lng - 0.5 * width;
    grid->right = lng + 0.5 * width;
    grid->bottom = lat - 0.5 * height;
    grid->top = lat + 0.5 * height;

    grid->dx = (grid->right - grid->left) / ((double)(grid->nx - 1));
    grid->dy = (grid->top - grid->bottom) / ((double)(grid->ny - 1));
}

void gf_init_grid_bounds(gf_grid *grid, double left, double right, double bottom, double top, int nlat, int nlng) {
    gf_init_grid_point(grid, 0.5 * (top + bottom), 0.5 * (left + right), right - left, top - bottom, nlat, nlng);
}

void gf_lengths(double lat, double lng, double dlat, double dlng, double ecc, double *dx, double *dy) {
    /*
    Returns (dx, dy) for (dlng, dlat) at (lng, lat). The
    default eccentricity is zero which is what Bing and Google
    use. Real scientists use e^2 ~ 0.006.

    lat : latitude at which to perform projection
    lng : longitude at which to perform projection
    dlat : azimuthal distance in WGS84 degrees
    dlng : azimuthal distance in WGS84 degrees
    e : eccentricity of Earth. Sphere is zero.
    */

    double theta, dtheta, dphi;
    
    theta = PI * lat / 180.0;
    dtheta = PI * dlat / 180.0;
    dphi = PI * dlng / 180.0;

    if (ecc == 0.0) {
        *dy = dtheta * EQ_RADIUS;
        *dx = dphi * EQ_RADIUS / cos(theta);
    } else {
        *dy = dtheta * EQ_RADIUS;
        *dx = dphi * EQ_RADIUS * sqrt(1.0 - (ecc * sin(theta)) * (ecc * sin(theta))) / cos(theta);
    }
}

void gf_cellsize_meters(gf_grid *grid, double *dxm, double *dym) {
    double lat, lng;

    lat = 0.5 * (grid->top + grid->bottom);
    lng = 0.5 * (grid->left + grid->right);

    gf_lengths(lat, lng, grid->dy, grid->dx, 0.0, dxm, dym);
}

int gf_parse_hdr(const char *hdr_file, gf_struct *gf) {
    FILE *fp;
    char line[LINE_BUF];
    char *name, *value, *saveptr;
    int result = 0;
    gf_grid *grid = &gf->grid;

    fp = fopen(hdr_file, "r");

    if (fp == NULL) {
        fprintf(stderr, "No such header file: '%s'\n", hdr_file);
        return -1;
    }

    while (fgets(line, LINE_BUF, fp) != NULL) {

        name = strtok_r(line, TOKEN_DELIM, &saveptr);
        if (name == NULL) {
            /* For empty lines */
            continue;
        }
        value = strtok_r(NULL, TOKEN_DELIM, &saveptr);

        if (strcmp(name, "ncols") == 0) {
            grid->nx = atoi(value);
        } else if (strcmp(name, "nrows") == 0) {
            grid->ny = atoi(value);
        } else if (strcmp(name, "xllcorner") == 0) {
            grid->left = atof(value);
        } else if (strcmp(name, "yllcorner") == 0) {
            grid->bottom = atof(value);
        } else if (strcmp(name, "cellsize") == 0) {
            grid->dx = atof(value);
            grid->dy = atof(value);
        } else if (strcmp(name, "xcellsize") == 0) {
            grid->dx = atof(value);
        } else if (strcmp(name, "ycellsize") == 0) {
            grid->dy = atof(value);
        } else if (strcmp(name, "NODATA_value") == 0) {
            gf->null_value = (gf_float) atof(value);
        } else if (strcmp(name, "byteorder") == 0) {
            strcpy(gf->byte_order, value);
        } else {
            fprintf(stderr, "Unrecognized gridgf_float header field: '%s'\n", name);
            result = -1;
        }
    }

    grid->right = grid->left + (grid->nx - 1) * grid->dx;
    grid->top = grid->bottom + (grid->ny - 1) * grid->dy;

    if (fp != NULL) {
        fclose(fp);
    }

    return result;
}

void gf_close(gf_struct *gf) {
    if (gf->flt != NULL) {
        fclose(gf->flt);
    }
}

int gf_open(const char *hdr_file, const char *flt_file, gf_struct *gf) {
    if (gf_parse_hdr(hdr_file, gf) != 0) {
        return -1;
    }

    gf->flt = fopen(flt_file, "r");

    if (gf->flt == NULL) {
        fprintf(stderr, "Grid gf_float file does not exist: '%s'\n", flt_file);
        return -2;
    }

    return 0;
}

int gf_get_line(long ii, long jj_start, long jj_end, const gf_struct *gf, gf_float *line) {
    fseek(gf->flt, sizeof(gf_float) * (ii * gf->grid.nx + jj_start), SEEK_SET);
    fread((void *)line, sizeof(gf_float), jj_end - jj_start, gf->flt);
    return 0;
}


void gf_print(const gf_grid *grid, gf_float *data, int xy) {
    long ni, nj, i, j, k;
    int c, nc = 1;

    ni = grid->ny;
    nj = grid->nx;
    if (xy) {
        ni = grid->nx;
        nj = grid->ny;
    }

    //fprintf(stdout, "[");
    for (i = 0, k = 0; i < ni; ++i) {
        fprintf(stdout, "[");
        for (j = 0; j < nj; ++j) {
            if (nc > 1) {
                fprintf(stdout, "[");
            }
                
            for (c = 0; c < nc; ++c) {
                if (xy) {
                    //fprintf(stdout, "%.12e", data[k++]);
                    /* i is ix and j is iy */
                    k = c + nc * (i + ni * (nj - 1 - j));
                } else {
                    /* j is ix and i is iy */
                    k = c + nc * (j + nj * i);
                }
                fprintf(stdout, "%.12e", data[k]);
                    
                if (c < nc - 1) {
                    fprintf(stdout, ", ");
                }
            }

            if (nc > 1) {
                fprintf(stdout, "]");
            }

            if (j < nj - 1) {
                fprintf(stdout, ", ");
            }
        }
        fprintf(stdout, "]\n");
/*
        fprintf(stdout, "]");
        if (i < ny - 1) {
            fprintf(stdout, ", ");
        }
*/
    }
    //fprintf(stdout, "]");
}

int gf_write_hdr(gf_grid *grid, const char *filename) {
    FILE *fp;

    fp = fopen(filename, "w");
    if (fp == NULL) {
        return -1;
    }

    fprintf(fp, "ncols         %d\n", grid->nx);
    fprintf(fp, "nrows         %d\n", grid->ny);
    fprintf(fp, "xllcorner     %f\n", grid->left);
    fprintf(fp, "yllcorner     %f\n", grid->bottom);
    fprintf(fp, "xcellsize     %f\n", grid->dx);
    fprintf(fp, "ycellsize     %f\n", grid->dy);
    fprintf(fp, "NODATA_value  -9999\n");
    fprintf(fp, "byteorder     LSBFIRST");

    fclose(fp);

    return 0;
}

void gf_save(gf_grid *grid, gf_float *data, const char *prefix) {
    FILE *flt;
    char filename[2048];

    strcpy(filename, prefix);
    strcat(filename, ".hdr");

    if (gf_write_hdr(grid, filename)) {
        fprintf(stderr, "Could not open %s for writing.\n", filename);
        return;
    }

    strcpy(filename, prefix);
    strcat(filename, ".flt");

    flt = fopen(filename, "wb");
    if (flt == NULL) {
        fprintf(stderr, "Could not open %s for writing.\n", filename);
        return;
    }

    fwrite((void *)data, sizeof(gf_float), grid->nx * grid->ny, flt);
    fclose(flt);
}

void gf_print_grid_info(gf_grid *grid) {
    fprintf(stdout,
        "grid info:\n"
        "  bounds:\n"
        "    left: %f\n"
        "    right: %f\n"
        "    bottom: %f\n"
        "    top: %f\n"
        "  cellsize: %fx%f degrees\n"
        "  resolution: %dx%d\n",
        grid->left,
        grid->right,
        grid->bottom,
        grid->top,
        grid->dx,
        grid->dy,
        grid->nx,
        grid->ny);
}
