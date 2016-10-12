//
//  api.hpp
//  App
//
//  Created by Josh McNamee on 07/10/2016.
//
//

#ifndef api_hpp
#define api_hpp

#include <stdio.h>
#include <OpenCL/opencl.h>

void init(int width, int height, bool share_opencl, cl_context c, cl_device_id d, cl_command_queue q);
void launch_threads();
float * update(bool share_opencl, bool update, cl_image load_image);
void close_down();

const char * _get_resource_path(const char * filename);

#endif /* api_hpp */
