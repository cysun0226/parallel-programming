#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifndef W
#define W 20                                    // Width
#endif

#define MAIN_RANK 0

typedef struct {
    int up;
    int low;
} Range;

int main(int argc, char **argv) {
  if (argc < 3){
    printf("please input L, iter, seed\n");
    exit(0);
  }

  int L = atoi(argv[1]);                        // Length
  int iteration = atoi(argv[2]);                // Iteration
  srand(atoi(argv[3]));                         // Seed
  
  float d = (float) random() / RAND_MAX * 0.2;  // Diffusivity
  int *temp = malloc(L*W*sizeof(int));          // Current temperature
  int *next = malloc(L*W*sizeof(int));          // Next time step
  int *top = malloc(W*sizeof(int));             // the top bar buffer (exchange with other worker)
  int *bottom = malloc(W*sizeof(int));          // the btm bar buffer (exchange with other worker)

  clock_t begin = clock();


  // ----------------------------------------------------------------------------
  // MPI_Init(&argc,&argv);
  // ----------------------------------------------------------------------------
  MPI_Init(NULL, NULL);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);


  int interval = L / size;
  Range range[size];
  for (size_t i = 0; i < size; i++){
    range[i].low = i*interval;
    range[i].up = (i+1)*interval;
  }

  // seperate this loop by i
  for (int i = 0; i < L; i++) {
    for (int j = 0; j < W; j++) {
      temp[i*W+j] = random()>>3;
    }
  }
  
  int count = 0, balance = 0;
  while (iteration--) {     // Compute with up, left, right, down points
    balance = 1;
    count++;
   for (int i = range[rank].low; i < range[rank].up; i++) {
      //  printf("i=%d\n", i);
      for (int j = 0; j < W; j++) {
        float t = temp[i*W+j] / d; 
        // t = temp[i][j] / d

        t += temp[i*W+j] * -4;   
        // t += temp[i][j] * -4
        
        t += temp[(i - 1 <  0 ? 0 : i - 1) * W + j];
        // t += tmp[i-1][j]

        t += temp[(i + 1 >= L ? i : i + 1)*W+j];
        // t += tmp[i+1][j]

        t += temp[i*W+(j - 1 <  0 ? 0 : j - 1)];
        // t += tmp[i][j-1]

        t += temp[i*W+(j + 1 >= W ? j : j + 1)];
        // t += tmp[i][j+1]

        t *= d;
        // t *= d

        next[i*W+j] = t ;
        // next[i][j] = t

        if (next[i*W+j] != temp[i*W+j]) {
          balance = 0;
        }
      }
   }

   // report balance to master
        MPI_Status status;
        int global_balance = balance;
        int tag = 0;
        if (rank == 0){ // master
          for (size_t i = 1; i < size; i++){
            int local_balance;
            MPI_Recv(&local_balance, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &status);
            if (local_balance == 0){
              global_balance = 0;
            }
          }
        }
        else{ // worker
            MPI_Send(&balance, 1, MPI_INT, MAIN_RANK, tag, MPI_COMM_WORLD);
        }

        // send global result to each worker
        if (rank == 0){ // master
          for (size_t i = 1; i < size; i++){
            MPI_Send(&global_balance, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
          }
        }
        else{ // worker
            MPI_Recv(&global_balance, 1, MPI_INT, MAIN_RANK, tag, MPI_COMM_WORLD, &status);
        }

        // determine if continue
        if (global_balance){
          break;
        }
        
        /* Definition of MPI_SendRecv
        MPI_Sendrecv(*sendbuf, sendcount, MPI_Datatype, dest(rank), sendtag,
                     *recvbuf, recvcount, MPI_Datatype, source(rank), recvtag,
                     MPI_COMM_WORLD, &status)

        int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm, MPI_Request *request)
        int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)

        */

        // send boundary
        if(rank == MAIN_RANK && size >1){
          MPI_Request req_t, req_b, req_s_t, req_s_b;
          MPI_Isend(&next[(range[rank].up-1)*W], W, MPI_INT, rank+1, rank*100+(rank+1), MPI_COMM_WORLD, &req_s_t);
          MPI_Irecv(top, W, MPI_INT, rank+1, (rank+1)*100+rank, MPI_COMM_WORLD, &req_t);
          MPI_Wait(&req_t, &status);

          for (size_t i = 0; i < W; i++){
            next[(range[rank].up)*W + i] = top[i];
          }
        }

        if(rank == size-1  && size >1){
          MPI_Request req_t, req_b, req_s_t, req_s_b;
          MPI_Isend(&next[range[rank].low*W], W, MPI_INT, rank-1, rank*100+(rank-1), MPI_COMM_WORLD, &req_s_b);
          MPI_Irecv(bottom, W, MPI_INT, rank-1, (rank-1)*100+rank, MPI_COMM_WORLD, &req_b);
          MPI_Wait(&req_b, &status);
          
          for (size_t i = 0; i < W; i++){
            next[(range[rank].low-1)*W + i] = bottom[i];
          }
        }


        if (rank != size-1 && rank != MAIN_RANK && size >1){
          // send the bottom row
          MPI_Request req_t, req_b, req_s_t, req_s_b;

          MPI_Isend(&next[(range[rank].up-1)*W], W, MPI_INT, rank+1, rank*100+(rank+1), MPI_COMM_WORLD, &req_s_t);
          MPI_Isend(&next[range[rank].low*W], W, MPI_INT, rank-1, rank*100+(rank-1), MPI_COMM_WORLD, &req_s_b);
          MPI_Irecv(bottom, W, MPI_INT, rank-1, (rank-1)*100+rank, MPI_COMM_WORLD, &req_b);
          MPI_Irecv(top, W, MPI_INT, rank+1, (rank+1)*100+rank, MPI_COMM_WORLD, &req_t);

          MPI_Wait(&req_t, &status);
          MPI_Wait(&req_b, &status);

          // update temp according to the received data
          for (size_t i = 0; i < W; i++){
            next[(range[rank].low-1)*W + i] = bottom[i];
            next[(range[rank].up)*W + i] = top[i];
          }
        }


    // print the temp array
    /*
    if (rank == 1){
    printf("\n === iter %d === \n", iteration);
    for (size_t i = 0; i < L; i++){
      for (size_t j = 0; j < W; j++){
        printf("%d ", next[i*W+j]);
      }
      printf("\n");
    }
    }
    */
    
    int *tmp = temp;
    temp = next;
    next = tmp;

    MPI_Barrier(MPI_COMM_WORLD);
  }

  int min = temp[range[rank].low*W];
  int tag = 0;
  for (int i = range[rank].low; i < range[rank].up; i++) {
    for (int j = 0; j < W; j++) {
      if (temp[i*W+j] < min) {
        min = temp[i*W+j];
      }
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // find min
  MPI_Status status;
  int global_min = min;
  if (rank == MAIN_RANK){ // master
    for (size_t i = 1; i < size; i++){
      int part_min;
      MPI_Recv(&part_min, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &status);
      if(part_min < global_min){
        global_min = part_min;
      }
    }
  }
  else{ // worker
    MPI_Send(&min, 1, MPI_INT, MAIN_RANK, tag, MPI_COMM_WORLD);
  }
  
  if (rank == MAIN_RANK){
    clock_t end = clock();  
    printf("Size: %d*%d, Iteration: %d, Min Temp: %d\n", L, W, count, global_min);
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("time: %4f sec\n", time_spent);
  }

  // exit MPI
  MPI_Finalize();
  free(temp);
  free(next);
  free(top);
  free(bottom);
  
  return 0;
}

