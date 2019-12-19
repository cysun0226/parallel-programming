
    // System includes  
    #include <stdio.h>  
    #include <stdlib.h>  
      
    // OpenCL includes  
    #include <CL/cl.h>  
      
    // OpenCL kernel to perform an element-wise addition  
    const char* programSource =   
        "__kernel                                                         \n"  
        "void vecadd(__global int *A,                                     \n"  
        "            __global int *B,                                     \n"  
        "            __global int *C)                                     \n"  
        "{                                                                \n"  
        " // Get the work-item's unique ID                                \n"  
        " int idx=get_global_id(0);                                       \n"  
        "                                                                 \n"  
        " // Add the corresponding locations of                           \n"  
        " // 'A' and 'B', and store the result in 'C'.                    \n"  
        " C[idx]=A[idx]+B[idx];                                           \n"  
        "}                                                                \n"  
        ;  
    int main(){  
        // This code executes on the OpenCL host  
        // Host data  
        int *A = NULL; // Input array  
        int *B = NULL; // Input array  
        int *C = NULL; // Output array  
        // Elements in each array  
        const int elements = 2048;  
        // Compute the size of the data  
        size_t datasize = sizeof(int)*elements;  
        // Allocate space for input/output data  
        A = (int*)malloc(datasize);  
        B = (int*)malloc(datasize);  
        C = (int*)malloc(datasize);  
        // Initialize the input data  
        int i;  
        for(i=0; i<elements; i++)  
        {  
            A[i]=i;  
            B[i]=i;  
        }  
      
        // Use this to check the output of each API call  
        cl_int status;  
        // Retrieve the number of platforms  
        cl_uint numPlatforms=0;  
        status = clGetPlatformIDs(0, NULL, &numPlatforms);  
        printf("\t[Info] Total %d platforms...\n", numPlatforms);  
      
        // Allocate enough space for each platform  
        cl_platform_id *platforms=NULL;  
        platforms=(cl_platform_id*)malloc(numPlatforms*sizeof(cl_platform_id));  
        // Fill in the platforms  
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);  
        // Retrieve the number of devices  
        cl_uint numDevices=0;  
        status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);  
        printf("\t[Info] Total %d devices...\n", numDevices);  
      
        // Allocate enough space for each device  
        cl_device_id *devices;  
        devices=(cl_device_id*)malloc(numDevices*sizeof(cl_device_id));  
        // Fill in the devices  
        status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);  
        if(status == CL_SUCCESS) printf("\t[Info] Get DeviceIDs Success!\n");  
        else fprintf(stderr, "\t[Error] Get DeviceIDs Fail!\n");  
        // Create a context and associate it with the devices  
        cl_context context;  
        context = clCreateContext(NULL, numDevices, devices, NULL, NULL, &status);  
        if(status == CL_SUCCESS) printf("\t[Info] Create Context Success!\n");  
        else fprintf(stderr, "\t[Error] Create Context Fail!\n");  
        // Create a command queue and associate it with the device  
        cl_command_queue cmdQueue;  
        cmdQueue = clCreateCommandQueue(context, devices[0], 0, &status);  
        if(status == CL_SUCCESS) printf("\t[Info] Create CommandQueue Success!\n");  
        else fprintf(stderr, "\t[Error] Create CommandQueue Fail!\n");  
      
        // Create a buffer object that will contain the data from the host array A  
        cl_mem bufA;  
        bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &status);  
        // Create a buffer object that will contain the data from the host array B  
        cl_mem bufB;  
        bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &status);  
        // Create a buffer object that will hold the output data  
        cl_mem bufC;  
        bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, &status);  
        // Write input array A to the device buffer bufferA  
        status = clEnqueueWriteBuffer(cmdQueue, bufA, CL_FALSE, 0, datasize, A, 0, NULL, NULL);  
        // Write input array B to the device buffer bufferB  
        status = clEnqueueWriteBuffer(cmdQueue, bufB, CL_FALSE, 0, datasize, B, 0, NULL, NULL);  
      
        // Create a program with source code  
        cl_program program = clCreateProgramWithSource(context,   
            1,   
            (const char**)&programSource,   
            NULL,   
            &status);  
        if(status == CL_SUCCESS) printf("\t[Info] Create Program Success!\n");  
        else fprintf(stderr, "\t[Error] Create Program Fail!\n");  
        // Build (compile) the program for the device  
        status = clBuildProgram(program,        // The program object.  
            numDevices,                         // The number of devices listed in device_list.  
            devices,                            // A pointer to a list of devices associated with program.  
            NULL,  
            NULL,  
            NULL  
            );  
        if(status == CL_SUCCESS) printf("\t[Info] Build Program Success!\n");  
        else fprintf(stderr, "\t[Error] Build Program Fail!\n");  
        cl_kernel kernel;  
        kernel = clCreateKernel(program,  
            "vecadd",  
            &status);  
      
        // Associate the input/output buffers with the kernel  
        status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);  
        status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);  
        status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);  
        // Define an index space (global work size) of work items for execution.  
        // A workgroup size (local work size) is not required, but can be used.  
        size_t globalWorkSize[1];  
        // There are elements' work-items  
        globalWorkSize[0]=elements;  
        // Execute the kernel for execution  
        status = clEnqueueNDRangeKernel(cmdQueue,       // A valid command-queue  
            kernel,         // A valid kernel object.  
            1,              // work_dim  
            NULL,           // *global_work_offset  
            globalWorkSize, // *global_work_size  
            NULL,           // local_work_size  
            0,              // num_events_in_wait_list  
            NULL,           // *event_wait_list  
            NULL            // *event  
            );  
      
        // Read the device output buffer to the host output array  
        clEnqueueReadBuffer(cmdQueue,     // Refers to the command-queue in which the read command will be queued  
            bufC,         // Refers to a valid buffer object.  
            CL_TRUE,      // Indicates if the read operations are blocking or non-blocking.  
            0,            // The offset in bytes in the buffer object to read from.  
            datasize,     // The size in bytes of data being read.  
            C,            // The pointer to buffer in host memory where data is to be read into.  
            0,            // num_events_in_wait_list  
            NULL,         // *event_wait_list  
            NULL          // *event  
            );  
        // Verify the output  
        int result=1;  
        for(i=0; i<elements; i++)  
        {  
            if(C[i]!=i+i)  
            {  
                printf("C[%d]=%d (Should be %d)!\n", i, C[i], i+i);  
                result=0;  
                break;  
            }  
        }               
        if(result)  
        {  
            printf("Output is correct!\n");  
        }       
        else  
        {  
            printf("Output is incorrect!\n");  
        }  
      
        printf("\t[Info] Free resources...\n");  
        // Free OpenCL resources  
        clReleaseKernel(kernel);  
        clReleaseProgram(program);  
        clReleaseCommandQueue(cmdQueue);  
        clReleaseMemObject(bufA);  
        clReleaseMemObject(bufB);  
        clReleaseMemObject(bufC);  
        clReleaseContext(context);  
      
        // Free host resources  
        free(A);  
        free(B);  
        free(C);  
        free(platforms);  
        free(devices);  
        return 0;  
    }  
