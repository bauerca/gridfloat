#include "gridfloat.h"
#include "rtree.h"
#include "sort.h"
#include "db.h"

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_GF_ALLOC 32

void gf_init_db(gf_db *db) {
    db->count = 0;
    db->tiles = NULL;
}

int gf_load_db(const char *path, gf_db *db) {
    DIR *dir, *dir2;
    struct dirent ent, ent2, *pent, *pent2;
    struct stat st;
    char subpath[256], flt[256], errbuf[1024];
    int baselen, err = 0;

    if ((dir = opendir(path)) == NULL)
        return 1;

    readdir_r(dir, &ent, &pent);
    while (pent != NULL) {
        if (strcmp(ent.d_name, ".") == 0 || strcmp(ent.d_name, "..") == 0)
            goto next;

        strcpy(subpath, path);
        if (subpath[strlen(subpath) - 1] != '/')
            strcat(subpath, "/");
        strcat(subpath, ent.d_name);

        if ((err = lstat(subpath, &st)) != 0) {
            sprintf(errbuf, "lstat path error on '%s'", ent.d_name);
            perror(errbuf);
            break;
        }

        if (S_ISDIR(st.st_mode)) {
            //printf("%s\n", subpath);

            /* recurse. */
            if ((err = gf_load_db(subpath, db)) != 0)
                break;

            goto next;
        }

        baselen = strlen(subpath) - 4;
        if (S_ISREG(st.st_mode) && strcmp(subpath + baselen, ".hdr") == 0) {
            strncpy(flt, subpath, baselen);
            flt[baselen] = '\0';
            strcat(flt, ".flt");

            /* Check if float file exists. */
            if ((err = lstat(flt, &st)) != 0) {
                sprintf(errbuf, "Could not locate data at '%s'", flt);
                perror(errbuf);
                break;
            }

            if (db->count == 0) {
                db->tiles = (gf_struct *)malloc(NUM_GF_ALLOC * sizeof(gf_struct));
                printf("allocing\n");
            }
            else if (db->count % NUM_GF_ALLOC == 0) {
                printf("reallocing\n");
                db->tiles = (gf_struct *)realloc((void *)db->tiles,
                    (db->count + NUM_GF_ALLOC) * sizeof(gf_struct));
            }

            gf_open(subpath, flt, &db->tiles[db->count++]);
            printf("Opened tile:\n");
            printf("  %s\n", subpath);
            printf("  %s\n", flt);
        }

next:
        readdir_r(dir, &ent, &pent);
    }

    closedir(dir);
    return err;
}

static
int gf_build_db_rtree(gf_db *db) {
    int i, len;
    gf_rtree_node *tree, **nodes;
    gf_struct **sorted_tiles =
        (gf_struct **)malloc(db->count * sizeof(gf_struct *));

    for (i = 0; i < db->count; i++)
        sorted_tiles[i] = &db->tiles[i];

    /* Sort tiles based on Hilbert curve value of tile midpoint. */
    gf_sort(sorted_tiles, db->count, hilbert_cmp);

    /* Make the top level of nodes for the rtree. Simply wrap each
       gf_struct in a gf_rtree_node structure. */
    tree = (gf_rtree_node *)malloc(db->count * sizeof(gf_rtree_node));
    memset((void *)tree, 0, db->count * sizeof(gf_rtree_node));
    for (i = 0; i < db->count; i++)
        tree[i].gf = sorted_tiles[i];

    len = db->count;
    while (len > 1)
        gf_build_rtree_level(&tree, &len);

    db->tree = tree;
    return 0;
}


void gf_close_db(gf_db *db) {
    int i;
    for (i = 0; i < db->count; i++)
        gf_close(&db->tiles[i]);
    gf_free_rtree(db->tree);
    free(db->tiles);
}

