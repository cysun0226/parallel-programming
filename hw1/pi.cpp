#include <stdio.h>
#include <stdlib.h>
#include <random>
#include <ctime>
#include "pthread.h"
#include <math.h>
#include <iostream>

long thread_count;
volatile unsigned long long total_number_in_circle;
unsigned long long thread_toss_num;
pthread_mutex_t mutex;

void* Thread_toss(void* tn){
    unsigned long long number_in_circle = 0, toss;
    // random generator
    std::random_device rd;
    std::default_random_engine generator(rd());
    std::uniform_real_distribution<float> unif(-1.0, 1.0);
    
    for (toss = 0; toss < thread_toss_num; toss++) {        
        
        // x, y = random double between -1 and 1;
        double x, y, distance_squared;
        x = unif(generator);
        y = unif(generator);
        
        distance_squared = x*x + y*y;
        if (distance_squared <= 1)
            number_in_circle++;
    }

    pthread_mutex_lock(&mutex);
    total_number_in_circle += number_in_circle;
    pthread_mutex_unlock(&mutex);

    std::cout << "total: " << total_number_in_circle << std::endl;

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

    
    // prepare for thread creation
    long       thread;  // Use long in case of a 64-bit system 
    pthread_t* thread_handles;
    thread_count = number_of_cpu;
    thread_handles = (pthread_t*) malloc(thread_count*sizeof(pthread_t)); 
    pthread_mutex_init(&mutex,  NULL);
    unsigned long long total_number_in_circle = 0;
    thread_toss_num = floor(number_of_tosses / number_of_cpu);

    for (thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], NULL, Thread_toss, (void*)thread); 
    }

    for (thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], NULL); 
    }

    pthread_mutex_destroy(&mutex);
    free(thread_handles);

    std::cout << "total(main): " << total_number_in_circle << std::endl;

    double pi_estimate = 4*total_number_in_circle/((double) number_of_tosses);

    
    
    printf("%f\n",pi_estimate);
    return 0;
}
