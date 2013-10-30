
#include "tile.h"
#include "db.h"

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


void print_usage(void) {
    fprintf(stdout,
        "Summary:\n"
        "  Tile a database of GridFloat files.\n"
        "\n"
        "Usage:\n"
        "  tiler -d <path>\n"
        "        -D <path>\n"
        "        -R <resolution>\n"
        "        (-B <bounds>|-n <lat> -w <lng> -s <size>)\n"
        "        -p <path template>\n"
        "        [<options>]\n"
        "\n"
        "Options:\n"
        "  -h:  Print this help message.\n"
        "  -R:  Resolution of a tile. If a single integer is\n"
        "       supplied, then the resolution is the same in both\n"
        "       directions. Otherwise, the format is (by example):\n"
        "       '-R 128x256' where the first number is the resolution\n"
        "       in the x-direction (along lines of latitude).\n"
        "  -l:  Left bound of area to tile.\n"
        "  -r:  Right bound of area to tile.\n"
        "  -b:  Bottom bound of area to tile.\n"
        "  -t:  Top bound of area to tile.\n"
        "  -B:  Specify tiling bounds using a comma-separated\n"
        "       list of the form LEFT,RIGHT,BOTTOM,TOP. E.g.\n"
        "       '-B -123.112,-123.110,42.100,42.101'.\n"
        "  -n:  Longitude midpoint of tiled area. Must use with '-s'\n"
        "       option. Example: '-n 42'.\n"
        "  -w:  Latitude midpoint of tiled area. Must use with '-s'\n"
        "       option. Example: '-w 123' (notice there is no minus\n"
        "       sign here).\n"
        "  -D:  Tile directory. Individual tile paths are specified\n"
        "       relative to this directory (see option -p).\n"
        "  -p:  Path to a tile (relative to -D option). This is a format\n"
        "       string; the tiler injects tile indices where specified.\n"
        "       A sane path naming scheme is usually along the lines of:\n"
        "\n"
        "           -p \"[z]/[y]/[x]/tile\"\n"
        "\n"
        "       which will expand to \"1/0/0/tile.flt\" for the data file\n"
        "       of the top-left tile at zoom level 1.\n"
        "  -d:  Path to root of gridfloat database (which is simply a\n"
        "       directory that contains .hdr/.flt file pairs at any depth)\n"
        "  -s:  Size of tiled area (in degrees) around midpoint. Used with\n"
        "       '-n' and '-w' options. Examples: '-s 1.0x1.0' or just '-s 1'.\n"
        "       If a rectangular size is requested, the first number refers\n"
        "       to the width of the box (along x; i.e. along lines of\n"
        "       latitude).\n"
        "  -o:  Tile overlap. Integer specifying the number of layers of\n"
        "       cells that overlap for neighboring tiles (default 1).\n"
        "\n"
        "Examples:\n"
        "  nope.\n"
    );
}

void fail(const char *msg) {
    print_usage();
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

static
int check_dir(const char *dir) {
    struct stat st;
    int err;
    if ((err = lstat(dir, &st)) != 0) {
        perror("");
        return err;
    }

    if (S_ISDIR(st.st_mode) != 0)
        fprintf(stderr, "%s is not a directory\n", dir);
        return -1;

    return 0;
}

static struct option options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "db", required_argument,	NULL, 'd' },
	{ "dir", required_argument,	NULL, 'D' },
	{ "tile-path", required_argument, NULL, 'p' },
	{ "tile-res", required_argument, NULL, 'R' },
	{ "bounds", required_argument, NULL, 'B' },
	{ "left", required_argument, NULL, 'l' },
	{ "right", required_argument, NULL, 'r' },
	{ "bottom", required_argument, NULL, 'b' },
	{ "top", required_argument, NULL, 't' },
	{ "north", required_argument, NULL, 'n' },
	{ "west", required_argument, NULL, 'w' },
	{ "size", required_argument, NULL, 's' },
	{ "overlap", required_argument, NULL, 'o' },
	{ NULL, 0, 0, 0 }
};

#define BADLATLNG 1e9

