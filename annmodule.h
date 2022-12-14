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

/* This file is generated by nnir2openvx.py on 2022-11-09T15:32:00.622046-08:00 */

#ifndef included_file_annmodule_h
#define included_file_annmodule_h

#include <VX/vx.h>
#include <map>

////
// initialize graph neural network for inference
//   data -- dims[] = { 224, 224, 3, 1 } (input)
//   prob -- dims[] = { 1, 1, 1000, 1, } (output)
//
extern "C" VX_API_ENTRY vx_status VX_API_CALL annAddToGraph(vx_graph graph, vx_tensor data, vx_tensor prob, const char * binaryFilename);

#endif
