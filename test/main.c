
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include "../src/gridfloat.h"
#include "../src/rtree.h"


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

int test_load_db() {
    gf_db db;
    gf_init_db(&db);
    gf_load_db(dbpath, &db);
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
    gf_load_db(dbpath, &db);

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

int test_rtree_db() {
    int i;
    gf_db db;
    gf_struct **sorted_tiles, *tile;
    gf_rtree_node *tree;

    gf_init_db(&db);
    gf_load_db(dbpath, &db);

    sorted_tiles =
        (gf_struct **)malloc(db.count * sizeof(gf_struct *));
    for (i = 0; i < db.count; i++)
        sorted_tiles[i] = &db.tiles[i];

    gf_sort(sorted_tiles, db.count, cmp);
    tree = gf_build_rtree(sorted_tiles, db.count);

    gf_free_rtree(tree);
    free(sorted_tiles);
    gf_close_db(&db);
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

    
    test(test_load_db, "test loading a gridfloat db");
    test(test_sort_db, "sort the tiles in the gridfloat db");
    test(test_rtree_db, "make a btree out of the sorted tiles");
	printf("\nPASSED: %d\nFAILED: %d\n", test_passed, test_failed);

    return 0;
}
