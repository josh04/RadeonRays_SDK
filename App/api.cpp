//
//  api.cpp
//  App
//
//  Created by Josh McNamee on 07/10/2016.
//
//

#include "api.hpp"

#include "OpenImageIO/imageio.h"

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include "GL/glew.h"
#include "GLUT/GLUT.h"
#else
#include <CL/cl.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glx.h>
#endif


#include <memory>
#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <thread>
#include <atomic>
#include <mutex>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef RR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

#include "CLW.h"

#include "math/mathutils.h"

#include "tiny_obj_loader.h"
#include "perspective_camera.h"
#include "shader_manager.h"
#include "Scene/scene.h"
#include "PT/ptrenderer.h"
#include "AO/aorenderer.h"
#include "CLW/clwoutput.h"
#include "config_manager.h"

#include <Mush Core/mushLog.hpp>

using namespace RadeonRays;

// Help message
char const* kHelpMessage =
"App [-p path_to_models][-f model_name][-b][-r][-ns number_of_shadow_rays][-ao ao_radius][-w window_width][-h window_height][-nb number_of_indirect_bounces]";
//char const* g_path = "bmw";
//char const* g_modelname = "i8.obj";

//std::unique_ptr<ShaderManager>    g_shader_manager;

//GLuint g_vertex_buffer;
//GLuint g_index_buffer;
//GLuint g_texture;


char const* g_path = "bmw";
char const* g_modelname = "i8.obj";
int g_window_width = 1280;
int g_window_height = 720;
int g_num_shadow_rays = 1;
int g_num_ao_rays = 1;
int g_ao_enabled = false;
int g_progressive = false;
int g_num_bounces = 5;
int g_num_samples = -1;
int g_samplecount = 0;
float g_ao_radius = 1.f;
float g_envmapmul = 1.f;

const char * g_environment_map_path = "Textures";
const char * g_environment_map_name = "studio015.hdr";

float g_cspeed = 100.25f;

float3 g_camera_pos = float3(0.f, 1.f, 4.f);
float3 g_camera_at = float3(0.f, 1.f, 0.f);
float3 g_camera_up = float3(0.f, 1.f, 0.f);

float2 g_camera_sensor_size = float2(0.036f, 0.024f);  // default full frame sensor 36x24 mm
float2 g_camera_zcap = float2(0.0f, 100000.f);
float g_camera_focal_length = 0.035f; // 35mm lens
float g_camera_focus_distance = 0.f;
float g_camera_aperture = 0.f;


bool g_recording_enabled = false;
int g_frame_count = 0;
bool g_benchmark = false;
bool g_interop = true;
ConfigManager::Mode g_mode = ConfigManager::Mode::kUseSingleGpu;

using namespace tinyobj;


struct OutputData
{
    Baikal::ClwOutput* output;
    std::vector<float3> fdata;
    // JOSH
    std::vector<float> depth_data;
    std::vector<float3> normals_data;
    std::vector<unsigned char> udata;
    CLWBuffer<float3> copybuffer;
};

struct ControlData
{
    std::atomic<int> clear;
    std::atomic<int> stop;
    std::atomic<int> newdata;
    std::mutex datamutex;
    int idx;
};

std::vector<ConfigManager::Config> g_cfgs;
std::vector<OutputData> g_outputs;
std::unique_ptr<ControlData[]> g_ctrl;
std::vector<std::thread> g_renderthreads;
int g_primary = -1;


std::unique_ptr<Baikal::Scene> g_scene;

mush::camera_type g_camera_type;

static bool     g_is_left_pressed = false;
static bool     g_is_right_pressed = false;
static bool     g_is_fwd_pressed = false;
static bool     g_is_back_pressed = false;
static bool     g_is_home_pressed = false;
static bool     g_is_end_pressed = false;
static bool     g_is_mouse_tracking = false;
static float2   g_mouse_pos = float2(0, 0);
static float2   g_mouse_delta = float2(0, 0);


void setup(const mush::radeonConfig& config) {
	g_path = config.path;
	g_modelname = config.model_name;

	g_num_shadow_rays = config.shadow_rays;
	g_num_ao_rays = config.ao_rays;
	g_ao_enabled = config.ao_enabled;
	g_progressive = config.progressive_enabled;
	g_num_bounces = config.num_bounces;
	g_num_samples = config.num_samples;

	g_ao_radius = config.ao_radius;
	g_camera_pos = { config.camera_position.s0, config.camera_position.s1, config.camera_position.s2 };
	g_camera_sensor_size = { config.camera_sensor_size.s0, config.camera_sensor_size.s1 };
	g_camera_zcap = { config.camera_zcap.s0, config.camera_zcap.s1 };
	g_camera_focal_length = config.camera_focal_length;
	g_camera_focus_distance = config.camera_focus_distance;
	g_camera_aperture = config.camera_aperture;
    
    g_envmapmul = config.environment_map_mult;
    g_environment_map_name = config.environment_map_name;
    g_environment_map_path = config.environment_map_path;

	g_camera_type = config.camera;
}

