#ifndef GF_RANGE_H
#define GF_RANGE_H

typedef struct gf_rangetree_node {
    gf_struct *gfs[NODE_SIZE - 1];
    struct gf_rtree_node *children[NODE_SIZE];
    struct gf_rtree_node *sibling;
    char leaf;
} gf_rangetree_node;


#endif
