#include "gfstl.h"

#include <math.h>
#include <inttypes.h>

#define STL_HDR_LEN 80

static
const uint16_t byte_count = 0;

static
void vert(gf_grid *grid, gf_float *data, int i, int j, double dxm, double dym, float v[3]) {
    v[0] = j * dxm;
    v[1] = (grid->ny - 1 - i) * dym;
    v[2] = data[j + grid->nx * i];
}

/* (v2 - v1) x (v3 - v1) */
static
void calc_normal(float v1[3], float v2[3], float v3[3], float normal[3]) {
    int i;
    float vec1[3], vec2[3];
    float norm;

    for (i = 0; i < 3; i++) {
        vec1[i] = v2[i] - v1[i];
        vec2[i] = v3[i] - v1[i];
    }

    normal[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
    normal[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
    normal[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];

    norm = sqrt(
        normal[0] * normal[0] +
        normal[1] * normal[1] +
        normal[2] * normal[2]);

    for (i = 0; i < 3; i++) {
        normal[i] /= norm;
    }
}

static
void write_triangle(FILE *fp, float v1[3], float v2[3], float v3[3], float normal[3]) {
    fwrite((void *)normal, 4, 3, fp);
    fwrite((void *)v1, 4, 3, fp);
    fwrite((void *)v2, 4, 3, fp);
    fwrite((void *)v3, 4, 3, fp);
    fwrite((void *)&byte_count, 2, 1, fp);
}

int gf_save_stl(gf_grid *grid, gf_float *data, const char *filename) {
    int i, j;
    FILE *fp;
    char hdr[STL_HDR_LEN] = "gridfloat terrain stl!";
    uint32_t n;
    float normal[3], v1[3], v2[3], v3[3];
    double dxm, dym;

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        return -1;
    }

    n = 2 * grid->nx * grid->ny;
    gf_cellsize_meters(grid, &dxm, &dym);

    fwrite((void *)hdr, 1, STL_HDR_LEN, fp);
    fwrite((void *)&n, 4, 1, fp);

    for (i = 0; i < grid->ny - 1; ++i) {
        for (j = 0; j < grid->nx - 1; ++j) {
            /* Upper-left triangle */
            vert(grid, data, i, j, dxm, dym, v1);
            vert(grid, data, i + 1, j, dxm, dym, v2);
            vert(grid, data, i, j + 1, dxm, dym, v3);
            calc_normal(v1, v2, v3, normal);
            write_triangle(fp, v1, v2, v3, normal);

            /* Lower-right triangle */
            vert(grid, data, i + 1, j + 1, dxm, dym, v1);
            calc_normal(v1, v3, v2, normal);
            write_triangle(fp, v1, v3, v2, normal);
        }
    }

    fclose(fp);
    return 0;
}