void init_cl(bool share_opencl, cl_context c, cl_device_id d, cl_command_queue q) {
    g_cfgs.clear();
    
    if (!share_opencl) {
    ConfigManager::CreateConfigs(g_mode, false, g_cfgs, g_num_bounces);
    } else {
        ConfigManager::Config cfg;
        
        cfg.context = CLWContext::Create(c, &d, &q, 1);
        cfg.devidx = 0;
        cfg.type = ConfigManager::kPrimary;

        g_cfgs.push_back(cfg);

        for (int i = 0; i < g_cfgs.size(); ++i) {
            g_cfgs[i].renderer = new Baikal::PtRenderer(g_cfgs[i].context, g_cfgs[i].devidx, g_num_bounces);
        }
    }

    putLog("AMD: Running on devices:");

    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        std::stringstream strm;
        strm << "AMD: " << i << ": " << g_cfgs[i].context.GetDevice(g_cfgs[i].devidx).GetName();
        putLog(strm.str());
    }
    
    g_interop = false;
    
    g_outputs.resize(g_cfgs.size());
    g_ctrl.reset(new ControlData[g_cfgs.size()]);
    
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        if (g_cfgs[i].type == ConfigManager::kPrimary)
        {
            g_primary = i;
        }
        
        g_ctrl[i].clear.store(1);
        g_ctrl[i].stop.store(0);
        g_ctrl[i].newdata.store(0);
        g_ctrl[i].idx = i;
    }
    if (share_opencl)
    {
        putLog("AMD: OpenGL interop mode enabled.");
    }
    else
    {
        putLog("AMD: OpenGL interop mode disabled.");
    }
    
}
/*
void InitCl()
{
    bool force_disable_itnerop = false;
    
    try
    {
        ConfigManager::CreateConfigs(g_mode, g_interop, g_cfgs, g_num_bounces);
    }
    catch (CLWException &)
    {
        force_disable_itnerop = true;
        ConfigManager::CreateConfigs(g_mode, false, g_cfgs, g_num_bounces);
    }
    
    
    std::cout << "Running on devices: \n";
    
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        std::cout << i << ": " << g_cfgs[i].context.GetDevice(g_cfgs[i].devidx).GetName() << "\n";
    }
    
    g_interop = false;
    
    g_outputs.resize(g_cfgs.size());
    g_ctrl.reset(new ControlData[g_cfgs.size()]);
    
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        if (g_cfgs[i].type == ConfigManager::kPrimary)
        {
            g_primary = i;
            
            if (g_cfgs[i].caninterop)
            {
                g_cl_interop_image = g_cfgs[i].context.CreateImage2DFromGLTexture(g_texture);
                g_interop = true;
            }
        }
        
        g_ctrl[i].clear.store(1);
        g_ctrl[i].stop.store(0);
        g_ctrl[i].newdata.store(0);
        g_ctrl[i].idx = i;
    }
    
    if (force_disable_itnerop)
    {
        std::cout << "OpenGL interop is not supported, disabled, -interop flag is ignored\n";
    }
    else
    {
        if (g_interop)
        {
            std::cout << "OpenGL interop mode enabled\n";
        }
        else
        {
            std::cout << "OpenGL interop mode disabled\n";
        }
    }
}
*/
void InitData()
{
    
    rand_init();
    
    // Load obj file
    std::string basepath = g_path;
    basepath += "/";
    std::string filename = basepath + g_modelname;
    
    g_scene.reset(Baikal::Scene::LoadFromObj(filename, basepath));
    
    g_scene->camera_.reset(new PerspectiveCamera(
                                                 g_camera_pos
                                                 , g_camera_at
                                                 , g_camera_up));
    
    // Adjust sensor size based on current aspect ratio
    float aspect = (float)g_window_width / g_window_height;
    g_camera_sensor_size.y = g_camera_sensor_size.x / aspect;

	switch (g_camera_type) {
	case mush::camera_type::perspective:
		g_scene->camera_type_ = (int)Baikal::CameraType::kDefault;
			break;
	case mush::camera_type::perspective_dof:
		g_scene->camera_type_ = (int)Baikal::CameraType::kPhysical;
		break;
	case mush::camera_type::spherical_equirectangular:
		g_scene->camera_type_ = (int)Baikal::CameraType::kSpherical;
		break;
	}
    
    g_scene->camera_->SetSensorSize(g_camera_sensor_size);
    g_scene->camera_->SetDepthRange(g_camera_zcap);
    g_scene->camera_->SetFocalLength(g_camera_focal_length);
    g_scene->camera_->SetFocusDistance(g_camera_focus_distance);
    g_scene->camera_->SetAperture(g_camera_aperture);
    
    std::stringstream strm;
    strm << "AMD: Camera type: " << (g_scene->camera_->GetAperture() > 0.f ? "Physical" : "Pinhole") << "\n";
    strm << "AMD: Lens focal length: " << g_scene->camera_->GetFocalLength() * 1000.f << "mm\n";
    strm << "AMD: Lens focus distance: " << g_scene->camera_->GetFocusDistance() << "m\n";
    strm << "AMD: F-Stop: " << 1.f / (g_scene->camera_->GetAperture() * 10.f) << "\n";
    strm << "AMD: Sensor size: " << g_camera_sensor_size.x * 1000.f << "x" << g_camera_sensor_size.y * 1000.f << "mm\n";
    putLog(strm.str());
    
    g_scene->SetEnvironment(g_environment_map_name, g_environment_map_path, g_envmapmul);
    
#pragma omp parallel for
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        //g_cfgs[i].renderer->SetNumBounces(g_num_bounces);
        g_cfgs[i].renderer->Preprocess(*g_scene);
        
        g_outputs[i].output = (Baikal::ClwOutput*)g_cfgs[i].renderer->CreateOutput(g_window_width, g_window_height);
        
        g_cfgs[i].renderer->SetOutput(g_outputs[i].output);
        
        g_outputs[i].fdata.resize(g_window_width * g_window_height);
        // JOSH
        g_outputs[i].depth_data.resize(g_window_width * g_window_height);
        g_outputs[i].normals_data.resize(g_window_width * g_window_height);
        g_outputs[i].udata.resize(g_window_width * g_window_height * 4);
        
        if (g_cfgs[i].type == ConfigManager::kPrimary)
        {
            g_outputs[i].copybuffer = g_cfgs[i].context.CreateBuffer<float3>(g_window_width * g_window_height, CL_MEM_READ_WRITE);
        }
    }
    
    g_cfgs[g_primary].renderer->Clear(float3(0, 0, 0), *g_outputs[g_primary].output);
}

