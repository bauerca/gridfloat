#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "gridfloat.h"


int print_usage(void) {
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
        "  -R:  Resolution of extraction. If a single integer is\n"
        "       supplied, then the resolution is the same in both\n"
        "       directions. Otherwise, the format is (by example):\n"
        "       '-R 128x256'.\n"
        "  -l:  Left bound for extraction.\n"
        "  -r:  Right bound for extraction.\n"
        "  -b:  Bottom bound for extraction.\n"
        "  -t:  Top bound for extraction.\n"
        "  -B:  Specify extraction bounds using a comma-separated\n"
        "       list of the form LEFT,RIGHT,BOTTOM,TOP. E.g.\n"
        "       '-B -123.112,-123.110,42.100,42.101'.\n"
    );
}

int main(int argc, char *argv[]) {

    int i, count, opt, len;
    float *data;
    int resolution[2] = {128, 128};
    char *fileish;
    struct grid_float gf;
    struct grid_float_bounds bounds;
    double *bounds_view[4] = {&bounds.left, &bounds.right, &bounds.bottom, &bounds.top};
    char flt[2048], hdr[2048];

    while ((opt = getopt(argc, argv, "hR:l:r:b:t:B:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
        case 'R':
            /* Look for 'x' in optarg */
            count = 0;
            while (optarg != NULL) {
                resolution[count] = atoi(strsep(&optarg, "x"));
                count++;
            }

            if (count > 2) {
                fprintf(stderr, "Bad resolution. Example: '128x256' "
                    "or '128' for 128x128\n");
                exit(EXIT_FAILURE);
            } else if (count == 1) {
                resolution[1] = resolution[0];
            }
            break;
        case 'l':
            bounds.left = atof(optarg);
            break;
        case 'r':
            bounds.right = atof(optarg);
            break;
        case 'b':
            bounds.bottom = atof(optarg);
            break;
        case 't':
            bounds.top = atof(optarg);
            break;
        case 'B':
            count = 0;
            while (optarg != NULL) {
                *bounds_view[count] = atof(strsep(&optarg, ","));
                count++;
            }

            if (count != 4) {
                fprintf(stderr, "Bad -B option.\n  Example: "
                    "'-113.0,-112.9,42,42.1'\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
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

        if ((len = strlen(fileish)) > 4) {
            if (!strcmp(fileish + len - 4, ".flt")) {
                strcpy(flt, fileish);
            } else if (!strcmp(fileish + len - 4, ".hdr")) {
                strcpy(hdr, fileish);
            } else {
                strcpy(flt, fileish);
                strcat(flt, ".flt");
                strcpy(hdr, fileish);
                strcat(hdr, ".hdr");
                break;
            }
        }
        optind++;
    }

    if (grid_float_open(hdr, flt, &gf)) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    grid_float_data(&gf, &bounds, resolution[0], resolution[1], &data);
    grid_float_print(resolution[0], resolution[1], data);
    grid_float_close(&gf);

    free(data);

    exit(EXIT_SUCCESS);
}
