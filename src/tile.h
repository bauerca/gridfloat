#ifndef GF_TILE_H
#define GF_TILE_H


typedef struct tile_schema {
    int overlap;
    int nx_tile;
    int ny_tile;
    gf_bounds bounds;
    char tile_name[256];
    char dir[256];
} tile_schema;


int gf_build_tile(int ix, int iy, int iz, int extra, tile_schema *schema,
    gf_db *db, float *out);


int quad_interp(float *quads, int nx, int ny, float *out);


#endif
