#include "gridfloat.h"
#include "sort.h"
#include "rtree.h"
#include <stdlib.h>
#include <string.h>

/* Returns the node containing the child that is immediately
   above (w.r.t. the given comparator) the given gf_struct.
int gf_rtree_search(gf_struct *gf, gf_rtree_node *root,
        int (*cmp)(gf_struct *gf1, gf_struct *gf2),
        gf_rtree_node **below_node, int *below_child)
{
    gf_rtree_node *node, *child, *below, *above;
    gf_struct *child_gf;
    int i, last;

    while (!node->leaf) {
        
        last = node->count - 1;
        for (i = 0, child = NULL; i < last; i++) {
            if (cmp(gf, &node->gfs[i]) < 0) {
                child = node->children[i];
                break;
            }
        }
        if (child == NULL) {
            child = node->children[last];
            i = last;
        }

        node = child;
    }


}

int gf_rtree_insert(gf_struct *gf, gf_rtree_node *root) {
    gf_grid *grid = &gf->grid;

}
*/



#define CMP(side) \
    static \
    int cmp_##side(gf_struct *tile1, gf_struct *tile2) { \
        if (tile1->grid.side < tile2->grid.side) return -1; \
        else if (tile1->grid.side == tile2->grid.side) return 0; \
        else return 1; \
    }

CMP(left)
CMP(right)
CMP(top)
CMP(bottom)

static
int cmp_midx(gf_struct *tile1, gf_struct *tile2) {
    double midx1 = 0.5 * (tile1->grid.left + tile1->grid.right),
           midx2 = 0.5 * (tile2->grid.left + tile2->grid.right);

    if (midx1 < midx2) return -1;
    else if (midx1 == midx2) return 0;
    else return 1;
}

static
int cmp_midy(gf_struct *tile1, gf_struct *tile2) {
    double midy1 = 0.5 * (tile1->grid.left + tile1->grid.right),
           midy2 = 0.5 * (tile2->grid.left + tile2->grid.right);

    if (midy1 < midy2) return -1;
    else if (midy1 == midy2) return 0;
    else return 1;
}

/*
gf_db must have been loaded. An rtree is a nested btree. That is,
for every subtree of the outermost tree, there is another "sibling"
tree (containing the elements of the subtree) that is sorted along a
different data dimension. For each "sibling" subtree, there is then
another sibling tree, and so on for the remaining dimensions.


*/

static int nodes_allocd = 0;

gf_rtree_node *gf_get_rtree_node() {
    int i;
    gf_rtree_node *node;

    node = (gf_rtree_node *)malloc(sizeof(gf_rtree_node));
    memset((void *)node, '\0', sizeof(gf_rtree_node));

    nodes_allocd++;
    return node;
}

static int depth = 0;


/* Builds the level of nodes below the level passed in as
 * <nodes>. Each node of the new level is formed by grouping
 * NODE_SIZE nodes from the passed in set. It is assumed that
 * the given nodes array is sorted. The sort order should be one
 * that correlates adjacency in the array with spatial proximity.
 *
 * @param nodes (in/out) Pointer to an array of nodes. On function exit,
 *      the array will be replaced by a new array of nodes
 *      representing the next level in the tree.
 * @param len (in/out) On input, the address of the length of the <nodes>
 *      array. On exit, the length will be overwritten with the length
 *      of the newly created level (the output <nodes> array).
 */
int gf_build_rtree_level(gf_rtree_node **nodes, int *len) {
    int i, j, ii, n1, n2;
    gf_rtree_node *node, *child, *nodes1, *nodes2;
    gf_bounds *nb, *cb;

    n1 = *len;
    n2 = (n1 + NODE_SIZE - 1) / NODE_SIZE;

    nodes1 = *nodes;
    nodes2 = (gf_rtree_node *)malloc(n2 * sizeof(gf_rtree_node));
    memset((void *)nodes2, 0, n2 * sizeof(gf_rtree_node));

    //printf("current level: %d nodes -> new level: %d nodes\n", n1, n2);

    ii = 0;
    for (i = 0; i < n2; i++) {
        node = nodes2 + i;

        for (j = 0; j < NODE_SIZE; j++) {
            if (ii == n1)
                break;

            //printf("  processing old node %d\n", ii);
            node->children[j] = child = &nodes1[ii++];
            cb = &child->bounds;
            nb = &node->bounds;

            if (j == 0)
                memcpy(nb, cb, sizeof(gf_bounds));
            else {
                if (cb->left < nb->left) nb->left = cb->left;
                if (cb->right > nb->right) nb->right = cb->right;
                if (cb->bottom < nb->bottom) nb->bottom = cb->bottom;
                if (cb->top > nb->top) nb->top = cb->top;
            }
        }
        //printf("made node with bbox: %f, %f, %f, %f\n",
        //    nb->left, nb->right, nb->bottom, nb->top);
    }

    *nodes = nodes2;
    *len = n2;

    return 0;
}

static int compares = 0;
static int sdepth = 0;

/* @param leaves (out) Should be preallocated array of pointers with
 *      length at least as large as the number of leaves in the
 *      tree.
 * @param found (out) Number of leaves found and placed in given
 *      leaves array.
 */
int gf_search_rtree(gf_bounds *query, gf_rtree_node *root,
    gf_rtree_node **leaves, int *found)
{
    int i, child_found;
    gf_rtree_node *child;
    gf_bounds *b;

    *found = 0;

    /* Test for intersection. */
    b = &root->bounds;
    //printf("compares = %d\n", ++compares);
    //printf("depth: %d, bbox: %f, %f, %f, %f\n", sdepth,
    //    b->left, b->right, b->bottom, b->top);
    if (query->right >= b->left && query->left <= b->right &&
        query->bottom <= b->top && query->top >= b->bottom) {
        //printf("  intersection! %p\n", root->gf);
        
        if (root->gf != NULL) {
            //printf("  leaf node. adding to results.\n");
            leaves[0] = root;
            *found = 1;
            sdepth--;
            return 0;
        }

        for (i = 0; i < NODE_SIZE; i++) {
            if ((child = root->children[i]) == NULL)
                break;

            //printf("  checking child...\n");
            sdepth++;
            gf_search_rtree(query, child, leaves + *found,
                &child_found);

            *found += child_found;
        }

    } else
        //printf("  no intersection!\n");

    sdepth--;
    return 0;
}


/* Only call this with the root node as argument! */
void gf_free_rtree(gf_rtree_node *tree) {
    if (tree == NULL) return;
    /* Delete entire level above. */
    if (tree->children[0] != NULL)
        gf_free_rtree(tree->children[0]);
    free(tree);
}




