#ifndef GF_RTREE_H
#define GF_RTREE_H

#define NODE_SIZE 3

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

void gf_free_rtree(gf_rtree_node *root);

#endif
