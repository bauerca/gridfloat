#ifndef GF_DB_H
#define GF_DB_H


typedef struct gf_db {
    gf_struct *tiles;
    int count;
    gf_rtree_node *tree;
} gf_db;

void gf_init_db(gf_db *db);

int gf_db_load_tiles(const char *dirpath, gf_db *db);

int gf_db_build_rtree(gf_db *db);

int gf_open_db(const char *dirpath, gf_db *db);

/*
 * Extract a grid of data from the open gridfloat database.
 *
 * Fills pre-allocated array of floats with elevation interpolated
 * to the given grid.
 */
int gf_db_get_data(gf_grid *grid, gf_db *db, float *data);

void gf_close_db(gf_db *db);

#endif
