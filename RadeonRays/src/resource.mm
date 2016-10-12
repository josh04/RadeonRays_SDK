//
//  resource.m
//  RadeonRays
//
//  Created by Josh McNamee on 10/10/2016.
//
//

#include <string>
#import <Foundation/Foundation.h>



const char * _get_resource_path(const char * filename) {
    const char * path = "../RadeonRays/src/kernels/";
    std::string output = std::string(path) + std::string(filename);
    path = output.c_str();
#ifdef RAD_FRAMEWORK
#ifdef __APPLE__
    NSString * resStr = [[[NSBundle bundleWithIdentifier:@"josh04.RadeonRays"] resourcePath] stringByAppendingString:@"/"];
    NSString * NSvertStr = [resStr stringByAppendingString:[NSString stringWithUTF8String:filename]];
    path = [NSvertStr UTF8String];
#endif
    #endif
    return path;
}