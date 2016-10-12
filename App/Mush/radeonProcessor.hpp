//
//  radeonProcessor.hpp
//  App
//
//  Created by Josh McNamee on 08/10/2016.
//
//

#ifndef radeonProcessor_hpp
#define radeonProcessor_hpp

#include <memory>
#include <Mush Core/imageProcessor.hpp>
#include <Mush Core/quitEventHandler.hpp>

class radeonProcess;
class radeonEventHandler;

namespace mush {
    class timerWrapper;
}

class radeonProcessor : public mush::imageProcessor {
public:
    radeonProcessor(unsigned int width, unsigned int height);
    ~radeonProcessor();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) override;
    
    void process() override;
    
    void go() override;
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;
    
    std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override {
        return std::vector<std::shared_ptr<mush::frameStepper>>();
    }
    
    std::vector<std::shared_ptr<azure::Eventable>> getEventables() const override;
private:
    std::shared_ptr<radeonProcess> _radeon = nullptr;
    std::shared_ptr<mush::imageProcess> _copy = nullptr;
    
    
    std::shared_ptr<mush::quitEventHandler> _quit = nullptr;
    std::shared_ptr<radeonEventHandler> _rad_event = nullptr;
    
    std::unique_ptr<mush::timerWrapper> _timer = nullptr;
    
    unsigned int _width, _height;
};

#endif /* radeonProcessor_hpp */


