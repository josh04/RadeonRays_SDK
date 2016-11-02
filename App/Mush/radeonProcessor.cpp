//
//  radeonProcessor.cpp
//  App
//
//  Created by Josh McNamee on 08/10/2016.
//
//

#include <Mush Core/fixedExposureProcess.hpp>
#include <Mush Core/singleKernelProcess.hpp>

#include <Mush Core/timerWrapper.hpp>

#include "radeonEventHandler.hpp"
#include "radeonProcessor.hpp"
#include "radeonProcess.hpp"

radeonProcessor::radeonProcessor(mush::radeonConfig config) : mush::imageProcessor(), _width(width), _height(height) {
    
}

radeonProcessor::~radeonProcessor() {
    
}

void radeonProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) {
    
    _quit = std::make_shared<mush::quitEventHandler>();
    _rad_event = std::make_shared<radeonEventHandler>();
    
    _radeon = std::make_shared<radeonProcess>(_rad_event, _width, _height, true);
    _radeon->init(context, {});
    _copy = std::make_shared<mush::singleKernelProcess>("flip_vertical");
    _copy->init(context, _radeon);
    
    _radeon->setTagInGuiName("Renderer Output");
    _copy->setTagInGuiName("Copy");
    
    _timer = std::unique_ptr<mush::timerWrapper>(new mush::timerWrapper("RR: "));
    
    _timer->register_node(_radeon, "Raytracer");
    _timer->register_node(_copy, "Vertical Flip");
}

void radeonProcessor::process() {
    _timer->process(_radeon);
    _timer->process(_copy);
    
    _timer->print_metered_report();
}

void radeonProcessor::go() {
    while (!_quit->getQuit()) {
        process();
    }
    
    _timer->print_final_report();
    _radeon->release();
    _copy->release();
}

std::vector<std::shared_ptr<mush::guiAccessible>> radeonProcessor::getGuiBuffers() {
    return {/*_radeon, */_copy};
}

const std::vector<std::shared_ptr<mush::ringBuffer>> radeonProcessor::getBuffers() const {
    return {_copy};
}

std::vector<std::shared_ptr<azure::Eventable>> radeonProcessor::getEventables() const {
    return {_quit, _rad_event};
}
