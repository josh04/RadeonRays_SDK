//
//  radeonDepthProcess.hpp
//  App
//
//  Created by Josh McNamee on 12/11/2016.
//
//

#ifndef radeonDepthProcess_h
#define radeonDepthProcess_h

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

#include "radeonProcess.hpp"

class radeonDepthProcess : public mush::imageProcess {
public:
    radeonDepthProcess(cl::Image2D * image, bool flip) : mush::imageProcess(), _image(image), _flip(flip) {
        
    }
    
    ~radeonDepthProcess() {
        
    }
    
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) override {
        assert(buffers.size() == 1);
        
        _rad = std::dynamic_pointer_cast<radeonProcess>(buffers.begin()[0]);
        _rad->getParams(_width, _height, _size);
        
        if (_flip) {
            _copy = context->getKernel("flip_vertical");
        } else {
            _copy = context->getKernel("copyImage");
        }
        
        addItem(context->floatImage(_width, _height));
        queue = context->getQueue();
    }
    
    void process() override {
        _rad->outLock();
        inLock();
        
        _copy->setArg(0, *_image);
        _copy->setArg(1, *_getImageMem(0));
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*_copy, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        inUnlock();
        _rad->outUnlock();
    }
    
private:
    mush::registerContainer<radeonProcess> _rad;
    
    bool _flip;
    
    cl::Image2D * _image = nullptr;
    cl::Kernel * _copy = nullptr;
};


#endif /* radeonDepthProcess_h */
