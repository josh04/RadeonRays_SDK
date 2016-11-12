//
//  resource.m
//  RadeonRays
//
//  Created by Josh McNamee on 10/10/2016.
//
//

#include <string>

std::string _get_resource_path(const char * filename) {
	std::string path = "RadeonResources/" + std::string(filename);
#ifdef RAD_FRAMEWORK
#ifdef __APPLE__
	NSString * resStr = [[[NSBundle bundleWithIdentifier : @"josh04.RadeonRays"] resourcePath] stringByAppendingString:@" / "];
		NSString * NSvertStr = [resStr stringByAppendingString : [NSString stringWithUTF8String : filename]];
	path = [NSvertStr UTF8String];
	return path;
#endif
#endif
	return path;
}
/*
const char * _get_resource_path(const char * filename) {
    const char * path = "../RadeonRays/src/kernels/";
    std::string output = std::string(path) + std::string(filename);
    path = output.c_str();

    return path;
}
*/