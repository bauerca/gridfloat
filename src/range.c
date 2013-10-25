


gf_rangetree_node *gf_build_btree(gf_struct **sorted_gfs, int len) {
    int i, jump, split, prev_split;
    gf_rangetree_node *node;

    printf("build tree at depth %d\n", depth);
    depth++;

    node = gf_get_rangetree_node();
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
            node->children[i - 1] = gf_build_btree(
                sorted_gfs + prev_split + 1, 
                split - prev_split - 1);
        else
            node->children[i - 1] = NULL;

        prev_split = split;
    }
    /* Tree on current comparator is built. */

    depth--;
    return node;
}

/*
We'll be using a combination of 1-D rtrees and a 2-D
range tree with fractional cascading.
*/

void *gf_build_db_tree(gf_db *db) {
    int i;
    gf_rtree_node *xtree;
    gf_struct **sorted_tiles =
        (gf_struct **)malloc(db->count * sizeof(gf_struct *));

    for (i = 0; i < db->count; i++)
        sorted_tiles[i] = &db->tiles[i];

    /* First sort on x-coord of bbox midpoints */
    gf_sort(sorted_tiles, db->count, cmp_midx);

    /* Make tree on x. */
    xtree = gf_build_rtree(sorted_tiles, db->count);

    


}

