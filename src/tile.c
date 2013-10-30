#include "gridfloat.h"
#include "linear.h"
#include "db.h"
#include "rtree.h"
#include "tile.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Uses the schema to build the path to a tile as a string (placed in
 * path parameter) AND creates any needed directories.
 */
int gf_get_tile_path(const gf_tile_schema *schema, int ix, int iy, int iz, char *path) {
    int pos, vlen, dirlen;
    char c, buf[32], *cp;
    const char *tmpl = schema->tile_path_tmpl;

    strcpy(path, schema->dir);
    dirlen = strlen(path);
    if (path[dirlen - 1] != '/') {
        strcat(path, "/");
        dirlen++;
    }

    pos = tmpl[0] == '/' ? 1 : 0;
    while ((c = tmpl[pos]) != '\0') {
        buf[0] = '\0';

        switch (c) {
        case '[':
            if (strncmp(tmpl + pos, "[x]", 3) == 0) {
                sprintf(buf, "%d", ix);
                vlen = 3;
            }
            if (strncmp(tmpl + pos, "[y]", 3) == 0) {
                sprintf(buf, "%d", iy);
                vlen = 3;
            }
            if (strncmp(tmpl + pos, "[z]", 3) == 0) {
                sprintf(buf, "%d", iz);
                vlen = 3;
            }

            if (strlen(buf) > 0) {
                pos += vlen;
                break;
            }
            // Else fall thru.
        default:
            sprintf(buf, "%c", c);
            pos++;
        }
        strcat(path, buf);
    }

    /* Make directories to tile. */
    for (cp = path + dirlen; *cp; cp++) {
        if (*cp == '/') {
            *cp = 0;
            mkdir(path, 0777); //S_ISVTX);
            *cp = '/';
        }
    }

    return 0;
}


/**
 * Build tile at position (ix,iy) and zoom level iz. This is
 * a recursive function. If called as,
 *
 *     gf_build_tile(0, 0, 0, 0, schema, db)
 *
 * it will build your entire tiled database with all possible
 * (reasonable) zoom levels.
 *
 * A tile is defined by a bounds and resolution, like a grid.
 * However, there is an important difference in interpretation.
 * The following illustrates a tile with 3x3 resolution; each
 * 'o' is a datapoint.
 *
 *    -------------------------------------
 *    |           |           |           |
 *    |           |           |           |
 *    |     o     |     o     |     o     |
 *    |           |           |           |
 *    |           |           |           |
 *    -------------------------------------
 *    |           |           |           |
 *    |           |           |           |
 *    |     o     |     o     |     o     |
 *    |           |           |           |
 *    |           |           |           |
 *    -------------------------------------
 *    |           |           |           |
 *    |           |           |           |
 *    |     o     |     o     |     o     |
 *    |           |           |           |
 *    |           |           |           |
 *    -------------------------------------
 *
 * Notice that the bbox of the grid (of data points) is smaller
 * than the bbox of the tile by half a cell on each side.
 * This prevents data overlap for neighboring tiles (if the schema
 * overlap is set to zero).
 *
 * If the current tile is at a higher resolution than all data
 * found within its bounds in the db, then this function will
 * fill the tile with data from the db, interpolating as necessary.
 * Otherwise, this function will seek data from the four tiles it
 * bounds at the next higher zoom level,.
 *
 * @param xtra Layers of cells outside target bbox. These values
 *      should be returned to the caller, but not saved in the
 *      tile.
 *
 * Returns -1 if no data intersects the requested tile.
 */
