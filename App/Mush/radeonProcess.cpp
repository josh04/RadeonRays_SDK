//
//  radeonProcess.cpp
//  App
//
//  Created by Josh McNamee on 09/10/2016.
//
//

#include <Mush Core/opencl.hpp>
#include "radeonProcess.hpp"
#include "radeonEventHandler.hpp"

#include "api.hpp"

radeonProcess::radeonProcess(std::shared_ptr<radeonEventHandler> rad_event, unsigned int width, unsigned int height, bool share_opencl) : mush::imageProcess(), _share_opencl(share_opencl), _rad_event(rad_event) {
    _width = width;
    _height = height;
}

radeonProcess::~radeonProcess() {
    
}

void radeonProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer> > &buffers) {
    assert(buffers.size() == 0);
    
    bool success = ::init(_width, _height, _share_opencl, context->get_cl_context(), context->get_cl_device(), (*context->getQueue())());
    
	if (!success) {
		kill();
		throw std::runtime_error("Exception thrown in RadeonRays.");
	}

    //float_image = context->floatImage(_width, _height);
    
    _divide = context->getKernel("amd_divide");
    _depth_to_image = context->getKernel("parDepthToImageNoSamples");

    
    depth_image = context->floatImage(_width, _height);
    normals_image = context->floatImage(_width, _height);
    depth_buffer = context->buffer(_width * _height * sizeof(float));
    
    addItem(context->floatImage(_width, _height));
    queue = context->getQueue();
}

void radeonProcess::process() {
    bool up = _rad_event->update();
    
    if (!call_once) {
        launch_threads();
        //up = true;
        call_once = true;
    }
    
    
    inLock();
    
    auto update_return = update(_share_opencl, up, (*_getImageMem(0))(), (*depth_image)(), (*normals_image)());
    auto ptr = update_return.image;
    auto depth_ptr = update_return.depth;
    auto normals_ptr = update_return.normals;
    
    if (!_share_opencl) {
        cl::Event event;
        cl::size_t<3> origin, region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _width; region[1] = _height; region[2] = 1;
    
        queue->enqueueWriteImage(*_getImageMem(0), CL_TRUE, origin, region, 0, 0, ptr, NULL, &event);
        event.wait();
        
        _divide->setArg(0, *_getImageMem(0));
        _divide->setArg(1, *_getImageMem(0));
        queue->enqueueNDRangeKernel(*_divide, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        queue->enqueueWriteBuffer(*depth_buffer, CL_TRUE, 0, _width*_height*sizeof(float), depth_ptr, NULL, &event);
        event.wait();
        
        _depth_to_image->setArg(0, *depth_buffer);
        _depth_to_image->setArg(1, *depth_image);
        queue->enqueueNDRangeKernel(*_depth_to_image, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        //queue->enqueueWriteImage(*depth_image, CL_TRUE, origin, region, 0, 0, depth_ptr, NULL, &event);
        //event.wait();
        
        queue->enqueueWriteImage(*normals_image, CL_TRUE, origin, region, 0, 0, normals_ptr, NULL, &event);
        event.wait();
        
        _divide->setArg(0, *normals_image);
        _divide->setArg(1, *normals_image);
        queue->enqueueNDRangeKernel(*_divide, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
    }
    
    inUnlock();
    
}


void radeonProcess::release() {
    close_down();
    imageProcess::release();
}