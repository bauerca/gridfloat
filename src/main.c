#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "gridfloat.h"
#include "linear.h"
#include "gfpng.h"
#include "gfstl.h"


void print_usage(void) {
    fprintf(stdout,
        "Summary:\n"
        "  Extract subgrids of data from a GridFloat file. GridFloat\n"
        "  is a simple format commonly used by the USGS for packaging\n"
        "  elevation data. A header file with geolocation metadata is\n"
        "  required for making queries based on latitude/longitude.\n"
        "\n"
        "Usage:\n"
        "  gridfloat [options] FLOATFILE HEADERFILE\n"
        "  gridfloat [options] FLOAT_HEADER_PREFIX\n"
        "\n"
        "  where, for the first case, FLOATFILE should have\n"
        "  extension .flt and HEADERFILE .hdr. If both HEADERFILE\n"
        "  and FLOATFILE have the same filename prefix, then you\n"
        "  can use the second case.\n"
        "\n"
        "Options:\n"
        "  -h:  Print this help message.\n"
        "  -i:  Print helpful info derived from GridFloat header file.\n"
        "  -R:  Resolution of extraction. If a single integer is\n"
        "       supplied, then the resolution is the same in both\n"
        "       directions. Otherwise, the format is (by example):\n"
        "       '-R 128x256' where the first number is the resolution\n"
        "       in the x-direction (along lines of latitude).\n"
        "  -l:  Left bound for extraction.\n"
        "  -r:  Right bound for extraction.\n"
        "  -b:  Bottom bound for extraction.\n"
        "  -t:  Top bound for extraction.\n"
        "  -B:  Specify extraction bounds using a comma-separated\n"
        "       list of the form LEFT,RIGHT,BOTTOM,TOP. E.g.\n"
        "       '-B -123.112,-123.110,42.100,42.101'.\n"
        "  -n:  Longitude midpoint of extraction. Must use with '-s'\n"
        "       option. Example: '-n 42'.\n"
        "  -w:  Latitude midpoint of extraction. Must use with '-s'\n"
        "       option. Example: '-w 123' (notice there is no minus\n"
        "       sign here).\n"
        "  -p:  Lat/Lng midpoint of extraction. Must use with '-s'\n"
        "       option. Example: '-p -123,42'.\n"
        "  -s:  Size of box (in degrees) around midpoint. Used with '-p'\n"
        "       and '-n','-w' options. Examples: '-s 1.0x1.0' or just '-s 1'.\n"
        "       If a rectangular size is requested, the first number refers\n"
        "       to the width of the box (along x; i.e. along lines of\n"
        "       latitude).\n"
        "  -T:  Transpose and invert along y before printing the array\n"
        "       (so that a[i, j] gives longitude increasing with i and\n"
        "       latitude increasing with j).\n"
        "  -o:  Output subgrid data to a file. Detects output format based\n"
        "       on file extension. Supported: (.png, .stl). For a .png\n"
        "       extension, see \"PNG output options\" below. Otherwise,\n"
        "       gridfloat will assume you want to save another GridFloat\n"
        "       file. In this case, it will write the appropriate data\n"
        "       and header files, appending .flt and .hdr, respectively,\n"
        "       to the argument of -o.\n"
        "\n"
        "PNG output options:\n"
        "  When png output is specified, gridfloat automatically renders\n"
        "  a shaded relief map. The following options determine which\n"
        "  slopes are sunny.\n"
        "\n"
        "  -A:  Azimuthal angle (in degrees) of view toward sun (for\n"
        "       relief shading). 0 means sun is East, 90 North, etc.\n"
        "       Default: 45 (NE).\n"
        "  -P:  Polar angle (in degrees) of view toward sun (for\n"
        "       relief shading). 0 means sun is on horizon, 90 when\n"
        "       directly overhead. Default: 30.\n"
        "\n"
        "Examples:\n"
        "  All of the following are equivalent and simply print data\n"
        "  to stdout.\n"
        "\n"
        "  gridfloat -l -123 -r -122 -b 42 -t 43 file.{hdr,flt}\n"
        "  gridfloat -B -123,-122,42,43 file.{hdr,flt}\n"
        "  gridfloat -p -122.5,42.5 -s 1 file.{hdr,flt}\n"
        "  gridfloat -w 122.5 -n 42.5 -s 1x1 file.{hdr,flt}\n"
        "\n"
    );
}

const float BAD_LATLNG = -1000.0;

