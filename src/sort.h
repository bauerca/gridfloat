#ifndef GF_SORT_H
#define GF_SORT_H

int gf_sort(gf_struct **gfs, int len,
        int (*cmp)(gf_struct *gf1, gf_struct *gf2));

int hilbert_cmp(gf_struct *gf1, gf_struct *gf2);

#endif
