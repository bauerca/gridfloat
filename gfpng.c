#include <math.h>
#include <stdlib.h>

#include "gfpng.h"
#include "gridfloat.h"

static
void write_row_callback(png_structp png_ptr, png_uint_32 row, int pass) {
    // No op
}

static
int gf_relief_shade_kernel(gf_float *quad, double cellsize, double *w, double *latlng, void *xtras, void **data_ptr) {
    int i;
    png_byte **shade_ptr = (png_byte **)data_ptr;
    double grad[2] = {GF_NULL_VAL, GF_NULL_VAL}, *grad_view, n_surf[3], *n_sun, norm, shade;

    grad_view = grad;

    n_sun = (double *)xtras;

    grid_float_bilinear_gradient_kernel(quad, cellsize, w, latlng, (void *)NULL, (void **)&grad_view);

    if (grad[0] == GF_NULL_VAL || grad[1] == GF_NULL_VAL) {
        fprintf(stderr, "gf_relief_shade_kernel: Something wrong. Please report.\n");
        return -1;
    }

    //printf("slope: %f\n", sqrt(grad[0] * grad[0] + grad[1] * grad[1]));
    printf("latlng: (%f, %f), weights: (%f, %f), grad: (%f, %f)\n", latlng[0], latlng[1], w[0], w[1], grad[0], grad[1]);

    norm = sqrt(1.0 + grad[0] * grad[0] + grad[1] * grad[1]);
    n_surf[0] = -grad[0] / norm;
    n_surf[1] = -grad[1] / norm;
    n_surf[2] = 1.0 / norm;

    shade = 0.0;
    for (i = 0; i < 3; ++i) {
        shade += n_sun[i] * n_surf[i];
    }

    (*shade_ptr)[0] = (int)(255.0 * (shade > 0.0 ? shade : 0.0));
    shade_ptr[0]++;
    return 0;
}

static
int gf_set_null_png_byte(void **data_ptr) {
    png_byte **dptr = (png_byte **)data_ptr;
    (*dptr)[0] = 0;
    dptr[0]++;
    return 0;
}

int gf_relief_shade(const struct grid_float *gf, const struct gf_grid *grid, double *n_sun, const char *filename) {
    int i;
    png_byte *shade, **shade_rows;
    
    shade = (png_byte *)malloc(grid->nx * grid->ny * sizeof(png_byte));
    shade_rows = (png_byte **)malloc(grid->ny * sizeof(png_byte *));

    grid_float_bilinear(gf, grid, (void *)n_sun,
        &gf_relief_shade_kernel, &gf_set_null_png_byte, (void *)shade);

    for (i = 0; i < grid->ny; ++i) {
        shade_rows[i] = shade + i * grid->nx;
    }

    grid_float_png(grid->nx, grid->ny, shade_rows, filename);

    free(shade);
    free(shade_rows);
}


int grid_float_png(int nx, int ny, png_byte **data, const char *filename) {

    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        return -1;
    }
    
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL);

    if (png_ptr == NULL) {
        fclose(fp);
        return -2;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(fp);
        return -3;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return -4;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, nx, ny, 8, PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_set_write_status_fn(png_ptr, &write_row_callback);

    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, data);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(fp);

    return 0;
}