int gf_build_tile(int ix, int iy, int iz, gf_tile_schema *schema,
    gf_db *db, float *out)
{

    int i, pad, nx, ny, nq, found, subdivide, free_out;
    double w, h, dx, dy;
    gf_bounds b, *sb;
    gf_struct *gf;
    float *quads;
    gf_rtree_node **nodes;
    gf_grid dgrid, tgrid;


    sb = &schema->bounds;

    /* Data resolution. Every successive zoom level needs another
    layer of pad cells for interpolation at the edges of each parent
    level's encapsulating tile. See quad_interp function desc. */
    pad = schema->overlap + iz;
    nx = schema->nx_tile + 2 * pad;
    ny = schema->ny_tile + 2 * pad;

    printf("building tile x=%d y=%d z=%d\n", ix, iy, iz);
    printf("  dims nx=%d ny=%d\n", nx, ny);

    /* Width/Height of tile (NOT data grid) w/out pad surrounding
    cells. This is so we can calculate the cell size at this zoom
    level. */
    w = (sb->right - sb->left) / ((double)pow(2, iz));
    h = (sb->top - sb->bottom) / ((double)pow(2, iz));
    /* Cell width/height. Crump. */
    dx = w / ((double)schema->nx_tile);
    dy = h / ((double)schema->ny_tile);

    /* Bounding box of data request including pad surrounding cells.
    Remember this is the bbox around the data points. Make sure to
    account for half-cell insets. */
    b.left = sb->left + ((double)ix) * w - (((double)pad) - 0.5) * dx;
    b.top = sb->top - ((double)iy) * h + (((double)pad) - 0.5) * dy;
    b.right = b.left + ((double)(nx - 1)) * dx;
    b.bottom = b.top - ((double)(ny - 1)) * dy;

    /* Search for the data intersecting this tile. */
    nodes = (gf_rtree_node **)malloc(db->count * sizeof(gf_rtree_node *));
    gf_search_rtree(&b, db->tree, nodes, &found);
    if (found == 0) {
        free(nodes);
        return -1;
    }

    free_out = 0;
    if (out == NULL) {
        out = (float *)malloc(nx * ny * sizeof(float));
        free_out = 1;
    }

    /* See if any found data has a finer resolution than this tile. */
    subdivide = 0;
    for (i = 0; i < found; i++) {
        gf = nodes[i]->gf;

        if (gf->grid.dx < dx || gf->grid.dy < dy) {
            subdivide = 1;
            free(nodes);
            break;
        }
    }

    if (subdivide) {
        /* Data is at a finer resolution; to combat aliasing,
        let's smooth some data from the next zoom level. */

        nq = (nx + 2) * (ny + 2);
        quads = (float *)malloc(4 * nq * sizeof(float));
        memset(quads, 0, 4 * nq * sizeof(float));

        for (i = 0; i < 4; i++) {
            gf_build_tile(
                2 * ix + i % 2,
                2 * iy + i / 2,
                iz + 1, schema, db,
                quads + i * nq);
        }
        quad_interp(quads, nx, ny, out);
        free(quads);
    }
    else {
        /* Tile is at a finer resolution; interpolate from data. */

        gf_init_grid_bounds(&dgrid, b.left, b.right, b.bottom, b.top, ny, nx);
        for (i = 0; i < found; i++)
            gf_bilinear_interpolate(nodes[i]->gf, &dgrid, out);

        free(nodes);
    }

    /* Grid describing tile data to be saved to disk. That is, without
    padding cells. */
    gf_init_grid_bounds(&tgrid,
        b.left + pad * dx,
        b.right - pad * dx,
        b.bottom + pad * dy,
        b.top - pad * dy,
        schema->ny_tile,
        schema->nx_tile);

    gf_save_tile(schema, &tgrid, ix, iy, iz, pad, out);

    if (free_out) free(out);

    return 0;
}

/* 
 * In the pic below, we are interpolating + smoothing from o's to
 * X's.
 *
 *                         |----------q2-------------|
 *       |------------q1-----------|                 |
 *       | o       o       o       o       o       o |
 *       |                                           | 
 *       |                                           | 
 *       |                                           | 
 *       | o       o       o       o       o       o |
 *       |                                           | 
 *       |             X------out------X             |
 *       |             |               |             |
 *       | o       o   |   o       o   |   o       o |
 *       |             |               |             |
 *       |             |               |             |
 *       |             |               |             |
 *       - o       o   |   o       o   |   o       o -
 *                     |               |
 *                     X---------------X
 *        
 *         o       o       o       o       o       o
 *        
 *        
 *        
 *         o       o       o       o       o       o
 *
 * @param quads (in) Four concatenated arrays of data. Each array should be
 *      (nx + 2)*(ny + 2) long and hold data for one of the quadrants from
 *      which we interpolate. Concatenated in the following order: upper-left,
 *      upper-right, lower-left, lower-right.
 * @param nx (in) Resolution in x of the output array.
 * @param ny (in) Resolution in y of the output array.
 * @param out (out) Array of data to be filled. Should be
 *      (2*nx + 2*pad)*(2*ny + 2*pad) long.
 */

