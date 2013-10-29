
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include "../src/gridfloat.h"
#include "../src/rtree.h"
#include "../src/db.h"
#include "../src/sort.h"
#include "../src/tile.h"


static int test_passed = 0;
static int test_failed = 0;

/* Terminate current test with error */
#define fail()	return __LINE__

/* Successfull end of the test case */
#define done() return 0

/* Check single condition */
#define check(cond) do { if (!(cond)) fail(); } while (0)

/* Test runner */
static void test(int (*func)(void), const char *name) {
	int r = func();
	if (r == 0) {
		test_passed++;
	} else {
		test_failed++;
		printf("FAILED: %s (at line %d)\n", name, r);
	}
}

static char dbpath[256] = "";

int test_load_db_tiles() {
    gf_db db;
    gf_init_db(&db);
    gf_db_load_tiles(dbpath, &db);
    gf_close_db(&db);
    return 0;
}

static
int cmp(gf_struct *tile1, gf_struct *tile2) {
    //printf("tile1 left=%f, tile2 left=%f\n", tile1->grid.left,
    //    tile2->grid.left);
    if (tile1->grid.left < tile2->grid.left) return -1;
    else if (tile1->grid.left == tile2->grid.left) return 0;
    else return 1;
}

int test_sort_db() {
    int i;
    gf_db db;
    gf_struct **sorted_tiles, *tile;

    gf_init_db(&db);
    gf_db_load_tiles(dbpath, &db);

    sorted_tiles =
        (gf_struct **)malloc(db.count * sizeof(gf_struct *));
    for (i = 0; i < db.count; i++)
        sorted_tiles[i] = &db.tiles[i];

    gf_sort(sorted_tiles, db.count, cmp);

    for (i = 0; i < db.count - 1; i++)
        check(sorted_tiles[i]->grid.left <= sorted_tiles[i+1]->grid.left);

    free(sorted_tiles);
    gf_close_db(&db);
    return 0;
}

int test_open_db() {
    gf_db db;
    gf_open_db(dbpath, &db);
    gf_close_db(&db);
    return 0;
}

int test_search_db() {
    gf_db db;
    gf_open_db(dbpath, &db);
    gf_rtree_node **leaves;
    int found;
    gf_bounds b;

    b.left = -121.1;
    b.right = -120.9;
    b.bottom = 44.9;
    b.top = 45.1;

    leaves = (gf_rtree_node **)malloc(db.count * sizeof(gf_rtree_node *));
    gf_search_rtree(&b, db.tree, leaves, &found);
    
    check(found > 0);
    printf("found %d\n", found);

    free(leaves);
    gf_close_db(&db);
    return 0;
}

int test_get_data_db() {
    gf_db db;
    gf_grid grid;
    float data[4];
    int i, n = 2;
    double wh = 0.002;

    gf_init_grid_point(&grid, 45, -121, wh, wh, n, n);

    gf_open_db(dbpath, &db);
    gf_db_get_data(&grid, &db, data);

    for (i = 0; i < n*n; i++)
        printf("%f, ", data[i]);
    printf("\n");
    
    gf_close_db(&db);
    return 0;
}

int test_quad_interp() {
    const int n = 2;
    int i, j, nq = (n+2)*(n+2);
    float quads[4*nq];
    float out[n*n];

    for (i = 0; i < 4*nq; i++)
        quads[i] = 1.0f;

    //for (i = 0; i < 4; i++)
    //    for (j = 0; j < (n+2)*(n+2); j++)
    //        printf("%f, ", ((float (*)[4])quads)[i][j]);
    //printf("\n");

    quad_interp(quads, n, n, out);

    for (i = 0; i < n*n; i++)
        check(out[i] == 1.0f);

    return 0;
}

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "db",	required_argument,	NULL, 'd' },
	{ NULL, 0, 0, 0 }
};

int main(int argc, char **argv)
{
	int n = 0;

	fprintf(stderr, "libwebsockets test fraggle\n"
			"(C) Copyright 2010-2013 Andy Green <andy@warmcat.com> "
						    "licensed under LGPL2.1\n");

	while (n >= 0) {
		n = getopt_long(argc, argv, "hd:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
		case 'd':
            strcpy(dbpath, optarg);
			break;
		case 'h':
			fprintf(stderr, "Usage: gridfloat-test "
					"-d <dirpath> [--help]\n\n"
					"<dirpath> is the path to a directory "
                    "containing any number of flt/hdr file pairs "
                    "at any directory depth. We call this a "
                    "gridfloat database.\n");
			exit(1);
		}
	}

    if (strlen(dbpath) == 0) {
        fprintf(stderr, "Must give path to gridfloat data.\n");
        return 1;
    }

    
    test(test_load_db_tiles, "test loading a gridfloat db");
    test(test_sort_db, "sort the tiles in the gridfloat db");
    test(test_open_db, "load the tiles, sort them, and make an rtree");
    test(test_search_db, "search db for some intersecting tiles");
    test(test_get_data_db, "get four values from a database, each from a different tile");
    test(test_quad_interp, "smooth/interp four tiles to one at half resolution");
	printf("\nPASSED: %d\nFAILED: %d\n", test_passed, test_failed);

    return 0;
}
