#include <stdio.h>
#include <stdlib.h>
#include <random>
#include <ctime>
#include "pthread.h"
#include <math.h>
#include <iostream>
#include <chrono>
#include <unistd.h>

#define XOR64MAX 18446744073709551615
#define XOR32MAX 4294967295
#define MAXDIAMETER 9223372036854775807
#define MAXRADIUS 18446744065119617025

long thread_count;
unsigned long long total_number_in_circle;
unsigned long long thread_toss_num;
pthread_mutex_t mutex;

// random function (xorshift)

uint64_t xorshift64(uint64_t xorshift64_state){
	xorshift64_state ^= xorshift64_state << 13;
	xorshift64_state ^= xorshift64_state >> 7;
	xorshift64_state ^= xorshift64_state << 17;
	return xorshift64_state;
}

uint32_t xorshift32(uint32_t state){
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return state;
}

void* Thread_toss(void* tn){
    unsigned long long number_in_circle = 0, toss;
    // random generator
    std::random_device rd;
    std::default_random_engine generator(rd());
    std::uniform_real_distribution<float> unif(-1.0, 1.0);
    
    uint32_t state_1 = generator();
    uint32_t state_2 = generator();
    
    for (toss = 0; toss < thread_toss_num; toss++) {        
        
        // x, y = random double between -1 and 1;
        double x, y, distance_squared;

        state_1 = xorshift32(state_1);
        state_2 = xorshift32(state_2);

        x = (double) state_1;
        y = (double) state_2;
        
        distance_squared = x*x + y*y;

        if (distance_squared <= (double)MAXRADIUS)
            number_in_circle++;
    }

    pthread_mutex_lock(&mutex);
    total_number_in_circle += number_in_circle;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char **argv)
{
    unsigned long long  number_of_cpu, number_of_tosses;
    if ( argc < 2) {
        exit(-1);
    }
    number_of_cpu = atoi(argv[1]);
    number_of_tosses = atoi(argv[2]);
    if (( number_of_cpu < 1) || ( number_of_tosses < 0)) {
        exit(-1);
    }

    // Record start time
    auto start = std::chrono::high_resolution_clock::now();

    
    // prepare for thread creation
    long       thread;  // Use long in case of a 64-bit system 
    pthread_t* thread_handles;
    thread_count = number_of_cpu;
    thread_handles = (pthread_t*) malloc(thread_count*sizeof(pthread_t)); 
    pthread_mutex_init(&mutex,  NULL);
    total_number_in_circle = 0;
    thread_toss_num = floor(number_of_tosses / number_of_cpu);

    for (thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], NULL, Thread_toss, (void*)thread); 
    }

    for (thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL); 
    }

    pthread_mutex_destroy(&mutex);
    free(thread_handles);

    double pi_estimate = 4*total_number_in_circle/((double) number_of_tosses);
    
    printf("%f\n",pi_estimate);
    
    // Record end time
    // auto finish = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> elapsed = finish - start;
    // std::cout << "exec time = " << elapsed.count() << " s" << std::endl;

    return 0;
}