int main(int argc, char *argv[]) {
    struct stat st;

    int n, count, opt, len, err = 0;
    gf_tile_schema schema;
    gf_db db;
    gf_bounds *b;

    /* Extraction grid */
    double *b_view[4] = { &b->left, &b->right, &b->bottom, &b->top };
    int *res_view[2] = { &schema.nx_tile, &schema.ny_tile };
    double latlng[2] = { 0, 0 };
    double wh[2] = {0, 0}; /* Width-Height */
    int from_point = 0;

    memset(&schema, 0, sizeof(gf_tile_schema));
    schema.nx_tile = schema.ny_tile = 128;
    schema.overlap = 1;
    b = &schema.bounds;
    b->left = b->right = b->top = b->bottom = BADLATLNG;

    memset(&db, 0, sizeof(gf_db));

    n = 0;
	while (n >= 0) {
		n = getopt_long(argc, argv, "hd:D:o:p:R:l:r:b:t:B:s:n:w:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
        case 'D':
            if ((err = lstat(optarg, &st)) != 0) {
                if (errno == ENOENT) {
                    /* Try to make the directory. */
                    if ((err = mkdir(optarg, 0777)) != 0) {
                        perror("Could not make -D directory");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    perror("Problem with -D option");
                    exit(EXIT_FAILURE);
                }
            }
            else if (S_ISDIR(st.st_mode) == 0) {
                fprintf(stderr, "%s exists and is not a directory\n", optarg);
                exit(EXIT_FAILURE);
            }

            strcpy(schema.dir, optarg);
            break;
        case 'o':
            schema.overlap = atoi(optarg);
            break;
        case 'p':
            strcpy(schema.tile_path_tmpl, optarg);
            break;
        case 'd':
            if ((err = gf_open_db(optarg, &db)) != 0)
                fail("Problem opening gridfloat database\n");
            break;
        case 'R':
            /* Look for 'x' in optarg */
            count = 0;
            while (optarg != NULL) {
                *res_view[count] = atoi(strsep(&optarg, "x"));
                count++;
            }

            if (count > 2) {
                fprintf(stderr, "Bad resolution. Example: '128x256' "
                    "or '128' for 128x128\n");
                exit(EXIT_FAILURE);
            } else if (count == 1) {
                schema.ny_tile = schema.nx_tile;
            }
            break;
        case 'l':
            b->left = atof(optarg);
            break;
        case 'r':
            b->right = atof(optarg);
            break;
        case 'b':
            b->bottom = atof(optarg);
            break;
        case 't':
            b->top = atof(optarg);
            break;
        case 'B':
            count = 0;
            while (optarg != NULL) {
                *b_view[count] = atof(strsep(&optarg, ","));
                count++;
            }

            if (count != 4) {
                fprintf(stderr, "Bad -B option.\n  Example: "
                    "'-113.0,-112.9,42,42.1'\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 's':
            count = 0;
            while (optarg != NULL) {
                wh[count] = atof(strsep(&optarg, "x"));
                count++;
            }

            if (count == 1) {
                wh[1] = wh[0];
            } else if (count > 2) {
                fprintf(stderr, "Bad -s option. Too many params.\n  Examples: "
                    "'-s 1.0x1.0' or just '-s 1.0'.\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'n':
            from_point = 1;
            latlng[0] = atof(optarg);
            break;
        case 'w':
            from_point = 1;
            latlng[1] = -atof(optarg);
            break;
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }


    if (from_point) {
        gf_init_bounds_point(b, latlng[0], latlng[1], wh[0], wh[1]);
    }

    if (schema.dir[0] == 0)
        fail("Need a directory for tiles (-D).\n\n");
    if (schema.tile_path_tmpl[0] == 0)
        fail("Need a relative tile path (-p).\n\n");
    if (db.tiles == NULL)
        fail("Need a path to a gridfloat database (-d).\n\n");
    if (b->left == BADLATLNG ||
        b->right == BADLATLNG ||
        b->top == BADLATLNG ||
        b->bottom == BADLATLNG)
        fail("Must set bounds for tiled area.\n\n");

    gf_build_tile(0, 0, 0, &schema, &db, NULL);
    gf_close_db(&db);
    exit(EXIT_SUCCESS);
}
