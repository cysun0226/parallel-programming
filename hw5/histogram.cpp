#include <fstream>
#include <iostream>
#include <string>
#include <ios>
#include <time.h>
#include <math.h>

//#include <CL/cl.hpp>
#include <CL/cl.h>
//#define DUMP_RGB true

const char *kernelSource =                                       "\n" \
"typedef struct \n" \
"{ \n" \
"    uchar R; \n" \
"    uchar G; \n" \
"    uchar B; \n" \
"    uchar align; \n" \
"} RGB; \n" \
" \n" \
"typedef struct \n" \
"{ \n" \
"    bool type; \n" \
"    uint size; \n" \
"    uint height; \n" \
"    uint weight; \n" \
"    RGB *data; \n" \
"} Image; \n" \
" \n" \
"kernel void hist_count( \n" \
"    global RGB *data, \n" \
"    global uint *R, \n" \
"    global uint *G, \n" \
"    global uint *B \n" \
") \n" \
"{ \n" \
"    int i = get_global_id(0); \n" \
"    atomic_add(&R[data[i].R], 1); \n" \
"    atomic_add(&G[data[i].G], 1); \n" \
"    atomic_add(&B[data[i].B], 1); \n" \
"} \n";

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

Image *readbmp(const char *filename)
{
    std::ifstream bmp(filename, std::ios::binary);
    char header[54];
    bmp.read(header, 54);
    uint32_t size = *(int *)&header[2];
    uint32_t offset = *(int *)&header[10];
    uint32_t w = *(int *)&header[18];
    uint32_t h = *(int *)&header[22];
    uint16_t depth = *(uint16_t *)&header[28];
    if (depth != 24 && depth != 32)
    {
        printf("we don't suppot depth with %d\n", depth);
        exit(0);
    }
    bmp.seekg(offset, bmp.beg);

    Image *ret = new Image();
    ret->type = 1;
    ret->height = h;
    ret->weight = w;
    ret->size = w * h;
    ret->data = new RGB[w * h]{};
    for (int i = 0; i < ret->size; i++)
    {
        bmp.read((char *)&ret->data[i], depth / 8);
    }
    return ret;
}

//int getImageSize(const char *filename)
//{
//    std::ifstream bmp(filename, std::ios::binary);
//    char header[54];
//    bmp.read(header, 54);
//    uint32_t size = *(int *)&header[2];
//    uint32_t offset = *(int *)&header[10];
//    uint32_t w = *(int *)&header[18];
//    uint32_t h = *(int *)&header[22];
//    return w*h;
//}

int writebmp(const char *filename, Image *img)
{

    uint8_t header[54] = {
        0x42,        // identity : B
        0x4d,        // identity : M
        0, 0, 0, 0,  // file size
        0, 0,        // reserved1
        0, 0,        // reserved2
        54, 0, 0, 0, // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0,  // bmp width
        0, 0, 0, 0,  // bmp height
        1, 0,        // planes
        32, 0,       // bit per pixel
        0, 0, 0, 0,  // compression
        0, 0, 0, 0,  // data size
        0, 0, 0, 0,  // h resolution
        0, 0, 0, 0,  // v resolution
        0, 0, 0, 0,  // used colors
        0, 0, 0, 0   // important colors
    };

    // file size
    uint32_t file_size = img->size * 4 + 54;
    header[2] = (unsigned char)(file_size & 0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    // width
    uint32_t width = img->weight;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) & 0x000000ff;
    header[20] = (width >> 16) & 0x000000ff;
    header[21] = (width >> 24) & 0x000000ff;

    // height
    uint32_t height = img->height;
    header[22] = height & 0x000000ff;
    header[23] = (height >> 8) & 0x000000ff;
    header[24] = (height >> 16) & 0x000000ff;
    header[25] = (height >> 24) & 0x000000ff;

    std::ofstream fout;
    fout.open(filename, std::ios::binary);
    fout.write((char *)header, 54);
    fout.write((char *)img->data, img->size * 4);
    fout.close();
}

void histogram(Image *img,uint32_t R[256],uint32_t G[256],uint32_t B[256]){
    std::fill(R, R+256, 0);
    std::fill(G, G+256, 0);
    std::fill(B, B+256, 0);

    for (int i = 0; i < img->size; i++){
        RGB &pixel = img->data[i];
        R[pixel.R]++;
        G[pixel.G]++;
        B[pixel.B]++;
    }
}

void dump_RGB(uint32_t R[256],uint32_t G[256],uint32_t B[256]){
    std::cout << "[R]" << std::endl;
    for (int i = 0; i < 256; i++) {
        std::cout << R[i] << " ";
        if(i%16==15){
            std::cout << std::endl;
        }
    }
    std::cout << std::endl << std::endl;

    std::cout << "[G]" << std::endl;
    for (int i = 0; i < 256; i++) {
        std::cout << G[i] << " ";
        if(i%16==15){
            std::cout << std::endl;
        }
    }
    std::cout << std::endl << std::endl;

    std::cout << "[B]" << std::endl;
    for (int i = 0; i < 256; i++) {
        std::cout << B[i] << " ";
        if(i%16==15){
            std::cout << std::endl;
        }
    }
    std::cout << std::endl << std::endl;
}

