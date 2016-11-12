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

#include <Mush Core/opencl.hpp>
#include "Mush/radeonConfig.hpp"

void setup(const mush::radeonConfig& config);
bool init(int width, int height, bool share_opencl, cl_context c, cl_device_id d, cl_command_queue q);
void launch_threads();

struct update_return_type {
    float * image;
    float * depth;
    float * normals;
};

update_return_type update(bool share_opencl, bool update, cl_mem load_image, cl_mem depth_image, cl_mem normals_image);
void close_down();

const char * _get_resource_path(const char * filename);

#endif /* api_hpp */
