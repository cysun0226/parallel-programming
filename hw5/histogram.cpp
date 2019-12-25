#include <fstream>
#include <iostream>
#include <string>
#include <ios>
#include <time.h>
#include <math.h>

#include <CL/cl.hpp>


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

cl::Platform getPlatform() {
    /* Returns the first platform found. */
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);

    if (all_platforms.size()==0) {
        std::cout << "No platforms found. Check OpenCL installation!\n";
        exit(1);
    }
    return all_platforms[0];
}

cl::Device getDevice(cl::Platform platform, int i, bool display=false) {
    /* Returns the deviced specified by the index i on platform.
     * If display is true, then all of the platforms are listed.
     */
    std::vector<cl::Device> all_devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if(all_devices.size()==0){
        std::cout << "No devices found. Check OpenCL installation!\n";
        exit(1);
    }

    if (display) {
        for (int j=0; j<all_devices.size(); j++)
            printf("Device %d: %s\n", j, all_devices[j].getInfo<CL_DEVICE_NAME>().c_str());
    }
    return all_devices[i];
}

int main(int argc, char *argv[])
{
    char *filename;
    
    if (argc >= 2)
    {
        // initialize opencl
        unsigned int platform_id = 0, device_id = 0;

        // // query platform
        // std::vector<cl::Platform> platforms;
        // cl::Platform::get(&platforms);

        // // get device list
        // std::vector<cl::Device> devices;
        // platforms[platform_id].getDevices(CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CPU, &devices);

        cl::Platform default_platform = getPlatform();
        cl::Device default_device     = getDevice(default_platform, 0);

        // create context
        cl::Context context(default_device);

        // command queue
        cl::CommandQueue queue = cl::CommandQueue(context, default_device);

        int many_img = argc - 1;
        clock_t begin = clock();
        for (int i = 0; i < many_img; i++)
        {
            filename = argv[i + 1];
            uint32_t z = 0;
            uint32_t R[256];
            uint32_t G[256];
            uint32_t B[256];
            Image *img = readbmp(filename);
            std::cout << img->weight << ":" << img->height << "\n";

            // memory buffers
            cl::Buffer buf_data = cl::Buffer(context, CL_MEM_READ_ONLY, img->size*sizeof(RGB));
            cl::Buffer buf_R = cl::Buffer(context, CL_MEM_WRITE_ONLY, 256*sizeof(uint32_t));
            cl::Buffer buf_G = cl::Buffer(context, CL_MEM_WRITE_ONLY, 256*sizeof(uint32_t));
            cl::Buffer buf_B = cl::Buffer(context, CL_MEM_WRITE_ONLY, 256*sizeof(uint32_t));
            
            queue.enqueueFillBuffer(buf_R, 0, 0, 256*sizeof(uint32_t));
            queue.enqueueFillBuffer(buf_G, 0, 0, 256*sizeof(uint32_t));
            queue.enqueueFillBuffer(buf_B, 0, 0, 256*sizeof(uint32_t));
            queue.finish();

            

            // move data to the device
            queue.enqueueWriteBuffer(buf_data, CL_TRUE, 0, img->size*sizeof(RGB), img->data);

            // load kernel code
            std::ifstream kernelFile("histogram.cl");
            std::string kernelCode(std::istreambuf_iterator<char>(kernelFile), (std::istreambuf_iterator<char>()));
            cl::Program::Sources source;
            source.push_back({kernelCode.c_str(), kernelCode.length()});
            
            // make & build
            cl::Program program = cl::Program(context, source);
            // program.build(devices);
            if (program.build({default_device}) != CL_SUCCESS) {
                std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
                exit(1);
            }
            cl::Kernel histogram_kernel(program, "hist_count");

            // arguments
            histogram_kernel.setArg(0, buf_data);
            histogram_kernel.setArg(1, buf_R);
            histogram_kernel.setArg(2, buf_G);
            histogram_kernel.setArg(3, buf_B);
            
            // execute
            cl::NDRange global(img->size);
            cl::NDRange local(256);
            queue.enqueueNDRangeKernel(histogram_kernel, cl::NullRange, global, local);
            queue.finish();

            // move data from device to host
            queue.enqueueReadBuffer(buf_R, CL_TRUE, 0, 256*sizeof(uint32_t), R);
            queue.enqueueReadBuffer(buf_G, CL_TRUE, 0, 256*sizeof(uint32_t), G);
            queue.enqueueReadBuffer(buf_B, CL_TRUE, 0, 256*sizeof(uint32_t), B);
            queue.finish();

            // histogram(img,R,G,B);

            dump_RGB(R, G, B);

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
            
        }
        clock_t end = clock();  
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("time: %4f sec\n", time_spent);
    }else{
        printf("Usage: ./hist <img.bmp> [img2.bmp ...]\n");
    }
    return 0;
}
