#ifndef GF_TILE_H
#define GF_TILE_H

#include "gridfloat.h"
#include "db.h"

typedef struct gf_tile_schema {
    int nx_tile;
    int ny_tile;
    int overlap;
    gf_bounds bounds;
    char tile_path_tmpl[256];
    char dir[256];
} gf_tile_schema;

int gf_get_tile_path(const gf_tile_schema *schema, int ix, int iy, int iz, char *path);

int gf_build_tile(int ix, int iy, int iz, gf_tile_schema *schema,
    gf_db *db, float *out);

int quad_interp(float *quads, int nx, int ny, float *out);

int gf_save_tile(const gf_tile_schema *schema, gf_grid *tgrid,
    int ix, int iy, int iz, int pad, float *data);


#endif
