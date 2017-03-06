//
//  radeonProcessor.cpp
//  App
//
//  Created by Josh McNamee on 08/10/2016.
//
//
#define _USE_MATH_DEFINES
#include <azure/glm/glm.hpp>
#include <azure/eventtypes.hpp>
#include <Mush Core/fixedExposureProcess.hpp>
#include <Mush Core/singleKernelProcess.hpp>
#include <Mush Core/fisheye2EquirectangularProcess.hpp>
#include <Mush Core/camera_base.hpp>
#include <Mush Core/camera_event_handler.hpp>

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

	_mush_camera = std::make_shared<mush::camera::base>();
	_mush_camera_event = std::make_shared<mush::camera::camera_event_handler>(_mush_camera);
	if (std::string(_config.automatic_camera_path).length() > 0) {
		_mush_camera_event->load_camera_path(_config.automatic_camera_path, 1.0f);
		auto_cam = true;
	}


    _quit = std::make_shared<mush::quitEventHandler>();
    
    bool is_spherical = (_config.camera == mush::camera_type::spherical_equirectangular);
    
    _rad_event = std::make_shared<radeonEventHandler>(is_spherical);

    _radeon = std::make_shared<radeonProcess>(_rad_event, _config.width, _config.height, _config.share_opencl);
	if (buffers.size() > 0) {

		_input = buffers.begin()[0];

        if (_config.environment_map_fish_eye) {
            _fish_eye = std::make_shared<mush::fisheye2EquirectangularProcess>(185.0f, 0.497f, 0.52f);
        
            _fish_eye->init(context, {buffers.begin()[0]});
			
            _fish_eye->setTagInGuiName("Fish Eye Projection");
            _radeon->init(context, {_fish_eye});

			//if (_per_frame == -1) {
			//	_fish_eye->removeRepeat();
			//}
        } else {
            _radeon->init(context, {buffers.begin()[0]});
        }

		if (_per_frame == -1) {
			buffers.begin()[0]->removeRepeat();
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
	_depth_copy = std::make_shared<mush::fixedExposureProcess>(0.0f);
	_depth_copy->init(context, _depth);

    if (_per_frame != -1) {
        _copy->removeRepeat();
		_depth->removeRepeat();
    }
    
    _radeon->setTagInGuiName("Renderer Output");
    _depth->setTagInGuiName("Renderer Depth Output");
    _normals->setTagInGuiName("Renderer Normals Output");
    _copy->setTagInGuiName("Copy");
	_copy->setTagInGuiName("Depth Copy");
    
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

	g_scene->camera_->Rotate(-M_PI/2.0f);
}

void radeonProcessor::process() {
	if (_tick == 0) {
		if (auto_cam) {
			if (_mush_camera_event->camera_path_finished()) {
				_quit->event(std::make_shared<azure::QuitEvent>());
			}
			_mush_camera_event->frame_tick();


			glm::vec4 cam_loc = glm::vec4(_mush_camera->get_location(), 1.0f);

			//cam_loc = glm::rotate((float)(-M_PI / 2.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * cam_loc;
			
			if (_config.stereo_displacement) {
				if (_config.camera == mush::camera_type::spherical_equirectangular) {
					cam_loc.z += _config.stereo_distance;
				}
			}
			g_scene->camera_->SetPosition({ cam_loc.x, cam_loc.y, cam_loc.z });
			if (_config.stereo_displacement) {
				if (_config.camera == mush::camera_type::perspective) {
					g_scene->camera_->MoveRight(_config.stereo_distance);
				}
			}

			float new_theta = _mush_camera->get_theta() + M_PI;
			float new_phi = _mush_camera->get_phi() - M_PI/2.0f;

			g_scene->camera_->Rotate(new_theta - prev_theta);
			g_scene->camera_->Tilt(new_phi - prev_phi);

			prev_theta = new_theta;
			prev_phi = new_phi;

			//g_scene->camera_->MoveWorldUp(16.0f);
			//g_scene->camera_->MoveRight(6.0f);
			g_scene->camera_->ApplyOculusTransform(RadeonRays::matrix(), RadeonRays::float3());

			_radeon->set_camera_change();
		}
		_radeon->set_change_environment();

	}

	if (_config.environment_map_fish_eye) {
		//if (_per_frame == -1) {
		//	_input->addRepeat();
		//}

		_fish_eye->process();

		//if (_per_frame == -1) {
		//	_input->removeRepeat();
		//}
	}

	if (_tick == _per_frame) {
		_copy->addRepeat();
		_depth->addRepeat();
	}

    _timer->process(_radeon);
    _timer->process(_depth);
    _timer->process(_normals);
    _timer->process(_copy);
    
    _timer->print_metered_report();

	if (_tick == _per_frame) {
		_output_copy->process();
		_depth_copy->process();
		_copy->removeRepeat();
		_depth->removeRepeat();
		_tick = -1;
	}
    
    if (_per_frame == -1) {
        _output_copy->process();
		_depth_copy->process();
    }

	_tick++;
}

void radeonProcessor::go() {
    while (!_quit->getQuit() && _radeon->good()) {
        process();
    }

	_rad_event->write_camera_path(_config.write_camera_path);
    
    _timer->print_final_report();
    _radeon->release();
    _depth->release();
    _normals->release();
    _copy->release();
	_output_copy->release();
	_depth_copy->release();
    if (_config.environment_map_fish_eye) {
        _fish_eye->release();
    }
}

std::vector<std::shared_ptr<mush::guiAccessible>> radeonProcessor::getGuiBuffers() {
    return {/*_radeon, */_copy, _depth, _normals, _output_copy, _depth_copy, _fish_eye};
}

const std::vector<std::shared_ptr<mush::ringBuffer>> radeonProcessor::getBuffers() const {
    return {_output_copy, _depth_copy};
}

std::vector<std::shared_ptr<azure::Eventable>> radeonProcessor::getEventables() const {
    return {_quit, _rad_event};
	//return{ _quit };
}