#define SMOOTH_SIDES 0.5
#define SMOOTH_CORNERS 0.25
int quad_interp(float *quads, int nx, int ny, float *out) {
    int i, j, ii, jj, ffi, ffj, qx, qy, nxq, nyq;
    float nine[3][3];
    float *quad;
    double a, b;

    a = SMOOTH_SIDES;
    b = SMOOTH_CORNERS;

    nxq = nx + 2;
    nyq = ny + 2;

    /* (i,j) is a position in the output grid. */
    for (i = 0; i < nx; i++) {
        /* Left or right quadrants? */
        qx = 2 * i / nx;

        /* Index (in quad array) of leftmost values in 4x4 grid
        surrounding current (i,j) interpolation point. */
        ffi = 2 * i - qx * nx;

        for (j = 0; j < ny; j++) {
            /* Top or bottom quadrant? */
            qy = 2 * j / ny;
            quad = quads + (2 * qy + qx) * nxq;
            //printf("qi=%d\n", 2 * qy + qx);

            /* Index (in quad array) of topmost values in 4x4 grid
            surrounding current (i,j) interpolation point. */
            ffj = 2 * j - qy * ny;

            for (ii = 0; ii < 3; ii++)
                for (jj = 0; jj < 3; jj++) {
                    //printf("%d, ", (ffj + jj    ) * nxq + (ffi + ii    ));
                    //printf("%d, ", (ffj + jj + 1) * nxq + (ffi + ii + 1));
                    nine[ii][jj] = 0.25 * (
                        quad[(ffj + jj    ) * nxq + (ffi + ii    )] +
                        quad[(ffj + jj + 1) * nxq + (ffi + ii    )] +
                        quad[(ffj + jj    ) * nxq + (ffi + ii + 1)] +
                        quad[(ffj + jj + 1) * nxq + (ffi + ii + 1)]
                    );
                }

            out[j * nx + i] = (1.0 - a - b) * nine[1][1] +
                0.25 * a * (nine[0][1] + nine[1][0] + nine[1][2] + nine[2][1]) +
                0.25 * b * (nine[0][0] + nine[0][2] + nine[2][0] + nine[2][2]);
        }
    }

    return 0;
}



int gf_save_tile(const gf_tile_schema *schema, gf_grid *tgrid,
    int ix, int iy, int iz, int pad, float *data)
{
    
    FILE *flt;
    char filename[256];
    int i, nx, ny, len, offset, nx_data, extra;

    gf_get_tile_path(schema, ix, iy, iz, filename);

    strcat(filename, ".hdr");
    if (gf_write_hdr(tgrid, filename)) {
        fprintf(stderr, "Could not open %s for writing.\n", filename);
        return -1;
    }

    filename[strlen(filename) - 4] = 0;
    strcat(filename, ".flt");

    flt = fopen(filename, "wb");
    if (flt == NULL) {
        fprintf(stderr, "Could not open %s for writing.\n", filename);
        return -1;
    }

    nx_data = schema->nx_tile + 2 * pad;
    extra = pad - schema->overlap;

    /* Resolution of tile with overlap. */
    nx = schema->nx_tile + 2 * schema->overlap;
    ny = schema->nx_tile + 2 * schema->overlap;

    for (i = 0; i < ny; i++)
        fwrite((void *)(data + nx_data * (extra + i) + extra),
            sizeof(float), nx, flt);

    fclose(flt);

    return 0;
}


/* First tile at zoom 0 is bbox of tiled area.
But, we cannot build this tile until we have built
zoom level 1 b/c we are smoothing data from
highest resolution to lowest. */


/* Every zoom level doubles the resolution. */

/** Building the max zoom level. **/
/* Start at upper-left corner. Keep zooming until
we find the tile spec that has a resolution higher
than the resolution of the src data, or determine
that the tile is completely out of src data bounds. */
