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
#include <Mush Core/timerWrapper.hpp>

#include "radeonConfig.hpp"
#include "mush-radeon-dll.hpp"

class radeonProcess;
class radeonDepthProcess;
class radeonEventHandler;

namespace mush {
	class imageProcess;
    class timerWrapper;
    class imageProcessor;
}

class RADEONEXPORTS_API radeonProcessor : public mush::imageProcessor {
public:
    radeonProcessor(const mush::radeonConfig& config);
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
    std::shared_ptr<radeonDepthProcess> _depth = nullptr;
    std::shared_ptr<radeonDepthProcess> _normals = nullptr;
    std::shared_ptr<mush::imageProcess> _copy = nullptr;
    std::shared_ptr<mush::imageProcess> _output_copy = nullptr;
    std::shared_ptr<mush::imageProcess> _fish_eye = nullptr;
    
    
    std::shared_ptr<mush::quitEventHandler> _quit = nullptr;
    std::shared_ptr<radeonEventHandler> _rad_event = nullptr;
    
    std::unique_ptr<mush::timerWrapper> _timer = nullptr;
    
	mush::radeonConfig _config;

	int _tick = 0;
	int _per_frame = 20;
};

#endif /* radeonProcessor_hpp */


