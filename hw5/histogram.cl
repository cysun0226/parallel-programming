#include <CL/cl.h>

typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t align;
} RGB;

typedef struct
{
    bool type;
    uint32_t size;
    uint32_t height;
    uint32_t weight;
    RGB *data;
} Image;


__kernel
void hist_count(
    __global Image *img,
    __global uint32_t *R,
    __global uint32_t *G,
    __global uint32_t *B
)
{
    int i = get_global_id(0);
    RGB &pixel = img->data[i];
    R[pixel.R]++;
    G[pixel.G]++;
    B[pixel.B]++;
}