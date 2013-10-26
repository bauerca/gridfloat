#ifndef GF_RTREE_H
#define GF_RTREE_H

#define NODE_SIZE 3

/**
 * Gridfloat implementation of a R-Tree.
 * Each level of the R-Tree is an array of nodes. Each node
 * represents the minimum bounding rectangle of its child nodes.
 *
 * The api here does no sorting. It simply provides data structures
 * and a tree-level-building function suitable for recursive
 * calling.
 *
 * The first call to gf_build_rtree_level should be given a set of
 * nodes in the desired sorted order.
 *
 * Gridfloat uses this rtree implementation to setup a Hilbert
 * Packed R-Tree, where the top
 * level (leaf nodes) is sorted by the location of each node's
 * midpoint along the Hilbert curve in 2D.
 */

typedef struct gf_bounds {
    double left;
    double right;
    double bottom;
    double top;
} gf_bounds;

typedef struct gf_rtree_node {
    struct gf_rtree_node *children[NODE_SIZE];
    gf_bounds bounds;
    gf_struct *gf;
} gf_rtree_node;


int gf_build_rtree_level(gf_rtree_node **nodes, int *len);

int gf_search_rtree(gf_bounds *query, gf_rtree_node *root,
    gf_rtree_node **leaves, int *found);

void gf_free_rtree(gf_rtree_node *root);

#endif
