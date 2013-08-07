#include "cubic.h"

#include <math.h>
#include <stdlib.h>
#include <limits.h>


int gf_bicubic(
    const struct grid_float *gf,
    const struct gf_grid *grid,
    void *set_data_xtras,
    int (*set_data)(
        gf_float nine[][3],
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
    struct gf_bounds in;

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
    const struct grid_float_hdr *hdr = &gf->hdr;
    const struct gf_bounds *b = &grid->bounds, *hb = &hdr->bounds;
    FILE *flt = gf->flt;

    /* For cubic operations, the bounds within which we can interpolate
    are more restrictive (b/c of the larger stencil). */
    in.left = hb->left + 0.5 * hdr->cellsize;
    in.right = hb->right - 0.5 * hdr->cellsize;
    in.bottom = hb->bottom + 0.5 * hdr->cellsize;
    in.top = hb->top - 0.5 * hdr->cellsize;

    /* Allocate output array */
    //d = (gf_float *)malloc(nc * nx * ny * sizeof(gf_float));

    //op_out = (gf_float *)malloc(nc * sizeof(gf_float));

    /* Find indices of dataset that bound the requested box in x. */
    jj_left = ((int)((b->left - hb->left) / hdr->cellsize + 0.5)) - 1;
    jj_left = jj_left >= 0 ? jj_left : 0;

    jj_right = ((int)((b->right - hb->left) / hdr->cellsize + 0.5)) + 2; /* exclusive */
    jj_right = jj_right < hdr->nx ? jj_right : hdr->nx - 1;

    dx = (b->right - b->left) / ((double)grid->nx);
    dy = (b->top - b->bottom) / ((double)grid->ny);

    line1 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));
    line2 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));
    line3 = (gf_float *)malloc((jj_right - jj_left) * sizeof(gf_float));

    rewind(flt);

    //fprintf(stdout, "req x bounds: %f, %f\n", bounds->left, bounds->right);
    
    latlng[0] = lat = b->top - 0.5 * dy;
    for (i = 0, ii = INT_MIN; i < ny; ++i) {
        if (lat > in.top || lat < in.bottom) {
            for (j = 0; j < nx; ++j) {
                (*set_null)(&d);
            }
        } else {

            // Read in data three lines at a time: the nearest line
            // (in y), the line above, and the line below.

            // ii_new is nearest line:
            ii_new = (int)((hb->top - lat) / hdr->cellsize + 0.5);

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
            w[0] = (hb->top - ii * hdr->cellsize - lat) / hdr->cellsize;

/*
            fprintf(stdout, "latitude: %f, line1 lat: %f, line2 lat: %f, wy: %f\n",
                lat, (hdr->top - ii * hdr->cellsize),
                (hdr->top - (ii + 1) * hdr->cellsize), wy);
*/

            latlng[1] = lng = b->left + 0.5 * dx;

            for (j = 0; j < nx; ++j) {
                if (lng > in.right || lng < in.left) {
                    (*set_null)(&d);
                } else {
                    // Nearest node.
                    jj = (int)((lng - hb->left) / hdr->cellsize + 0.5);
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
                    w[1] = (lng - (hb->left + jj * hdr->cellsize)) / hdr->cellsize;

                    (*set_data)(nine, hdr->cellsize, w, latlng, set_data_xtras, &d);
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
    free(line3);

    return 0;
}


int gf_bicubic_gradient_kernel(gf_float nine[][3], double cellsize, double *w, double *latlng, void *xtras, void **data_ptr) {
    double **dptr = (double **)data_ptr;
    double csz_m = -1.0; 
    int i, k;
    double avg_w;
    double v[3];

    grid_float_lengths(latlng[0], latlng[1], cellsize, cellsize, 0.0, &csz_m, &csz_m);
    
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
    (*dptr)[0] = ((v[0] + v[2] - 2.0 * v[1]) * w[1] + 0.5 * (v[2] - v[0])) / csz_m;

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
    (*dptr)[1] = -((v[0] + v[2] - 2.0 * v[1]) * w[0] + 0.5 * (v[2] - v[0])) / csz_m;
    
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


int gf_bicubic_gradient(const struct grid_float *gf, const struct gf_grid *grid, double *gradient) {
    return gf_bicubic(gf, grid, NULL,
        &gf_bicubic_gradient_kernel, &gf_set_null_gradient, (void *)gradient);
}
