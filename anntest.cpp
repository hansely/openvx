/*
MIT License

Copyright (c) 2018 - 2022 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/* This file is generated by nnir2openvx.py on 2022-09-14T15:12:30.961799-07:00 */

#include "annmodule.h"
#include <vx_ext_amd.h>
#include <vx_amd_nn.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <string>
#include <inttypes.h>
#include <chrono>
#include <unistd.h>
#include <math.h>
#include <half/half.hpp>
#include <immintrin.h>
#include <map>
#include <thread>
#include <mutex>
using half_float::half;

#if ENABLE_OPENCV
#include <opencv2/opencv.hpp>
using namespace cv;
#if USE_OPENCV_4
#define CV_LOAD_IMAGE_COLOR IMREAD_COLOR
#endif
#endif

#define ERROR_CHECK_OBJECT(obj) { vx_status status = vxGetStatus((vx_reference)(obj)); if(status != VX_SUCCESS) { vxAddLogEntry((vx_reference)context, status     , "ERROR: failed with status = (%d) at " __FILE__ "#%d\n", status, __LINE__); return status; } }
#define ERROR_CHECK_STATUS(call) { vx_status status = (call); if(status != VX_SUCCESS) { printf("ERROR: failed with status = (%d) at " __FILE__ "#%d\n", status, __LINE__); return -1; } }

static void VX_CALLBACK log_callback(vx_context context, vx_reference ref, vx_status status, const vx_char string[])
{
    size_t len = strlen(string);
    if (len > 0) {
        printf("%s", string);
        if (string[len - 1] != '\n')
            printf("\n");
        fflush(stdout);
    }
}

