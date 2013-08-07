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
            hdr->left = atof(value);
        } else if (strcmp(name, "yllcorner") == 0) {
            hdr->bottom = atof(value);
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

    hdr->right = hdr->left + (hdr->nx - 1) * hdr->cellsize;
    hdr->top = hdr->bottom + (hdr->ny - 1) * hdr->cellsize;

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

static
int get_line(long ii, long jj_start, long jj_end, const struct grid_float *gf, gf_float *line) {
    fseek(gf->flt, sizeof(gf_float) * (ii * gf->hdr.nx + jj_start), SEEK_SET);
    fread((void *)line, sizeof(gf_float), jj_end - jj_start, gf->flt);
}

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
    int (*set_null)(void **),
    void *data
) {
    int i, j, c, k;             /* Indices for subgrid */
    double lat, lng, latlng[2]; /* For passing to op */
    double dx, dy;              /* For requested subgrid */
    long nx = grid->nx, ny = grid->ny;
    const struct gf_bounds *b = &grid->bounds;

    void *d = data;

    int ii, ii_new, jj; /* Indices for gf_tile */

    /* Buffers for lines of gf_tile data */
    gf_float *line1, *line2, *line_swp;
    /* x-indices for bounds of line buffers */
    int jj_left, jj_right;
    int jjj; /* Index within line buffer */

    /* Data surrounding requested latlng point. */
    gf_float quad[4];
    /* Interpolation weights (y-weight, x-weight). */
    double w[2];

    /* Aliases */
    const struct grid_float_hdr *hdr = &gf->hdr;
    FILE *flt = gf->flt;

    /* Allocate output array */
    //d = (gf_float *)malloc(nc * nx * ny * sizeof(gf_float));

    //op_out = (gf_float *)malloc(nc * sizeof(gf_float));

    /* Find indices of dataset that bound the requested box in x. */
    jj_right = ((int)((b->right - hdr->left) / hdr->cellsize)) + 2; /* exclusive */
    jj_left = (int)((b->left - hdr->left) / hdr->cellsize);

    dx = (b->right - b->left) / ((double)grid->nx);
    dy = (b->top - b->bottom) / ((double)grid->ny);

    line1 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));
    line2 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));

    rewind(flt);

    //fprintf(stdout, "req x bounds: %f, %f\n", bounds->left, bounds->right);
    
    latlng[0] = lat = b->top - 0.5 * dy;
    for (k = 0, i = 0, ii = INT_MIN; i < ny; ++i) {
        if (lat > hdr->top || lat < hdr->bottom) {
            for (j = 0; j < nx; ++j) {
                (*set_null)(&d);
            }
        } else {

            // Read in data two lines at a time; the two lines
            // should bracket (in y) the current latitude.

            // ii_new brackets above:
            //ii_new = get_ii(lat, hdr);
            ii_new = (int) ((hdr->top - lat) / hdr->cellsize);


            if (ii_new == ii + 1) {
                line_swp = line1;
                line1 = line2;
                line2 = line_swp;
                line_swp = NULL;
                get_line(ii_new + 1, jj_left, jj_right, gf, line2);
            } else if (ii_new > ii + 1) {
                get_line(ii_new, jj_left, jj_right, gf, line1);
                get_line(ii_new + 1, jj_left, jj_right, gf, line2);
            }
            ii = ii_new;

            /* y-weight */
            w[0] = (hdr->top - ii * hdr->cellsize - lat) / hdr->cellsize;

/*
            fprintf(stdout, "latitude: %f, line1 lat: %f, line2 lat: %f, wy: %f\n",
                lat, (hdr->top - ii * hdr->cellsize),
                (hdr->top - (ii + 1) * hdr->cellsize), wy);
*/

            latlng[1] = lng = b->left + 0.5 * dx;

            for (j = 0; j < nx; ++j) {
                if (lng > hdr->right || lng < hdr->left) {
                    (*set_null)(&d);
                } else {
                    //jj = get_jj(lng, hdr);
                    jj = (int) ((lng - hdr->left) / hdr->cellsize);
                    jjj = jj - jj_left;
                    //fprintf(stdout, "lng: %f, jj: %d, jjj: %d, linelen: %d\n", lng, jj, jjj, jj_right - jj_left);

                    quad[0] = line1[jjj];
                    quad[1] = line1[jjj + 1];
                    quad[2] = line2[jjj];
                    quad[3] = line2[jjj + 1];

                    /* x-weight */
                    w[1] = (lng - (hdr->left + jj * hdr->cellsize)) / hdr->cellsize;

                    (*set_data)(quad, hdr->cellsize, w, latlng, set_data_xtras, &d);
                }

                lng += dx;
                latlng[1] = lng;
            }
        }
        lat -= dy;
        latlng[0] = lat;
    }

    free(line1);
    free(line2);

    return 0;
}


static
int gf_set_null_gf_float(void **data_ptr) {
    gf_float **dptr = (gf_float **)data_ptr;
    (*dptr)[0] = GF_NULL_VAL;
    dptr[0]++;
    return 0;
}

int grid_float_bilinear_interpolate_kernel(gf_float *quad, double cellsize, double *w, double *latlng, void *xtras, void **data_ptr) {
    gf_float **dptr = (gf_float **)data_ptr;

    //fprintf(stdout, "%f, %f, %f, %f\n", quad[0], quad[1], quad[2], quad[3]);
    (*dptr)[0] = (1.0 - w[0]) * (1.0 - w[1]) * quad[0] +
                 (1.0 - w[0]) *        w[1]  * quad[1] +
                        w[0]  * (1.0 - w[1]) * quad[2] +
                        w[0]  *        w[1]  * quad[3];
    dptr[0]++;
    return 0;
}


int grid_float_bilinear_interpolate(const struct grid_float *gf, const struct gf_grid *grid, gf_float *data) {
    return grid_float_bilinear(gf, grid, NULL,
        &grid_float_bilinear_interpolate_kernel,
        &gf_set_null_gf_float,
        (void *)data);
}


int grid_float_bilinear_gradient_kernel(gf_float *quad, double cellsize, double *w, double *latlng, void *xtras, void **data_ptr) {
    double **grad_ptr = (double **)data_ptr;
    double csz_m = -1.0; 

    grid_float_lengths(latlng[0], latlng[1], cellsize, cellsize, 0.0, &csz_m, &csz_m);

    /* Avg in y, diff in x */
    (*grad_ptr)[0] = (((1.0 - w[0]) * quad[1] + w[0] * quad[3]) - 
                      ((1.0 - w[0]) * quad[0] + w[0] * quad[2])) / csz_m;
    /* Avg in x, diff in y */
    (*grad_ptr)[1] = (((1.0 - w[1]) * quad[0] + w[1] * quad[2]) - 
                      ((1.0 - w[1]) * quad[1] + w[1] * quad[3])) / csz_m;
    grad_ptr[0] += 2;
    return 0;
}

static
int gf_set_null_gradient(void **data_ptr) {
    double **grad_ptr = (double **)data_ptr;
    (*grad_ptr)[0] = GF_NULL_VAL;
    (*grad_ptr)[1] = GF_NULL_VAL;
    grad_ptr[0] += 2;
    return 0;
}


int grid_float_bilinear_gradient(const struct grid_float *gf, const struct gf_grid *grid, double *gradient) {
    return grid_float_bilinear(gf, grid, NULL,
        &grid_float_bilinear_gradient_kernel, &gf_set_null_gradient, (void *)gradient);
}


int grid_float_print(const struct gf_grid *grid, gf_float *data, int xy) {
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
