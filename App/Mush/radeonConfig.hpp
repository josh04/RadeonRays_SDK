//
//  radeonConfig.hpp
//  App
//
//  Created by Josh McNamee on 02/11/2016.
//
//

#ifndef radeonConfig_h
#define radeonConfig_h

#include <Mush Core/opencl.hpp>

namespace mush {
	enum class camera_type
	{
		perspective,
		perspective_dof,
		spherical_equirectangular
	};

    struct radeonConfig {
        
        void defaults() {

			width = 1280;
			height = 720;

			share_opencl = false;

			path = "bmw";
			model_name = "i8.obj";

			shadow_rays = 1;
			ao_rays = 1;
			ao_enabled = false;
			progressive_enabled = false;
			num_bounces = 5;
			num_samples = -1;

			ao_radius = 1.f;

			camera_position = { 0.f, 1.f, 4.f };

			camera_sensor_size = { 0.036f, 0.024f };  // default full frame sensor 36x24 mm
			camera_zcap = { 0.0f, 100000.f };
			camera_focal_length = 0.035f; // 35mm lens
			camera_focus_distance = 0.f;
			camera_aperture = 0.f;
            
            environment_map_mult = 1.0f;
            environment_map_path = "Textures";
            environment_map_name = "studio015.hdr";

			//environment_width = 1280;
			//environment_height = 720;

			camera = camera_type::perspective;

			model_scale = 1.0f;
            
            environment_map_fish_eye = false;
        }
         
        
        int width;
        int height;

		bool share_opencl;
        
        char const * path;
        char const * model_name;
        
        int shadow_rays;
        int ao_rays;
        int ao_enabled;
        int progressive_enabled;
        int num_bounces;
        int num_samples;
        
        float ao_radius;
        
        cl_float3 camera_position;
        
        cl_float2 camera_sensor_size;  // default full frame sensor 36x24 mm
        cl_float2 camera_zcap;
        float camera_focal_length; // 35mm lens
        float camera_focus_distance;
        float camera_aperture;
        
        float environment_map_mult;
        const char * environment_map_path;
        const char * environment_map_name;

		//int environment_width;
		//int environment_height;
        
		camera_type camera;
		float model_scale;
        
        bool environment_map_fish_eye;
    };
}


#endif /* radeonConfig_h */
