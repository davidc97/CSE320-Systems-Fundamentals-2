#include <stdio.h>
#include "sfmm.h"
#include "sfmm2.h"

int main(int argc, char const *argv[]) {

    sf_mem_init();
    void *x = sf_malloc(sizeof(double) * 10);
    sf_realloc(x, 5);
    void *y = sf_malloc(1000);
    sf_free(x);
    void *a = sf_malloc(500);
    void *b = sf_malloc(100);
    sf_realloc(a, 50);
    void *c = sf_realloc(b, 1000);
    sf_free(c);
    sf_free(a);
    sf_free(y);
    sf_snapshot();
    sf_mem_fini();
    //sf_snapshot();
    return EXIT_SUCCESS;
}