int main(int argc, char *argv[]) {

    int count, opt, len;
    gf_float *data;
    char *fileish;
    char flt[2048], hdr[2048], savename[2048];
    gf_struct gf;
    gf_grid *from_grid = &gf.grid;

    /* Extraction grid */
    gf_grid to_grid;
    double *b_view[4] = {&to_grid.left, &to_grid.right, &to_grid.bottom, &to_grid.top};
    int *res_view[2] = {&to_grid.nx, &to_grid.ny};
    double latlng[2] = {BAD_LATLNG, BAD_LATLNG};
    double wh[2] = {0, 0}; /* Width-Height */
    int info = 0, from_point = 0, xy = 0, save = 0;
    double n_sun[3];
    double polar = 30.0, azimuth = 45.0;

    to_grid.nx = to_grid.ny = 128;

    while ((opt = getopt(argc, argv, "hiTR:l:r:b:t:B:p:n:w:s:o:P:A:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
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
                to_grid.ny = to_grid.nx;
            }
            break;
        case 'l':
            to_grid.left = atof(optarg);
            break;
        case 'r':
            to_grid.right = atof(optarg);
            break;
        case 'b':
            to_grid.bottom = atof(optarg);
            break;
        case 't':
            to_grid.top = atof(optarg);
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
        case 'p':
            from_point = 1;
            count = 0;
            while (optarg != NULL) {
                latlng[count] = atof(strsep(&optarg, ","));
                count++;
            }

            if (count != 2) {
                fprintf(stderr, "Bad -p option. Need lat and lng.\n  Example: "
                    "'-113.0,-112.9'\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'i':
            info = 1;
            break;
        case 'T':
            xy = 1;
            break;
        case 'o':
            save = 1;
            strcpy(savename, optarg);
            break;
        case 'P':
            polar = atof(optarg);
            break;
        case 'A':
            azimuth = atof(optarg);
            break;
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }


    if (from_point) {
        gf_init_grid_point(&to_grid, latlng[0], latlng[1], wh[0], wh[1], to_grid.ny, to_grid.nx);
    } else {
        /* Just calculate dx, dy. */
        gf_init_grid_bounds(&to_grid, to_grid.left, to_grid.right, to_grid.bottom, to_grid.top, to_grid.ny, to_grid.nx);
    }


    /* Final unhandled args are gridfloat and header filename pair
       or the shared prefix (no extension) of both gridfloat and
       header filenames. */

    if (argc - optind == 0 || argc - optind > 2) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    while (optind < argc) {
        fileish = argv[optind];
        len = strlen(fileish);

        if (len > 4 && !strcmp(fileish + len - 4, ".flt")) {
            strcpy(flt, fileish);
        } else if (len > 4 && !strcmp(fileish + len - 4, ".hdr")) {
            strcpy(hdr, fileish);
        } else {
            strcpy(flt, fileish);
            strcat(flt, ".flt");
            strcpy(hdr, fileish);
            strcat(hdr, ".hdr");
            break;
        }
        optind++;
    }


    if (gf_open(hdr, flt, &gf)) {
        fprintf(stderr, "Failed to open %s or %s.\n\n", hdr, flt);
        print_usage();
        exit(EXIT_FAILURE);
    }

    if (info) {
        fprintf(stdout, "data file: %s\nheader file: %s\n", flt, hdr);
        gf_print_grid_info(from_grid);
    } else if (save) {
        len = strlen(savename);
        if (len > 4 && !strcmp(savename + len - 4, ".png")) {
            polar *= PI / 180.0;
            azimuth *= PI / 180.0;

            n_sun[0] = cos(polar) * cos(azimuth);
            n_sun[1] = cos(polar) * sin(azimuth);
            n_sun[2] = sin(polar);
            gf_relief_shade(&gf, &to_grid, n_sun, savename);
        } else if (len > 4 && !strcmp(savename + len - 4, ".stl")) {
            data = (gf_float *)malloc(to_grid.nx * to_grid.ny * sizeof(gf_float));
            gf_bilinear_interpolate(&gf, &to_grid, data);
            gf_save_stl(&to_grid, data, savename);
            free(data);
        } else {
            data = (gf_float *)malloc(to_grid.nx * to_grid.ny * sizeof(gf_float));
            gf_bilinear_interpolate(&gf, &to_grid, data);
            gf_save(&to_grid, data, savename);
            free(data);
        }

        exit(EXIT_SUCCESS);
    } else {
        data = (gf_float *)malloc(to_grid.nx * to_grid.ny * sizeof(gf_float));
        gf_bilinear_interpolate(&gf, &to_grid, data);
        gf_print(&to_grid, data, xy);
        free(data);
    }

    gf_close(&gf);
    exit(EXIT_SUCCESS);
}
