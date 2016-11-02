//
//  radeonConfig.hpp
//  App
//
//  Created by Josh McNamee on 02/11/2016.
//
//

#ifndef radeonConfig_h
#define radeonConfig_h

#include "CLW.h"

namespace mush {
    struct radeonConfig {
        /*
        void defaults() {
            
        }
         */
        
        int width = 1280;
        int height = 720;
        
        char const * path = "bmw";
        char const * model_name = "i8.obj";
        
        int shadow_rays = 1;
        int ao_rays = 1;
        int ao_enabled = false;
        int progressive_enabled = false;
        int num_bounces = 5;
        int num_samples = -1;
        
        float ao_radius = 1.f;
        
        float3 camera_position = float3(0.f, 1.f, 4.f);
        
        float2 camera_sensor_size = float2(0.036f, 0.024f);  // default full frame sensor 36x24 mm
        float2 camera_zcap = float2(0.0f, 100000.f);
        float camera_focal_length = 0.035f; // 35mm lens
        float camera_focus_distance = 0.f;
        float camera_aperture = 0.f;
        
    };
}


#endif /* radeonConfig_h */
