typedef struct
{
    uchar R;
    uchar G;
    uchar B;
    uchar align;
} RGB;

typedef struct
{
    bool type;
    uint size;
    uint height;
    uint weight;
    RGB *data;
} Image;

kernel void hist_count(
    global RGB *data,
    global uint *R,
    global uint *G,
    global uint *B
)
{
    int i = get_global_id(0);
    R[data[i].R]++;
    G[data[i].G]++;
    B[data[i].B]++;
}