void RenderThread(ControlData& cd)
{
    auto renderer = g_cfgs[cd.idx].renderer;
    auto output = g_outputs[cd.idx].output;
    
    auto updatetime = std::chrono::high_resolution_clock::now();
    
    while (!cd.stop.load())
    {
        int result = 1;
        bool update = false;
        
        if (std::atomic_compare_exchange_strong(&cd.clear, &result, 0))
        {
            renderer->Clear(float3(0, 0, 0), *output);
            update = true;
        }
        
        renderer->Render(*g_scene.get());
        
        auto now = std::chrono::high_resolution_clock::now();
        
        update = update || (std::chrono::duration_cast<std::chrono::seconds>(now - updatetime).count() > 1);
        
        if (update)
        {
            g_outputs[cd.idx].output->GetData(&g_outputs[cd.idx].fdata[0]);
            updatetime = now;
            cd.newdata.store(1);
        }
        
        g_cfgs[cd.idx].context.Finish(0);
    }
}


void StartRenderThreads()
{
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        if (i != g_primary)
        {
            g_renderthreads.push_back(std::thread(RenderThread, std::ref(g_ctrl[i])));
            g_renderthreads.back().detach();
        }
    }
    
    std::stringstream strm;
    strm << "AMD: " << g_cfgs.size() << " OpenCL submission threads started.";
    putLog(strm.str());
}

bool init(int width, int height, bool share_opencl, cl_context c, cl_device_id d, cl_command_queue q) {
    g_window_width = width;
    g_window_height = height;
	bool exception_thrown = false;
    try
    {
        init_cl(share_opencl, c, d, q);
        InitData();
    }
    catch (std::runtime_error& err)
    {
        putLog(err.what());
		exception_thrown = true;
    }
	return !exception_thrown;
}

void launch_threads() {
    StartRenderThreads();
}

