//
//  radeonProcessor.cpp
//  App
//
//  Created by Josh McNamee on 08/10/2016.
//
//

#include <Mush Core/fixedExposureProcess.hpp>
#include <Mush Core/singleKernelProcess.hpp>
#include <Mush Core/fisheye2EquirectangularProcess.hpp>

#include <Mush Core/timerWrapper.hpp>

#include "radeonEventHandler.hpp"
#include "radeonProcessor.hpp"
#include "radeonProcess.hpp"
#include "radeonDepthProcess.hpp"
#include "api.hpp"

radeonProcessor::radeonProcessor(const mush::radeonConfig& config) : mush::imageProcessor(), _config(config) {
	_per_frame = config.num_samples;
	_config.num_samples = -1;
}

radeonProcessor::~radeonProcessor() {
    
}

void radeonProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) {
	setup(_config);

    _quit = std::make_shared<mush::quitEventHandler>();
    
    bool is_spherical = (_config.camera == mush::camera_type::spherical_equirectangular);
    
    _rad_event = std::make_shared<radeonEventHandler>(is_spherical);
    
    bool env_map_set_dirty = true;
    if (_per_frame < 1) {
        env_map_set_dirty = false;
    }
    _radeon = std::make_shared<radeonProcess>(_rad_event, _config.width, _config.height, _config.share_opencl, env_map_set_dirty);
	if (buffers.size() > 0) {
        
        if (_config.environment_map_fish_eye) {
            _fish_eye = std::make_shared<mush::fisheye2EquirectangularProcess>(185.0f, 0.497f, 0.52f);
        
            _fish_eye->init(context, {buffers.begin()[0]});
            _fish_eye->setTagInGuiName("Fish Eye Projection");
            _radeon->init(context, {_fish_eye});
        } else {
            _radeon->init(context, {buffers.begin()[0]});
        }
	} else {
		_radeon->init(context, {});
	}
    
    _depth = std::make_shared<radeonDepthProcess>(_radeon->get_depth_image(), true);
    _depth->init(context, {_radeon});
    
    _normals = std::make_shared<radeonDepthProcess>(_radeon->get_normals_image(), true);
    _normals->init(context, {_radeon});
    
    _copy = std::make_shared<mush::singleKernelProcess>("flip_vertical");
    _copy->init(context, _radeon);

	_output_copy = std::make_shared<mush::fixedExposureProcess>(0.0f);
	_output_copy->init(context, _copy);

    if (_per_frame != -1) {
        _copy->removeRepeat();
    }
    
    _radeon->setTagInGuiName("Renderer Output");
    _depth->setTagInGuiName("Renderer Depth Output");
    _normals->setTagInGuiName("Renderer Normals Output");
    _copy->setTagInGuiName("Copy");
    
    _timer = std::unique_ptr<mush::timerWrapper>(new mush::timerWrapper("RR: "));
    /*
    if (_config.environment_map_fish_eye) {
        _timer->register_node(_fish_eye, "Fish eye");
    }
    */
    
    _timer->register_node(_radeon, "Raytracer");
    _timer->register_node(_depth, "Depth");
    _timer->register_node(_normals, "Normals");
    _timer->register_node(_copy, "Vertical Flip");
}

void radeonProcessor::process() {
	if (_tick == 0) {
		_radeon->set_change_environment();
        if (_config.environment_map_fish_eye) {
            _fish_eye->process();
        }
	}

	if (_tick == _per_frame) {
		_copy->addRepeat();
	}

    _timer->process(_radeon);
    _timer->process(_depth);
    _timer->process(_normals);
    _timer->process(_copy);
    
    _timer->print_metered_report();

	if (_tick == _per_frame) {
		_output_copy->process();
		_copy->removeRepeat();
		_tick = -1;
	}
    
    if (_per_frame == -1) {
        _output_copy->process();
    }

	_tick++;
}

void radeonProcessor::go() {
    while (!_quit->getQuit() && _radeon->good()) {
        process();
    }
    
    _timer->print_final_report();
    _radeon->release();
    _depth->release();
    _normals->release();
    _copy->release();
	_output_copy->release();
    if (_config.environment_map_fish_eye) {
        _fish_eye->release();
    }
}

std::vector<std::shared_ptr<mush::guiAccessible>> radeonProcessor::getGuiBuffers() {
    return {/*_radeon, */_copy, _depth, _normals, _output_copy, _fish_eye};
}

const std::vector<std::shared_ptr<mush::ringBuffer>> radeonProcessor::getBuffers() const {
    return {_output_copy};
}

std::vector<std::shared_ptr<azure::Eventable>> radeonProcessor::getEventables() const {
    return {_quit, _rad_event};
}
