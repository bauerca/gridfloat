
int gf_bicubic(
    const struct grid_float *gf,
    const struct gf_grid *grid,
    void *set_data_xtras,
    int (*set_data)(
        gf_float **nine,
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
    gf_float nine[3][3];
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
