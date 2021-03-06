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

radeonProcess::radeonProcess(std::shared_ptr<radeonEventHandler> rad_event, unsigned int width, unsigned int height, bool share_opencl, bool catch_exceptions) : mush::imageProcess(), _share_opencl(share_opencl), _rad_event(rad_event), _catch_exceptions(catch_exceptions) {
    _width = width;
    _height = height;
}

radeonProcess::~radeonProcess() {
    
}

void radeonProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 0 || buffers.size() == 1);
    
	unsigned int env_depth = 0;

	if (buffers.size() > 0 && buffers.begin()[0] != nullptr) {
		_environment_map = castToImage(buffers.begin()[0]);
		_environment_map->getParams(env_width, env_height, env_depth);

		env_down_buffer = context->hostReadBuffer(env_width * env_height * sizeof(cl_float4));

	}

    bool success = ::init(_width, _height, _share_opencl, context->get_cl_context(), context->get_cl_device(), (*context->getQueue())(), env_width, env_height);
	
	//g_scene->AddPointLight(RadeonRays::float3(0.0f, 16.0f, 6.0f), RadeonRays::float3(1.f, 0.9f, 0.6f) * 80.0f);

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
	bool up = _rad_event->update() || _external_camera_change;
	_external_camera_change = false;
	if (!call_once) {
		launch_threads();
		//up = true;
		call_once = true;
	}

	//_change_environment = _change_environment && up;

	if (_change_environment) {
		_change_environment = false;
		if (_environment_map.get() != nullptr) {
			auto ptr = _environment_map->outLock();

			if (ptr != nullptr) {
				cl::Event event;
				cl::size_t<3> origin, region;
				origin[0] = 0; origin[1] = 0; origin[2] = 0;
				region[0] = env_width; region[1] = env_height; region[2] = 1;

				queue->enqueueReadImage(ptr.get_image(), CL_TRUE, origin, region, 0, 0, env_down_buffer, NULL, &event);
				event.wait();

				update_environment(true, env_down_buffer);
			} else {
				_environment_map->outUnlock();
				release();
				return;
			}

			_environment_map->outUnlock();

		}
	} /*else {
		if (_environment_map.get() != nullptr) {
			auto ptr = _environment_map->outLock();
			_environment_map->outUnlock();
		}
	}*/


	auto buf = inLock();
	auto loc = g_scene->camera_->GetPosition();
	auto tpf = _rad_event->get_theta_phi_fov();
	tpf.s[2] = 75.0f;
	buf.set_camera_position({ loc.x, loc.y, loc.z }, tpf);
    
    update_return_type update_return;
	if (_catch_exceptions) {
		try {
			update_return = update_catch(_share_opencl, up, _getImageMem(0), (*depth_image)(), (*normals_image)());
		} catch (std::runtime_error& e) {
			putLog(e.what());
			release();
			return;
		}
	} else {
		update_return = update(_share_opencl, up, _getImageMem(0), (*depth_image)(), (*normals_image)());
	}
    auto ptr = update_return.image;
    auto depth_ptr = update_return.depth;
    auto normals_ptr = update_return.normals;
    
    if (!_share_opencl) {
        cl::Event event;
        cl::size_t<3> origin, region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _width; region[1] = _height; region[2] = 1;
    
        queue->enqueueWriteImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, ptr, NULL, &event);
        event.wait();
        
        _divide->setArg(0, _getImageMem(0));
        _divide->setArg(1, _getImageMem(0));
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