int main(int argc, char *argv[])
{
    char *filename;
    
    if (argc >= 2)
    {
        // initialize opencl
        cl_int err;
        cl_platform_id cpPlatform;        // OpenCL platform
        cl_device_id device_id;           // device ID
        cl_context context;               // context
        cl_command_queue queue;           // command queue
        cl_program program;               // program
        cl_kernel kernel;                 // kernel

        // query platform
        clGetPlatformIDs(1, &cpPlatform, NULL);
        
        // get device list
        clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);

        // create context
        context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);

        // command queue
        queue = clCreateCommandQueue(context, device_id, 0, &err);

        // load kernel code
        std::ifstream kernelFile("histogram.cl");
        std::string kernelCode(std::istreambuf_iterator<char>(kernelFile), (std::istreambuf_iterator<char>()));
//        program = clCreateProgramWithSource(context, 1, (const char **)kernelCode.c_str(), NULL, &err);
        program = clCreateProgramWithSource(context, 1,
                                            (const char **) & kernelSource, NULL, &err);

        // make & build
        clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
        kernel = clCreateKernel(program, "hist_count", &err);

        // get build failed log
//        size_t len = 0;
//        cl_int ret = CL_SUCCESS;
//        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
//        char *buffer = static_cast<char *>(calloc(len, sizeof(char)));
//        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);

        int many_img = argc - 1;
        clock_t begin = clock();
        for (int i = 0; i < many_img; i++)
        {
            filename = argv[i + 1];
            uint32_t R[256];
            uint32_t G[256];
            uint32_t B[256];
            Image *img = readbmp(filename);
            std::cout << img->weight << ":" << img->height << "\n";

            // memory buffers
            cl_mem buf_data = clCreateBuffer(context, CL_MEM_READ_ONLY, img->size*sizeof(RGB), NULL, NULL);
            cl_mem buf_R = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256*sizeof(uint32_t), NULL, NULL);
            cl_mem buf_G = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256*sizeof(uint32_t), NULL, NULL);
            cl_mem buf_B = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256*sizeof(uint32_t), NULL, NULL);
            
            cl_uint z = 0;
            clEnqueueFillBuffer(queue, buf_R, &z, sizeof(z), 0, 256*sizeof(uint32_t), 0, NULL, NULL);
            clEnqueueFillBuffer(queue, buf_G, &z, sizeof(z), 0, 256*sizeof(uint32_t), 0, NULL, NULL);
            clEnqueueFillBuffer(queue, buf_B, &z, sizeof(z), 0, 256*sizeof(uint32_t), 0, NULL, NULL);
            

            // move data to the device
            clEnqueueWriteBuffer(queue, buf_data, CL_TRUE, 0, img->size*sizeof(RGB), img->data, 0, NULL, NULL);
            // queue.enqueueWriteBuffer(buf_data, CL_TRUE, 0, img->size*sizeof(RGB), img->data);


            
            // cl::Program program = cl::Program(context, source);
            // program.build(devices);
            // if (program.build({default_device}) != CL_SUCCESS) {
            //     std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
            //     exit(1);
            // }
            // cl::Kernel histogram_kernel(program, "hist_count");

            // arguments
            clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_data);
            clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_R);
            clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_G);
            clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_B);
            clFinish(queue);
            
            // execute
            size_t localSize = 64;
            size_t globalSize = ceil(img->size/(float)localSize)*localSize;
            clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);
            clFinish(queue);

            // move data from device to host
            clEnqueueReadBuffer(queue, buf_R, CL_TRUE, 0, 256*sizeof(uint32_t), R, 0, NULL, NULL);
            clEnqueueReadBuffer(queue, buf_G, CL_TRUE, 0, 256*sizeof(uint32_t), G, 0, NULL, NULL);
            clEnqueueReadBuffer(queue, buf_B, CL_TRUE, 0, 256*sizeof(uint32_t), B, 0, NULL, NULL);
            clFinish(queue);

            #ifdef DUMP_RGB
            dump_RGB(R, G, B);
            #endif

            int max = 0;
            for(int i=0;i<256;i++){
                max = R[i] > max ? R[i] : max;
                max = G[i] > max ? G[i] : max;
                max = B[i] > max ? B[i] : max;
            }

            Image *ret = new Image();
            ret->type = 1;
            ret->height = 256;
            ret->weight = 256;
            ret->size = 256 * 256;
            ret->data = new RGB[256 * 256];

            for(int i=0;i<ret->height;i++){
                for(int j=0;j<256;j++){
                    if(R[j]*256/max > i)
                        ret->data[256*i+j].R = 255;
                    if(G[j]*256/max > i)
                        ret->data[256*i+j].G = 255;
                    if(B[j]*256/max > i)
                        ret->data[256*i+j].B = 255;
                }
            }

            std::string newfile = "hist_" + std::string(filename); 
            writebmp(newfile.c_str(), ret);

            // release the resource
            delete[] img;
            clReleaseMemObject(buf_data);
            clReleaseMemObject(buf_R);
            clReleaseMemObject(buf_G);
            clReleaseMemObject(buf_B);
        }
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        clock_t end = clock();  
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("time: %4f sec\n", time_spent);
    }else{
        printf("Usage: ./hist <img.bmp> [img2.bmp ...]\n");
    }
    return 0;
}