inline int64_t clockCounter()
{
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

inline int64_t clockFrequency()
{
    return std::chrono::high_resolution_clock::period::den / std::chrono::high_resolution_clock::period::num;
}

static vx_status copyTensor(std::string tensorName, vx_tensor tensor, std::string args, vx_enum usage = VX_WRITE_ONLY, std::string add = "0,0,0", std::string multiply = "1,1,1")
{
    std::vector<float> addVec, mulVec;
    std::stringstream sa(add), sm(multiply);
    float i, j;
    while (sa >> i && sm >> j)
    {
        addVec.push_back(i);
        mulVec.push_back(j);
        if (sa.peek() == ',')
            sa.ignore();
        if (sm.peek() == ',')
            sm.ignore();
    }
    
    // split the args into fileName and other parameters
    std::vector<std::string> argList;
    std::istringstream sf(args);
    for(std::string s; std::getline(sf, s, ','); ) {
        argList.push_back(s);
    }
    std::cout << "Preprocessing add: " << addVec[0] << " " << addVec[1] << " " << addVec[2] << " " << std::endl;
    std::cout << "Preprocessing multiply: " << mulVec[0] << " " << mulVec[1] << " " << mulVec[2] << " " << std::endl;
    std::string fileName = argList[0];
    // access the tensor object
    vx_enum data_type = VX_TYPE_FLOAT32;
    vx_size num_of_dims = 4, dims[4] = { 1, 1, 1, 1 }, stride[4];
    vxQueryTensor(tensor, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
    vxQueryTensor(tensor, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
    vxQueryTensor(tensor, VX_TENSOR_DIMS, &dims, sizeof(dims[0])*num_of_dims);
    if((data_type != VX_TYPE_FLOAT32) && (data_type != VX_TYPE_FLOAT16) && (data_type != VX_TYPE_INT64) && (data_type != VX_TYPE_INT32)) {
        std::cerr << "ERROR: copyTensor() supports only VX_TYPE_FLOAT32 or VX_TYPE_FLOAT16 or VX_TYPE_INT64 or VX_TYPE_INT32: invalid for " << fileName << std::endl;
        return -1;
    }
    vx_size count = dims[0] * dims[1] * dims[2] * dims[3];
    vx_map_id map_id;
    void * ptr;
    vx_status status = vxMapTensorPatch(tensor, num_of_dims, nullptr, nullptr, &map_id, stride, (void **)&ptr, usage, VX_MEMORY_TYPE_HOST);
    if(status) {
        std::cerr << "ERROR: vxMapTensorPatch() failed for " << fileName << std::endl;
        return -1;
    }
    if(usage == VX_WRITE_ONLY) {
#if ENABLE_OPENCV
        if(dims[2] == 3 && fileName.size() > 4 && (fileName.substr(fileName.size()-4, 4) == ".png" || fileName.substr(fileName.size()-4, 4) == ".jpg"))
        {
            for(size_t n = 0; n < dims[3]; n++) {
                char imgFileName[1024];
                sprintf(imgFileName, fileName.c_str(), (int)n);
                Mat img;
                img = imread(imgFileName, CV_LOAD_IMAGE_COLOR);
                if(!img.data || img.rows != dims[1] || img.cols != dims[0]) {
                    printf("ERROR: invalid image or dimensions: %s\n", imgFileName);
                    return -1;
                }
                for(vx_size y = 0; y < dims[1]; y++) {
                    unsigned char * src = img.data + y*dims[0]*dims[2];
                    if(data_type == VX_TYPE_FLOAT32) {
                        float * dstR = (float *)ptr + ((n * stride[3] + y * stride[1]) >> 2);
                        float * dstG = dstR + (stride[2] >> 2);
                        float * dstB = dstG + (stride[2] >> 2);
                        for(vx_size x = 0; x < dims[0]; x++, src += 3) {
                            *dstR++ = (src[2] * mulVec[0]) + addVec[0];
                            *dstG++ = (src[1] * mulVec[1]) + addVec[1];
                            *dstB++ = (src[0] * mulVec[2]) + addVec[2];
                        }
                    } else if(data_type == VX_TYPE_FLOAT16) {
                        short * dstR = (short *)ptr + ((n * stride[3] + y * stride[1]) >> 1);
                        short * dstG = dstR + (stride[2] >> 2);
                        short * dstB = dstG + (stride[2] >> 2);                    
                        for(vx_size x = 0; x < dims[0]; x++, src += 3) {
                            *dstR++ = (src[2] * mulVec[0]) + addVec[0];
                            *dstG++ = (src[1] * mulVec[1]) + addVec[1];
                            *dstB++ = (src[0] * mulVec[2]) + addVec[2];
                        }
                    } else if(data_type == VX_TYPE_INT64) {
                        long int* dstR = (long int*)ptr + ((n * stride[3] + y * stride[1]) >> 3);
                        long int* dstG = dstR + (stride[2] >> 2);
                        long int* dstB = dstG + (stride[2] >> 2);                    
                        for(vx_size x = 0; x < dims[0]; x++, src += 3) {
                            *dstR++ = (src[2] * mulVec[0]) + addVec[0];
                            *dstG++ = (src[1] * mulVec[1]) + addVec[1];
                            *dstB++ = (src[0] * mulVec[2]) + addVec[2];
                        }
                    } else if(data_type == VX_TYPE_INT32) {
                        int* dstR = (int*)ptr + ((n * stride[3] + y * stride[1]) >> 2);
                        int* dstG = dstR + (stride[2] >> 2);
                        int* dstB = dstG + (stride[2] >> 2);                    
                        for(vx_size x = 0; x < dims[0]; x++, src += 3) {
                            *dstR++ = (src[2] * mulVec[0]) + addVec[0];
                            *dstG++ = (src[1] * mulVec[1]) + addVec[1];
                            *dstB++ = (src[0] * mulVec[2]) + addVec[2];
                        }
                    }
                }
            }
        }
        else
#endif
        {
            FILE * fp = fopen(fileName.c_str(), "rb");
            if(!fp) {
                std::cerr << "ERROR: unable to open: " << fileName << std::endl;
                return -1;
            }
            for(size_t n = 0; n < dims[3]; n++) {
                for(size_t c = 0; c < dims[2]; c++) {
                    for(size_t y = 0; y < dims[1]; y++) {
                        if(data_type == VX_TYPE_FLOAT32) {
                            float * ptrY = (float *)ptr + ((n * stride[3] + c * stride[2] + y * stride[1]) >> 2);
                            vx_size n = fread(ptrY, sizeof(float), dims[0], fp);
                            if(n != dims[0]) {
                                std::cerr << "ERROR: expected char[" << count*sizeof(float) << "], but got less in " << fileName << std::endl;
                                return -1;
                            }
                            for(size_t x = 0; x < dims[0]; x++) {
                                *(ptrY+x) = *(ptrY+x) * mulVec[c] + addVec[c];
                            }
                        } else if(data_type == VX_TYPE_FLOAT16){
                            short * ptrY = (short *)ptr + ((n * stride[3] + c * stride[2] + y * stride[1]) >> 1);
                            vx_size n = fread(ptrY, sizeof(short), dims[0], fp);
                            if(n != dims[0]) {
                                std::cerr << "ERROR: expected char[" << count*sizeof(short) << "], but got less in " << fileName << std::endl;
                                return -1;
                            }
                            for(size_t x = 0; x < dims[0]; x++) {
                                *(ptrY+x) = *(ptrY+x) * mulVec[c] + addVec[c];
                            }
                        } else if(data_type == VX_TYPE_INT64){
                            long int * ptrY = (long int *)ptr + ((n * stride[3] + c * stride[2] + y * stride[1]) >> 3);
                            vx_size n = fread(ptrY, sizeof(long int), dims[0], fp);
                            if(n != dims[0]) {
                                std::cerr << "ERROR: expected char[" << count*sizeof(long int) << "], but got less in " << fileName << std::endl;
                                return -1;
                            }
                            for(size_t x = 0; x < dims[0]; x++) {
                                *(ptrY+x) = *(ptrY+x) * mulVec[c] + addVec[c];
                            }
                        } else if(data_type == VX_TYPE_INT32){
                            int * ptrY = (int *)ptr + ((n * stride[3] + c * stride[2] + y * stride[1]) >> 3);
                            vx_size n = fread(ptrY, sizeof(int), dims[0], fp);
                            if(n != dims[0]) {
                                std::cerr << "ERROR: expected char[" << count*sizeof(int) << "], but got less in " << fileName << std::endl;
                                return -1;
                            }
                            for(size_t x = 0; x < dims[0]; x++) {
                                *(ptrY+x) = *(ptrY+x) * mulVec[c] + addVec[c];
                            }
                        }
                    }
                }
            }
            fclose(fp);
        }
    }
    else {
        if(fileName != "-") {
            FILE * fp = fopen(fileName.c_str(), "wb");
            if(!fp) {
                std::cerr << "ERROR: unable to open: " << fileName << std::endl;
                return -1;
            }
            if (data_type == VX_TYPE_FLOAT32)
                fwrite(ptr, sizeof(float), count, fp);
            else if (data_type == VX_TYPE_FLOAT16)
                fwrite(ptr, sizeof(short), count, fp);
            else if (data_type == VX_TYPE_INT64)
                fwrite(ptr, sizeof(long int), count, fp); 
            else if (data_type == VX_TYPE_INT32)
                fwrite(ptr, sizeof(int), count, fp); 
            fclose(fp);
        }
        if(argList.size() >= 2) {
            float rmsErrorLimit = 0, maxErrorLimit = 0;
            if(argList.size() >= 3) {
                rmsErrorLimit = maxErrorLimit = (float)atof(argList[2].c_str());
                if(argList.size() >= 4) {
                    maxErrorLimit = (float)atof(argList[3].c_str());
                }
            }
            float * gold = new float [count];
            FILE * fp = fopen(argList[1].c_str(), "rb");
            if(!fp) {
                std::cerr << "ERROR: unable to open: " << argList[1] << std::endl;
                return -1;
            }
            if(fread(gold, sizeof(float), count, fp) != count) {
                std::cerr << "ERROR: not enought data (" << count << " floats needed) in " << argList[1] << std::endl;
                return -1;
            }
            fclose(fp);
            double sqrError = 0;
            float maxError = 0;
            if(data_type == VX_TYPE_FLOAT32) {
                for(size_t i = 0; i < count; i++) {
                    float err = ((float *)ptr)[i] - gold[i];
                    if(err < 0) err = -err;
                    sqrError += err * err;
                    if(!(err < maxError)) maxError = err;
                }
            }
            else if(data_type == VX_TYPE_FLOAT16)
            {
                for(size_t i = 0; i < count; i++) {
                    float src = _cvtsh_ss(((unsigned short*)ptr)[i]);
                    float err = src - gold[i];
                    if(err < 0) err = -err;
                    sqrError += err * err;
                    if(!(err < maxError)) maxError = err;
                }
            }
            delete[] gold;
            float rmsError = (float)sqrt(sqrError/count);
            bool isError = !(rmsError <= rmsErrorLimit) || !(maxError <= maxErrorLimit);
            printf("%s: [RMS-Error %e] [MAX-Error %e] for %s with %s\n",
                isError ? "ERROR" : "OK", rmsError, maxError, tensorName.c_str(), argList[1].c_str());
            if(isError) {
                return -1;
            }
        }

    }
    status = vxUnmapTensorPatch(tensor, map_id);
    if(status) {
        std::cerr << "ERROR: vxUnmapTensorPatch() failed for " << fileName << std::endl;
        return -1;
    }
    return 0;
}

const int num_gpu = 2;
vx_graph openvx_graph[num_gpu];
vx_context openvx_context[num_gpu];
vx_tensor openvx_input[num_gpu];
vx_tensor openvx_output[num_gpu];
std::thread * threadDeviceProcess[num_gpu];

void processDevice(int gpu) {
    vx_status status = vxProcessGraph(openvx_graph[gpu]);
    if(status != VX_SUCCESS) {
        printf("ERROR: vxProcessGraph() failed (%d)\n", status);
    }
}

int main(int argc, const char ** argv)
{
    // check command-line usage
    if(argc < 2) {
        printf(
            "\n"
            "Usage: anntest <weights.bin> [<input-data-file(s)> [<output-data-file(s)>]] <--add ADD> <--multiply MULTIPLY>]\n"
            "\n"
            "   <weights.bin>: is a filename of the weights file to be used for the inference\n"
            "   <input-data-file>: is a filename to initialize input tensor\n"
            "   <output-data-file>: is a filename to initialize output tensor\n"
#if ENABLE_OPENCV
            "     .jpg or .png: decode and initialize for 3 channel tensors\n"
            "         (use %%04d in fileName when batch-size > 1: batch index starts from 0)\n"
#endif
            "     other: initialize tensor with raw data from the file\n"
            "\n"
            "   <output-data-file>[,<reference-for-compare>,<maxErrorLimit>,<rmsErrorLimit>]:\n"
            "     <referece-to-compare> is raw tensor data for comparision\n"
            "     <maxErrorLimit> is max absolute error allowed\n"
            "     <rmsErrorLimit> is max RMS error allowed\n"
            "     <output-data-file> is filename for saving output tensor data\n"
            "       '-' to ignore\n"
            "       other: save raw tensor into the file\n"
            "   <add>: input preprocessing factor [optional - default:[0,0,0]]\n"
            "   <multiply>: input preprocessing factor [optional - default:[1,1,1]]\n"
            "\n"
        );
        return -1;
    }
    const char * binaryFilename = argv[1];
    argc -= 2;
    argv += 2;

    std::string add = "0,0,0", multiply = "1,1,1";
    for (int i = 0; i < argc; i++) {
        if (strcasecmp(argv[i], "--add") == 0) {
            add = argv[i+1];
        }
        if (strcasecmp(argv[i], "--multiply") == 0) {
            multiply = argv[i+1];
        }
    }
    
    for(int gpu=0; gpu < num_gpu; gpu++) {
        // create context, input, output, and graph
        vxRegisterLogCallback(NULL, log_callback, vx_false_e);
        openvx_context[gpu] = vxCreateContext();
        vx_status status = vxGetStatus((vx_reference)openvx_context[gpu]);
        if(status) {
            printf("ERROR: vxCreateContext() failed\n");
            return -1;
        }
        vxRegisterLogCallback(openvx_context[gpu], log_callback, vx_false_e);
        openvx_graph[gpu] = vxCreateGraph(openvx_context[gpu]);
        status = vxGetStatus((vx_reference)openvx_graph[gpu]);
        if(status) {
            printf("ERROR: vxCreateGraph(...) failed (%d)\n", status);
            return -1;
        }

        // create and initialize input tensor data
        vx_size dims_data[4] = { 224, 224, 3, 1 };
        openvx_input[gpu] = vxCreateTensor(openvx_context[gpu], 4, dims_data, VX_TYPE_FLOAT32, 0);
        if(vxGetStatus((vx_reference)openvx_input[gpu])) {
            printf("ERROR: vxCreateTensor() failed for data\n");
            return -1;
        }
        if(*argv) {
            if(strcmp(*argv, "-") != 0) {
                if(copyTensor("data", openvx_input[gpu], *argv, VX_WRITE_ONLY, add, multiply) < 0) {
                    return -1;
                }
                printf("OK: initialized tensor 'data' from %s\n", *argv);
            }
            argv++;
        }

        // create output tensor prob
        vx_size dims_prob[4] = { 1, 1, 1000, 1 };
        openvx_output[gpu] = vxCreateTensor(openvx_context[gpu], 4, dims_prob, VX_TYPE_FLOAT32, 0);
        if(vxGetStatus((vx_reference)openvx_output[gpu])) {
            printf("ERROR: vxCreateTensor() failed for prob\n");
            return -1;
        }

        // build graph using annmodule
        int64_t freq = clockFrequency(), t0, t1;
        t0 = clockCounter();
        status = annAddToGraph(openvx_graph[gpu], openvx_input[gpu], openvx_output[gpu], binaryFilename);
        if(status) {
            printf("ERROR: annAddToGraph() failed (%d)\n", status);
            return -1;
        }

        int hipDevice = gpu;
        status = vxSetContextAttribute(openvx_context[gpu], VX_CONTEXT_ATTRIBUTE_AMD_HIP_DEVICE, &hipDevice, sizeof(hipDevice));

        status = vxVerifyGraph(openvx_graph[gpu]);
        if(status) {
            printf("ERROR: vxVerifyGraph(...) failed (%d)\n", status);
            return -1;
        }
        t1 = clockCounter();
        printf("OK: graph initialization with annAddToGraph() took %.3f msec\n", (float)(t1-t0)*1000.0f/(float)freq);

        // save tensor prob
        if(*argv) {
            if(strcmp(*argv, "-") != 0) {
                if(copyTensor("prob", openvx_output[gpu], *argv, VX_READ_ONLY, add, multiply) < 0) {
                    return -1;
                }
                printf("OK: wrote tensor 'prob' into %s\n", *argv);
            }
            argv++;
        }
    }
    
    // multithreading
    for(int gpu = 0; gpu < num_gpu; gpu++) {
        threadDeviceProcess[gpu] = new std::thread(&processDevice, gpu);
    }

    // release resources
    for(int gpu=0; gpu<num_gpu; gpu++) {
        if(threadDeviceProcess[gpu] && threadDeviceProcess[gpu]->joinable()) {
            threadDeviceProcess[gpu]->join();
        }
        ERROR_CHECK_STATUS(vxReleaseGraph(&openvx_graph[gpu]));
        ERROR_CHECK_STATUS(vxReleaseTensor(&openvx_input[gpu]));
        ERROR_CHECK_STATUS(vxReleaseTensor(&openvx_output[gpu]));
        ERROR_CHECK_STATUS(vxReleaseContext(&openvx_context[gpu]));
    }
    printf("OK: successful\n");

    return 0;
}
