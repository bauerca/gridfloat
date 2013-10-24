#include "gridfloat.h"
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

static
void gf_sift_down(gf_struct **gfs, int start, int end,
        int (*cmp)(gf_struct *gf1, gf_struct *gf2))
{
    int child, root, node_swap;
    gf_struct *gf_swap;

    root = start;
    child = 2 * root + 1; /* Left child. */

    while (child < end) {
        //printf("%d\n", child);
        node_swap = root;
        /* Swap root for first child? */
        if (cmp(gfs[root], gfs[child]) < 0)
            node_swap = child;
        /* Swap root for second child? */
        if (child + 1 < end && cmp(gfs[node_swap], gfs[child + 1]) < 0)
            node_swap = child + 1;
        /* Require a swap? */
        if (node_swap != root) {
            gf_swap = gfs[node_swap];
            gfs[node_swap] = gfs[root];
            gfs[root] = gf_swap;
            root = node_swap;
            child = 2 * root + 1;
        } else return;
    }
}

/* Heapsort of gf_structs. */
int gf_sort(gf_struct **gfs, int len, int (*cmp)(gf_struct *gf1, gf_struct *gf2)) {
    int start, end;
    gf_struct *gf_swap;
    printf("starting sort\n");

    for (start = (len - 2) / 2; start >= 0; start--)
        gf_sift_down(gfs, start, len, cmp);

    printf("built heap\n");

    for (end = len - 1; end > 0; end--) {
        gf_swap = gfs[end];
        gfs[end] = gfs[0];
        gfs[0] = gf_swap;
        
        gf_sift_down(gfs, 0, end, cmp);
    }

    return 0;
}


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

/*
gf_db must have been loaded. An rtree is a nested btree. That is,
for every subtree of the outermost tree, there is another "sibling"
tree (containing the elements of the subtree) that is sorted along a
different data dimension. For each "sibling" subtree, there is then
another sibling tree, and so on for the remaining dimensions.


*/

gf_rtree_node *gf_get_rtree_node() {
    int i;
    gf_rtree_node *node;

    node = (gf_rtree_node *)malloc(sizeof(gf_rtree_node));
    memset((void *)node, '\0', sizeof(gf_rtree_node));

    return node;
}

static int depth = 0;

gf_rtree_node *gf_build_rtree(gf_struct **sorted_gfs, int len) {
    int i, jump, split, prev_split;
    gf_rtree_node *node;
    /* Traversing given tree will produce array of gf_structs
       */

    printf("build tree at depth %d\n", depth);
    depth++;

    node = gf_get_rtree_node();
    if (len < NODE_SIZE) {
        for (i = 0; i < len; i++)
            node->gfs[i] = sorted_gfs[i];
        depth--;
        return node;
    }
    
    jump = len / NODE_SIZE;
    prev_split = -1;
    for (i = 1; i <= NODE_SIZE; i++) {
        split = (i == NODE_SIZE) ? len : i * jump;
        if (i < NODE_SIZE)
            node->gfs[i - 1] = sorted_gfs[split];

        /* Create new node */
        if (split - prev_split > 1)
            node->children[i - 1] = gf_build_rtree(
                sorted_gfs + prev_split + 1, 
                split - prev_split - 1);
        else
            node->children[i - 1] = NULL;

        prev_split = split;
    }

    depth--;
    return node;
}


void gf_free_rtree(gf_rtree_node *tree) {
    if (tree == NULL) return;
    int i;
    for (i = 0; i < NODE_SIZE; i++)
        gf_free_rtree(tree->children[i]);
    free(tree);
}
