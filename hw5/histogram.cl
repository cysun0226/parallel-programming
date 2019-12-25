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
    if(i % 1000000 == 0){
        // printf("id = %d\n", i);
    }
    //R[data[i].R]++;
    //G[data[i].G]++;
    //B[data[i].B]++;
    atomic_add(&R[data[i].R], 1);
    atomic_add(&G[data[i].G], 1);
    atomic_add(&B[data[i].B], 1);
}