update_return_type update(bool share_opencl, bool update, cl_mem load_image, cl_mem depth_image, cl_mem normals_image) {
    
    if (update)
    {
        g_scene->set_dirty(Baikal::Scene::kCamera);
        
        if (g_num_samples > -1)
        {
            g_samplecount = 0;
        }
        
        for (int i = 0; i < g_cfgs.size(); ++i)
        {
            if (i == g_primary)
                g_cfgs[i].renderer->Clear(float3(0, 0, 0), *g_outputs[i].output);
            else
                g_ctrl[i].clear.store(true);
        }
        
    }
    
    if (g_num_samples == -1 || g_samplecount++ < g_num_samples)
    {
        g_cfgs[g_primary].renderer->Render(*g_scene.get());
    }
    
    
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        if (g_cfgs[i].type == ConfigManager::kPrimary)
            continue;
        
        int desired = 1;
        if (std::atomic_compare_exchange_strong(&g_ctrl[i].newdata, &desired, 0))
        {
            {
                //std::unique_lock<std::mutex> lock(g_ctrl[i].datamutex);
                //std::cout << "Start updating acc buffer\n"; std::cout.flush();
                g_cfgs[g_primary].context.WriteBuffer(0, g_outputs[g_primary].copybuffer, &g_outputs[i].fdata[0], g_window_width * g_window_height);
                //std::cout << "Finished updating acc buffer\n"; std::cout.flush();
            }
            
            CLWKernel acckernel = g_cfgs[g_primary].renderer->GetAccumulateKernel();
            
            int argc = 0;
            acckernel.SetArg(argc++, g_outputs[g_primary].copybuffer);
            acckernel.SetArg(argc++, g_window_width * g_window_width);
            acckernel.SetArg(argc++, g_outputs[g_primary].output->data());
            
            int globalsize = g_window_width * g_window_height;
            g_cfgs[g_primary].context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, acckernel);
        }
    }
    
    if (share_opencl) {
        CLWKernel copykernel = g_cfgs[g_primary].renderer->GetCopyKernel();
        
        int argc = 0;
        copykernel.SetArg(argc++, g_outputs[g_primary].output->data());
        copykernel.SetArg(argc++, g_outputs[g_primary].output->width());
        copykernel.SetArg(argc++, g_outputs[g_primary].output->height());
        copykernel.SetArg(argc++, 1.0f);
        
        auto err = clSetKernelArg(copykernel, argc++, sizeof(cl_mem), &load_image);
        
        //copykernel.SetArg(argc++, load_image);
        
        int globalsize = g_outputs[g_primary].output->width() * g_outputs[g_primary].output->height();
        g_cfgs[g_primary].context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, copykernel);
        
        argc = 0;
        copykernel.SetArg(argc++, g_outputs[g_primary].output->normals_data());
        copykernel.SetArg(argc++, g_outputs[g_primary].output->width());
        copykernel.SetArg(argc++, g_outputs[g_primary].output->height());
        copykernel.SetArg(argc++, 1.0f);
        
        auto err3 = clSetKernelArg(copykernel, argc++, sizeof(cl_mem), &normals_image);
        
        //copykernel.SetArg(argc++, load_image);
        
        g_cfgs[g_primary].context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, copykernel);
        
        //g_cfgs[g_primary].context.ReleaseGLObjects(0, objects);
        
        CLWKernel depthcopykernel = g_cfgs[g_primary].renderer->GetDepthCopyKernel();
        
        argc = 0;
        depthcopykernel.SetArg(argc++, g_outputs[g_primary].output->depth_data());
        depthcopykernel.SetArg(argc++, g_outputs[g_primary].output->width());
        depthcopykernel.SetArg(argc++, g_outputs[g_primary].output->height());
        
        auto err2 = clSetKernelArg(depthcopykernel, argc++, sizeof(cl_mem), &depth_image);
        
        g_cfgs[g_primary].context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, depthcopykernel);
        
        g_cfgs[g_primary].context.Finish(0);
    } else {
        
        g_outputs[g_primary].output->GetData(&g_outputs[g_primary].fdata[0]);
        g_outputs[g_primary].output->GetDepthData(&g_outputs[g_primary].depth_data[0]);
        g_outputs[g_primary].output->GetNormalsData(&g_outputs[g_primary].normals_data[0]);
        /*
        float gamma = 2.2f;
        for (int i = 0; i < (int)g_outputs[g_primary].fdata.size(); ++i)
        {
            g_outputs[g_primary].udata[4 * i] = (unsigned char)clamp(clamp(pow(g_outputs[g_primary].fdata[i].x / g_outputs[g_primary].fdata[i].w, 1.f / gamma), 0.f, 1.f) * 255, 0, 255);
            g_outputs[g_primary].udata[4 * i + 1] = (unsigned char)clamp(clamp(pow(g_outputs[g_primary].fdata[i].y / g_outputs[g_primary].fdata[i].w, 1.f / gamma), 0.f, 1.f) * 255, 0, 255);
            g_outputs[g_primary].udata[4 * i + 2] = (unsigned char)clamp(clamp(pow(g_outputs[g_primary].fdata[i].z / g_outputs[g_primary].fdata[i].w, 1.f / gamma), 0.f, 1.f) * 255, 0, 255);
            g_outputs[g_primary].udata[4 * i + 3] = 1;
        }
        */
    }
    return { (float *)g_outputs[g_primary].fdata.data(), g_outputs[g_primary].depth_data.data(),  (float *)g_outputs[g_primary].normals_data.data()  };
    
}

void close_down() {
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        if (i == g_primary)
            continue;
        
        g_ctrl[i].stop.store(true);
    }
}


