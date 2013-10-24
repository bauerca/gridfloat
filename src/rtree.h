#ifndef GF_RTREE_H
#define GF_RTREE_H

#define NODE_SIZE 3

typedef struct gf_rtree_node {
    gf_struct *gfs[NODE_SIZE - 1];
    struct gf_rtree_node *children[NODE_SIZE];
    struct gf_rtree_node *sibling;
    char leaf;
} gf_rtree_node;

int gf_rtree_insert(gf_struct *gf, gf_rtree_node *root);

int gf_sort(gf_struct **gfs, int len,
        int (*cmp)(gf_struct *gf1, gf_struct *gf2));


gf_rtree_node *gf_get_rtree_node();

gf_rtree_node *gf_build_rtree(gf_struct **sorted_gfs, int len);

void gf_free_rtree(gf_rtree_node *tree);

#endif
