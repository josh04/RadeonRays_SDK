//
//  resource.m
//  RadeonRays
//
//  Created by Josh McNamee on 10/10/2016.
//
//

#include <string>



const char * _get_resource_path(const char * filename) {
    const char * path = "../RadeonRays/src/kernels/";
    std::string output = std::string(path) + std::string(filename);
    path = output.c_str();

    return path;
}