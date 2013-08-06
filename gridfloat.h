#include <stdio.h>

struct grid_float_hdr {
    int nx;
    int ny;
    double left;
    double right;
    double top;
    double bottom;
    double cellsize;
    float null_value;
    char byte_order[64];
};

struct grid_float_bounds {
    double left;
    double right;
    double bottom;
    double top;
};

struct grid_float {
    struct grid_float_hdr hdr;
    FILE *flt;         /* Descriptor for .flt file */
};


int grid_float_parse_hdr(const char *hdr_file, struct grid_float_hdr *hdr);

int grid_float_open(const char *hdr_file, const char *flt_file, struct grid_float *gf);

void grid_float_close(struct grid_float *gf);

int grid_float_data(const struct grid_float *gf,
    const struct grid_float_bounds *bounds, int nx, int ny, float **data);

int grid_float_print(int nx, int ny, float *data);
