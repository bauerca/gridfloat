#include "gridfloat.h"
#include "linear.h"
#include "db.h"
#include "rtree.h"
#include "tile.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>



/**
 * Build tile at position (ix,iy) and zoom level iz. This is
 * a recursive function. If called as,
 *
 *     gf_build_tile(0, 0, 0, 0, schema, db)
 *
 * it will build your entire tiled database with all possible
 * (reasonable) zoom levels.
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
 * @param nodes Pre-allocated array of rtree nodes. Length should
 *      equal the number of leaf nodes in the database. This is
 *      is used to hold those nodes in the database that intersect
 *      the current tile bounds.
 *
 * Returns -1 if no data intersects the requested tile.
 */
int gf_build_tile(int ix, int iy, int iz, int extra, tile_schema *schema,
    gf_db *db, float *out)
{

    int i, nx, ny, nq, found, subdivide;
    double w, h, dx, dy;
    gf_bounds b, *sb;
    gf_struct *gf;
    float *quads;
    gf_rtree_node **nodes;

    sb = &schema->bounds;

    nx = schema->nx_tile + 2 * extra;
    ny = schema->ny_tile + 2 * extra;

    /* Width/Height of tile w/out extra surrounding cells. */
    w = (sb->right - sb->left) / ((double)pow(2, iz));
    h = (sb->top - sb->bottom) / ((double)pow(2, iz));
    /* Cell width/height. */
    dx = w / ((double)schema->nx_tile);
    dy = h / ((double)schema->ny_tile);

    /* Bounding box of request including extra surrounding cells. */
    b.left = sb->left + ((double)ix) * w - ((double)extra) * dx;
    b.top = sb->top - ((double)iy) * h + ((double)extra) * dy;
    b.right = b.left + w + 2.0 * ((double)extra) * dx;
    b.bottom = b.top - h - 2.0 * ((double)extra) * dx;

    /* Search for the data intersecting this tile. */
    nodes = (gf_rtree_node **)malloc(db->count * sizeof(gf_rtree_node *));
    gf_search_rtree(&b, db->tree, nodes, &found);
    if (found == 0) {
        return -1;
    }

    /* See if any found data has a finer resolution than this tile. */
    for (i = 0; i < found; i++) {
        gf = nodes[i]->gf;

        if (gf->grid.dx < dx || gf->grid.dy < dy) {
            subdivide = 1;
            break;
        }
    }

    /* Data is at a finer resolution; since we don't want aliasing,
    let's smooth some data from the next zoom level. */
    if (subdivide) {
        nq = (nx + 2) * (ny + 2);
        quads = (float *)malloc(4 * nq * sizeof(float));
        memset(quads, 0, 4 * nq * sizeof(float));

        for (i = 0; i < 4; i++) {
            gf_build_tile(
                2 * ix + i % 2,
                2 * iy + i / 2,
                iz + 1, extra + 1, schema, db,
                quads + i * nq);
        }
        quad_interp(quads, nx, ny, out);
        free(quads);
    }
    else {



    }

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
 *      (2*nx + 2*extra)*(2*ny + 2*extra) long.
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
            printf("qi=%d\n", 2 * qy + qx);

            /* Index (in quad array) of topmost values in 4x4 grid
            surrounding current (i,j) interpolation point. */
            ffj = 2 * j - qy * ny;

            for (ii = 0; ii < 3; ii++)
                for (jj = 0; jj < 3; jj++) {
                    printf("%d, ", (ffj + jj    ) * nxq + (ffi + ii    ));
                    printf("%d, ", (ffj + jj + 1) * nxq + (ffi + ii + 1));
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
