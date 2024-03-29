/**********************************************************************
 * DESCRIPTION:
 *   Serial Concurrent Wave Equation - C Version
 *   This program implements the concurrent wave equation
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// #define CHECK_TIME

#define MAXPOINTS 1000000
#define MAXSTEPS 1000000
#define MINPOINTS 20
#define PI 3.14159265f

void check_param(void);
void init_line(void);
void update (void);
void printfinal (void);

int nsteps,                 	/* number of time steps */
    tpoints, 	     		/* total points along string */
    rcode;                  	/* generic return code */
float  values[MAXPOINTS+2], 	/* values at time t */
       oldval[MAXPOINTS+2], 	/* values at time (t-dt) */
       newval[MAXPOINTS+2]; 	/* values at time (t+dt) */

float *dev_values, *dev_oldval, *dev_newval;

/**********************************************************************
 *	Checks input values from parameters
 *********************************************************************/
void check_param(void)
{
   char tchar[20];

   /* check number of points, number of iterations */
   while ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)) {
      printf("Enter number of points along vibrating string [%d-%d]: "
           ,MINPOINTS, MAXPOINTS);
      scanf("%s", tchar);
      tpoints = atoi(tchar);
      if ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS))
         printf("Invalid. Please enter value between %d and %d\n", 
                 MINPOINTS, MAXPOINTS);
   }
   while ((nsteps < 1) || (nsteps > MAXSTEPS)) {
      printf("Enter number of time steps [1-%d]: ", MAXSTEPS);
      scanf("%s", tchar);
      nsteps = atoi(tchar);
      if ((nsteps < 1) || (nsteps > MAXSTEPS))
         printf("Invalid. Please enter value between 1 and %d\n", MAXSTEPS);
   }

   printf("Using points = %d, steps = %d\n", tpoints, nsteps);

}

/**********************************************************************
 *     Initialize points on line
 *********************************************************************/
void init_line(void)
{
   int i, j;
   float x, fac, k, tmp;

   /* Calculate initial values based on sine curve */
   fac = 2.0f * PI;
   k = 0.0f; 
   tmp = tpoints - 1.0f;
   for (j = 1.0f; j <= tpoints; j++) {
      x = k/tmp;
      values[j] = sin (fac * x);
      k = k + 1.0f;
   } 

   /* Initialize old values array */
   for (i = 1; i <= tpoints; i++) 
      oldval[i] = values[i];
}

/**********************************************************************
 *      Calculate new values using wave equation
 *********************************************************************/

 void move_to_device()
 {
    // load data to device memory
   cudaMalloc(&dev_values, MAXPOINTS+2);
   cudaMemcpy(dev_values, values, MAXPOINTS+2, cudaMemcpyHostToDevice);
   cudaMalloc(&dev_oldval, MAXPOINTS+2);
   cudaMemcpy(dev_oldval, oldval, MAXPOINTS+2, cudaMemcpyHostToDevice);
   cudaMalloc(&dev_newval, MAXPOINTS+2);
   cudaMemcpy(dev_newval, newval, MAXPOINTS+2, cudaMemcpyHostToDevice);
 }

 void move_to_host()
 {
    // load data from GPU
   cudaMemcpy(values, dev_values, MAXPOINTS+2, cudaMemcpyDeviceToHost);
   cudaFree(dev_newval);
   cudaFree(dev_oldval);
   cudaFree(dev_values);
 }

// cuda version
__global__ void DoMath(float* dev_newval, float* dev_oldval, float* dev_values)
{
   float dtime, c, dx, tau, sqtau;
   int i = threadIdx.x;

   dtime = 0.3f;
   c = 1.0f;
   dx = 1.0f;
   tau = (c * dtime / dx);
   sqtau = tau * tau;
   dev_newval[i] = (2.0f * dev_values[i]) - dev_oldval[i] + (sqtau *  (-2.0f)*dev_values[i]);
}

__global__ void UpdateOldVal(float* dev_newval, float* dev_oldval, float* dev_values)
{
   int i = threadIdx.x;
   dev_oldval[i] = dev_values[i];
   dev_values[i] = dev_newval[i];
}


 void do_math(int i)
{
   float dtime, c, dx, tau, sqtau;

   dtime = 0.3;
   c = 1.0;
   dx = 1.0;
   tau = (c * dtime / dx);
   sqtau = tau * tau;
   newval[i] = (2.0 * values[i]) - oldval[i] + (sqtau *  (-2.0)*values[i]);
}

/**********************************************************************
 *     Update all values along line a specified number of times
 *********************************************************************/
void update()
{
   int i;

   /* Update values for each time step */
   for (i = 1; i<= nsteps; i++) {
      /* Update points along line for this time step */
      // if ((j == 1) || (j  == tpoints))
      newval[1] = 0.0f;
      newval[tpoints] = 0.0f;

      DoMath<<<2,tpoints>>>(dev_newval, dev_oldval, dev_values);

      // for (j = 1; j <= tpoints; j++) {
      //    /* global endpoints */
      //    if ((j == 1) || (j  == tpoints))
      //       newval[j] = 0.0;
      //    else
      //       do_math(j);
      // }
      

      /* Update old values with new values */
      // for (j = 1; j <= tpoints; j++) {
      //    oldval[j] = values[j];
      //    values[j] = newval[j];
      // }
      UpdateOldVal<<<1,tpoints>>>(dev_newval, dev_oldval, dev_values);
   }
}

/**********************************************************************
 *     Print final results
 *********************************************************************/
void printfinal()
{
   int i;

   for (i = 1; i <= tpoints; i++) {
      printf("%6.4f ", values[i]);
      if (i%10 == 0)
         printf("\n");
   }
}

/**********************************************************************
 *	Main program
 *********************************************************************/
int main(int argc, char *argv[])
{
	sscanf(argv[1],"%d",&tpoints);
	sscanf(argv[2],"%d",&nsteps);
   check_param();
   #ifdef CHECK_TIME
   clock_t begin = clock();
   #endif
   printf("Initializing points on the line...\n");
   init_line();

   move_to_device();

	printf("Updating all points for all time steps...\n");
	update();
   printf("Printing final results...\n");
   
   move_to_host();

	printfinal();
   printf("\nDone.\n\n");
   #ifdef CHECK_TIME
	clock_t end = clock();  
   double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
   printf("time: %4f sec\n", time_spent);
   #endif
	return 0;
}
