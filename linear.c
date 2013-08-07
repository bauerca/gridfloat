#include "linear.h"

#include <math.h>
#include <stdlib.h>
#include <limits.h>


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
    int i, j;             /* Indices for subgrid */
    double lat, lng, latlng[2]; /* For passing to op */
    double dx, dy;              /* For requested subgrid */
    long nx = grid->nx, ny = grid->ny;

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
    const struct gf_bounds *b = &grid->bounds, *hb = &hdr->bounds;
    FILE *flt = gf->flt;

    /* Allocate output array */
    //d = (gf_float *)malloc(nc * nx * ny * sizeof(gf_float));

    //op_out = (gf_float *)malloc(nc * sizeof(gf_float));

    /* Find indices of dataset that bound the requested box in x. */
    jj_right = ((int)((b->right - hb->left) / hdr->cellsize)) + 2; /* exclusive */
    jj_left = (int)((b->left - hb->left) / hdr->cellsize);

    dx = (b->right - b->left) / ((double)grid->nx);
    dy = (b->top - b->bottom) / ((double)grid->ny);

    line1 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));
    line2 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));

    rewind(flt);

    //fprintf(stdout, "req x bounds: %f, %f\n", bounds->left, bounds->right);
    
    latlng[0] = lat = b->top - 0.5 * dy;
    for (i = 0, ii = INT_MIN; i < ny; ++i) {
        if (lat > hb->top || lat < hb->bottom) {
            for (j = 0; j < nx; ++j) {
                (*set_null)(&d);
            }
        } else {

            // Read in data two lines at a time; the two lines
            // should bracket (in y) the current latitude.

            // ii_new brackets above:
            //ii_new = get_ii(lat, hdr);
            ii_new = (int) ((hb->top - lat) / hdr->cellsize);

            if (ii_new == ii + 1) {
                line_swp = line1;
                line1 = line2;
                line2 = line_swp;
                line_swp = NULL;
                gf_get_line(ii_new + 1, jj_left, jj_right, gf, line2);
            } else if (ii_new > ii + 1) {
                gf_get_line(ii_new, jj_left, jj_right, gf, line1);
                gf_get_line(ii_new + 1, jj_left, jj_right, gf, line2);
            }
            ii = ii_new;

            /* y-weight */
            w[0] = (hb->top - ii * hdr->cellsize - lat) / hdr->cellsize;

/*
            fprintf(stdout, "latitude: %f, line1 lat: %f, line2 lat: %f, wy: %f\n",
                lat, (hdr->top - ii * hdr->cellsize),
                (hdr->top - (ii + 1) * hdr->cellsize), wy);
*/

            latlng[1] = lng = b->left + 0.5 * dx;

            for (j = 0; j < nx; ++j) {
                if (lng > hb->right || lng < hb->left) {
                    (*set_null)(&d);
                } else {
                    //jj = get_jj(lng, hdr);
                    jj = (int) ((lng - hb->left) / hdr->cellsize);
                    jjj = jj - jj_left;
                    //fprintf(stdout, "lng: %f, jj: %d, jjj: %d, linelen: %d\n", lng, jj, jjj, jj_right - jj_left);

                    quad[0] = line1[jjj];
                    quad[1] = line1[jjj + 1];
                    quad[2] = line2[jjj];
                    quad[3] = line2[jjj + 1];

                    /* x-weight */
                    w[1] = (lng - (hb->left + jj * hdr->cellsize)) / hdr->cellsize;

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
