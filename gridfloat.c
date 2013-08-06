#include "gridfloat.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>

const int LINE_BUF = 256;
const char * TOKEN_DELIM = " \t";

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
            hdr->nx = atoi(value);
        } else if (strcmp(name, "nrows") == 0) {
            hdr->ny = atoi(value);
        } else if (strcmp(name, "xllcorner") == 0) {
            hdr->left = atof(value);
        } else if (strcmp(name, "yllcorner") == 0) {
            hdr->bottom = atof(value);
        } else if (strcmp(name, "cellsize") == 0) {
            hdr->cellsize = atof(value);
        } else if (strcmp(name, "NODATA_value") == 0) {
            hdr->null_value = (float) atof(value);
        } else if (strcmp(name, "byteorder") == 0) {
            strcpy(hdr->byte_order, value);
        } else {
            fprintf(stderr, "Unrecognized gridfloat header field: '%s'\n", name);
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
        fprintf(stderr, "Grid float file does not exist: '%s'\n", flt_file);
        return -2;
    }

    return 0;
}

static
int get_line(int ii, int jj_start, int jj_end, const struct grid_float *gf, float *line) {
    fseek(gf->flt, sizeof(float) * (ii * gf->hdr.nx + jj_start), SEEK_SET);
    fread((void *)line, sizeof(float), jj_end - jj_start, gf->flt);
}


int grid_float_data(const struct grid_float *gf, const struct grid_float_bounds *bounds, int ny, int nx, float **data) {
    int i, j, k; /* Indices for output data */
    double lat, lng; /* For each requested location */
    double dx, dy; /* For requested subgrid */

    int ii, ii_new, jj; /* Indices for gridfloat data */

    /* d is alias for data. line{1,2} is buffer for line of gridfloat data */
    float *d, *line1, *line2, *line_swp;
    /* x-indices for bounds of line buffers */
    int jj_left, jj_right;
    int jjj; /* Index within line buffer */

    float wx, wy; /* Interpolation weights */

    /* Aliases */
    const struct grid_float_hdr *hdr = &gf->hdr;
    FILE *flt = gf->flt;

    /* Allocate output array */
    d = (float *)malloc(nx * ny * sizeof(float));

    jj_right = (int) ((bounds->right - hdr->left) / hdr->cellsize) + 1;
    jj_left = (int) ((bounds->left - hdr->left) / hdr->cellsize);

    dx = (bounds->right - bounds->left) / ((float) nx);
    dy = (bounds->top - bounds->bottom) / ((float) ny);

    line1 = (float *)malloc((jj_right - jj_left) * sizeof(float));
    line2 = (float *)malloc((jj_right - jj_left) * sizeof(float));

    rewind(gf->flt);
    
    lat = bounds->top - 0.5 * dy;
    for (k = 0, i = 0, ii = INT_MIN; i < ny; ++i) {
        if (lat > hdr->top || lat < hdr->bottom) {
            for (j = 0; j < nx; ++j) {
                d[k++] = hdr->null_value;
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

            wy = (lat - (hdr->top - ii * hdr->cellsize)) / hdr->cellsize;

            lng = bounds->left + 0.5 * dx;

            for (j = 0; j < nx; ++j) {
                if (lng > hdr->right || lng < hdr->left) {
                    d[k++] = hdr->null_value;
                } else {
                    //jj = get_jj(lng, hdr);
                    jj = (int) ((lng - hdr->left) / hdr->cellsize);
                    jjj = jj - jj_left;

                    wx = (lng - (hdr->left + jj * hdr->cellsize)) / hdr->cellsize;

                    d[k++] = (1.0 - wx) * (1.0 - wy) * line1[jjj]     +
                                    wx  * (1.0 - wy) * line1[jjj + 1] +
                             (1.0 - wx) *        wy  * line2[jjj]     +
                                    wx  *        wy  * line2[jjj + 1];
                }

                lng += dx;
            }
        }
        lat -= dy;
    }

    free(line1);
    free(line2);

    *data = d;

    return 0;
}


int grid_float_print(int ny, int nx, float *data) {
    int i, j, k;

    for (i = 0, k = 0; i < ny; ++i) {
        for (j = 0; j < nx; ++j) {
            fprintf(stdout, "%.12e", data[k++]);
            if (j < nx - 1) {
                fprintf(stdout, ", ");
            }
        }
        fprintf(stdout, "\n");
    }
}
