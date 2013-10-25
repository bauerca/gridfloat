#ifndef GF_DB_H
#define GF_DB_H


typedef struct gf_db {
    gf_struct *tiles;
    int count;
    gf_rtree_node *tree;
} gf_db;

void gf_init_db(gf_db *db);

int gf_load_db(const char *dirpath, gf_db *db);

void gf_close_db(gf_db *db);

#endif
