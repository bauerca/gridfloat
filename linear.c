#include "linear.h"

#include <math.h>
#include <stdlib.h>
#include <limits.h>


int gf_bilinear(
    const gf_struct *gf,
    const gf_grid *to_grid,
    void *set_data_xtras,
    int (*set_data)(
        gf_float *quad,
        const gf_grid *from_grid,
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
    int to_nx = to_grid->nx, to_ny = to_grid->ny;
    double to_dx = to_grid->dx, to_dy = to_grid->dy;

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
    const gf_grid *from_grid = &gf->grid;
    FILE *flt = gf->flt;

    /* Find indices of dataset that bound the requested box in x. */
    jj_left = (int)((to_grid->left - from_grid->left) / from_grid->dx);
    jj_right = ((int)((to_grid->right - from_grid->left) / from_grid->dx)) + 2; /* exclusive */

    line1 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));
    line2 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));

    rewind(flt);

    //fprintf(stdout, "req x bounds: %f, %f\n", bounds->left, bounds->right);
    
    latlng[0] = lat = to_grid->top;
    for (i = 0, ii = INT_MIN; i < to_ny; ++i) {
        if (lat > from_grid->top || lat < from_grid->bottom) {
            for (j = 0; j < to_nx; ++j) {
                (*set_null)(&d);
            }
        } else {

            // Read in data two lines at a time; the two lines
            // should bracket (in y) the current latitude.

            // ii_new brackets above:
            ii_new = (int) ((from_grid->top - lat) / from_grid->dy);

            /* Reuse lines from previous iteration. */
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

            /* y-weight. Normalized (to dy) distance from top line to
             * current latitude.
             */
            w[0] = (from_grid->top - ii * from_grid->dy - lat) / from_grid->dy;

            latlng[1] = lng = to_grid->left;

            for (j = 0; j < to_nx; ++j) {
                if (lng > from_grid->right || lng < from_grid->left) {
                    (*set_null)(&d);
                } else {
                    jj = (int) ((lng - from_grid->left) / from_grid->dx);
                    jjj = jj - jj_left;
                    //fprintf(stdout, "lng: %f, jj: %d, jjj: %d, linelen: %d\n", lng, jj, jjj, jj_right - jj_left);

                    quad[0] = line1[jjj];
                    quad[1] = line1[jjj + 1];
                    quad[2] = line2[jjj];
                    quad[3] = line2[jjj + 1];

                    /* x-weight */
                    w[1] = (lng - (from_grid->left + jj * from_grid->dx)) / from_grid->dx;

                    (*set_data)(quad, from_grid, w, latlng, set_data_xtras, &d);
                }

                lng += to_dx;
                latlng[1] = lng;
            }
        }
        lat -= to_dy;
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

int gf_bilinear_interpolate_kernel(gf_float *quad, const gf_grid *from_grid, double *w, double *latlng, void *xtras, void **data_ptr) {
    gf_float **dptr = (gf_float **)data_ptr;

    //fprintf(stdout, "%f, %f, %f, %f\n", quad[0], quad[1], quad[2], quad[3]);
    (*dptr)[0] = (1.0 - w[0]) * (1.0 - w[1]) * quad[0] +
                 (1.0 - w[0]) *        w[1]  * quad[1] +
                        w[0]  * (1.0 - w[1]) * quad[2] +
                        w[0]  *        w[1]  * quad[3];
    dptr[0]++;
    return 0;
}


int gf_bilinear_interpolate(const gf_struct *gf, const gf_grid *grid, gf_float *data) {
    return gf_bilinear(gf, grid, NULL,
        &gf_bilinear_interpolate_kernel,
        &gf_set_null_gf_float,
        (void *)data);
}


int gf_bilinear_gradient_kernel(gf_float *quad, const gf_grid *from_grid, double *w, double *latlng, void *xtras, void **data_ptr) {
    double **grad_ptr = (double **)data_ptr;
    double dx_m = -1, dy_m = -1; 

    gf_lengths(latlng[0], latlng[1], from_grid->dy, from_grid->dx, 0.0, &dy_m, &dx_m);

    /* Avg in y, diff in x */
    (*grad_ptr)[0] = (((1.0 - w[0]) * quad[1] + w[0] * quad[3]) - 
                      ((1.0 - w[0]) * quad[0] + w[0] * quad[2])) / dx_m;
    /* Avg in x, diff in y */
    (*grad_ptr)[1] = (((1.0 - w[1]) * quad[0] + w[1] * quad[2]) - 
                      ((1.0 - w[1]) * quad[1] + w[1] * quad[3])) / dy_m;
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


int gf_bilinear_gradient(const gf_struct *gf, const gf_grid *grid, double *gradient) {
    return gf_bilinear(gf, grid, NULL,
        &gf_bilinear_gradient_kernel, &gf_set_null_gradient, (void *)gradient);
}
