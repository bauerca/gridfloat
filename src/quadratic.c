#include "quadratic.h"

#include <math.h>
#include <stdlib.h>
#include <limits.h>


int gf_biquadratic(
    const gf_struct *gf,
    const gf_grid *to_grid,
    void *set_data_xtras,
    int (*set_data)(
        gf_float nine[][3],
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
    double in_r, in_l, in_b, in_t;

    /* Pointer that will be moved by set_data callback. */
    void *d = data;

    int ii, ii_new, jj; /* Indices for gf_tile */

    /* Buffers for lines of gf_tile data */
    gf_float *line1, *line2, *line3, *line_swp;
    /* x-indices for bounds of line buffers */
    int jj_left, jj_right;
    int jjj; /* Index within line buffer */

    /* Data surrounding requested latlng point. */
    gf_float nine[3][3];
    /* Interpolation weights (y-weight, x-weight). */
    double w[2];

    /* Aliases */
    const gf_grid *from_grid = &gf->grid;
    FILE *flt = gf->flt;

    /* For cubic operations, the bounds within which we can interpolate
    are more restrictive (b/c of the larger stencil). */
    in_l = from_grid->left + 0.5 * from_grid->dx;
    in_r = from_grid->right - 0.5 * from_grid->dx;
    in_b = from_grid->bottom + 0.5 * from_grid->dy;
    in_t = from_grid->top - 0.5 * from_grid->dy;

    /* Find indices of dataset that bound the requested box in x. */
    jj_left = ((int)((to_grid->left - from_grid->left) / from_grid->dx + 0.5)) - 1;
    jj_left = jj_left >= 0 ? jj_left : 0;

    jj_right = ((int)((to_grid->right - from_grid->left) / from_grid->dx + 0.5)) + 2; /* exclusive */
    jj_right = jj_right < from_grid->nx ? jj_right : from_grid->nx - 1;

    line1 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));
    line2 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));
    line3 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));

    rewind(flt);

    //fprintf(stdout, "req x bounds: %f, %f\n", bounds->left, bounds->right);
    
    latlng[0] = lat = to_grid->top;
    for (i = 0, ii = INT_MIN; i < to_ny; ++i) {
        if (lat > in_t || lat < in_b) {
            for (j = 0; j < to_nx; ++j) {
                (*set_null)(&d);
            }
        } else {

            // Read in data three lines at a time: the nearest line
            // (in y), the line above, and the line below.

            // ii_new is nearest line:
            ii_new = (int)((from_grid->top - lat) / from_grid->dy + 0.5);

            if (ii_new == ii + 1) {
                // Advance by one line. line2 -> line1, line3 -> line2.
                line_swp = line1;
                line1 = line2;
                line2 = line3;
                line3 = line_swp;
                gf_get_line(ii_new + 1, jj_left, jj_right, gf, line3);
            } else if (ii_new == ii + 2) {
                // Advance by two lines. line3 -> line1.
                line_swp = line1;
                line1 = line3;
                line3 = line_swp;
                gf_get_line(ii_new, jj_left, jj_right, gf, line2);
                gf_get_line(ii_new + 1, jj_left, jj_right, gf, line3);
            } else if (ii_new > ii + 2) {
                gf_get_line(ii_new - 1, jj_left, jj_right, gf, line1);
                gf_get_line(ii_new, jj_left, jj_right, gf, line2);
                gf_get_line(ii_new + 1, jj_left, jj_right, gf, line3);
            }
            ii = ii_new;

            /* y-weight. [-1, 1] */
            w[0] = (from_grid->top - ii * from_grid->dy - lat) / from_grid->dy;

            latlng[1] = lng = to_grid->left;

            for (j = 0; j < to_nx; ++j) {
                if (lng > in_r || lng < in_l) {
                    (*set_null)(&d);
                } else {
                    // Nearest node.
                    jj = (int)((lng - from_grid->left) / from_grid->dx + 0.5);
                    jjj = jj - jj_left;
                    //fprintf(stdout, "lng: %f, jj: %d, jjj: %d, linelen: %d\n", lng, jj, jjj, jj_right - jj_left);

                    nine[0][0] = line1[jjj - 1];
                    nine[0][1] = line1[jjj];
                    nine[0][2] = line1[jjj + 1];
                    nine[1][0] = line2[jjj - 1];
                    nine[1][1] = line2[jjj];
                    nine[1][2] = line2[jjj + 1];
                    nine[2][0] = line3[jjj - 1];
                    nine[2][1] = line3[jjj];
                    nine[2][2] = line3[jjj + 1];

                    /* x-weight. [-1, 1] */
                    w[1] = (lng - (from_grid->left + jj * from_grid->dx)) / from_grid->dx;

                    (*set_data)(nine, from_grid, w, latlng, set_data_xtras, &d);
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
    free(line3);

    return 0;
}


int gf_biquadratic_gradient_kernel(gf_float nine[][3], const gf_grid *from_grid, double *w, double *latlng, void *xtras, void **data_ptr) {
    double **dptr = (double **)data_ptr;
    double dx_m = -1.0, dy_m = -1.0; 
    int i, k;
    double avg_w;
    double v[3];

    gf_lengths(latlng[0], latlng[1], from_grid->dy, from_grid->dx, 0.0, &dy_m, &dx_m);
    
    /* Derivative in x. */
    /* Average in y */
    if (w[0] < 0.0) {
        k = 0;
        avg_w = 1.0 + w[0];
    } else {
        k = 1;
        avg_w = w[0];
    }
    for (i = 0; i < 3; ++i) {
        v[i] = (1.0 - avg_w) * nine[k][i] + avg_w * nine[k + 1][i];
    }
    //printf("df/dx from: (%f, %f, %f)\n", v[0], v[1], v[2]);
    //printf("nine: (%f, %f, %f)\n", nine[0][0], nine[0][1], nine[0][2]);
    //printf("      (%f, %f, %f)\n", nine[1][0], nine[1][1], nine[1][2]);
    //printf("      (%f, %f, %f)\n", nine[2][0], nine[2][1], nine[2][2]);

    /* x derivative from cubic */
    (*dptr)[0] = ((v[0] + v[2] - 2.0 * v[1]) * w[1] + 0.5 * (v[2] - v[0])) / dx_m;

    /* Derivative in y. */
    /* Average in x */
    if (w[1] < 0.0) {
        k = 0;
        avg_w = 1.0 + w[1];
    } else {
        k = 1;
        avg_w = w[1];
    }
    for (i = 0; i < 3; ++i) {
        v[i] = (1.0 - avg_w) * nine[i][k] + avg_w * nine[i][k + 1];
    }

    /* y derivative from cubic */
    (*dptr)[1] = -((v[0] + v[2] - 2.0 * v[1]) * w[0] + 0.5 * (v[2] - v[0])) / dy_m;
    
    dptr[0] += 2;
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


int gf_biquadratic_gradient(const gf_struct *gf, const gf_grid *grid, double *gradient) {
    return gf_biquadratic(gf, grid, NULL,
        &gf_biquadratic_gradient_kernel, &gf_set_null_gradient, (void *)gradient);
}
