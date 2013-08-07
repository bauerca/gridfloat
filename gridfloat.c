#include "gridfloat.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

const int LINE_BUF = 256;
const char * TOKEN_DELIM = " \t";

void grid_float_lengths(double lat, double lng, double dlat, double dlng, double ecc, double *dx, double *dy) {
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

int grid_float_parse_hdr(const char *hdr_file, struct grid_float_hdr *hdr) {
    FILE *fp;
    char line[LINE_BUF];
    char *name, *value, *saveptr;
    int result = 0;
    struct gf_bounds *b = &hdr->bounds;

    //memset((void *)hdr, 0, sizeof(struct grid_float_hdr));

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
            hdr->nx = atol(value);
        } else if (strcmp(name, "nrows") == 0) {
            hdr->ny = atol(value);
        } else if (strcmp(name, "xllcorner") == 0) {
            b->left = atof(value);
        } else if (strcmp(name, "yllcorner") == 0) {
            b->bottom = atof(value);
        } else if (strcmp(name, "cellsize") == 0) {
            hdr->cellsize = atof(value);
        } else if (strcmp(name, "NODATA_value") == 0) {
            hdr->null_value = (gf_float) atof(value);
        } else if (strcmp(name, "byteorder") == 0) {
            strcpy(hdr->byte_order, value);
        } else {
            fprintf(stderr, "Unrecognized gridgf_float header field: '%s'\n", name);
            result = -1;
        }
    }

    b->right = b->left + (hdr->nx - 1) * hdr->cellsize;
    b->top = b->bottom + (hdr->ny - 1) * hdr->cellsize;

    if (fp != NULL) {
        fclose(fp);
    }

    return result;
}

void grid_float_close(struct grid_float *gf) {
    if (gf->flt != NULL) {
        fclose(gf->flt);
    }
}

int grid_float_open(const char *hdr_file, const char *flt_file, struct grid_float *gf) {
    if (grid_float_parse_hdr(hdr_file, &gf->hdr) != 0) {
        return -1;
    }

    gf->flt = fopen(flt_file, "r");

    if (gf->flt == NULL) {
        fprintf(stderr, "Grid gf_float file does not exist: '%s'\n", flt_file);
        return -2;
    }

    return 0;
}

int gf_get_line(long ii, long jj_start, long jj_end, const struct grid_float *gf, gf_float *line) {
    fseek(gf->flt, sizeof(gf_float) * (ii * gf->hdr.nx + jj_start), SEEK_SET);
    fread((void *)line, sizeof(gf_float), jj_end - jj_start, gf->flt);
    return 0;
}


void grid_float_print(const struct gf_grid *grid, gf_float *data, int xy) {
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
