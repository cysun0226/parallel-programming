#include <stdio.h>
#include "omp.h"

int main()
{
    printf("num_procs = %d", omp_get_num_procs());
    return 0;
}

