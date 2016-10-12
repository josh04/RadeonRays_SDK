//
//  radeonProcess.hpp
//  App
//
//  Created by Josh McNamee on 09/10/2016.
//
//

#ifndef radeonProcess_hpp
#define radeonProcess_hpp

#include <Mush Core/imageProcess.hpp>

class radeonEventHandler;

class radeonProcess : public mush::imageProcess {
public:
    radeonProcess(std::shared_ptr<radeonEventHandler> rad_event, unsigned int width, unsigned int height, bool share_opencl);
    ~radeonProcess();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) override;
    
    void process();
    
    void release() override;
private:
    cl::Image2D * float_image = nullptr;
    cl::Kernel * _divide = nullptr;
    
    bool call_once = false;
    
    bool _share_opencl;
    
    std::shared_ptr<radeonEventHandler> _rad_event = nullptr;
};

#endif /* radeonProcess_hpp */

