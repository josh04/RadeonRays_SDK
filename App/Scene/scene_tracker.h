/**********************************************************************
 Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ********************************************************************/

/**
 \file scene_tracker.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains SceneTracker class implementation.
 */
#pragma once

#include <map>
#include "CLW.h"

#include "radeon_rays_cl.h"

namespace Baikal
{
    class Scene1;
    struct ClwScene;
    class Collector;
    class Bundle;
    class Material;
    class Light;
    class Texture;
    

    /**
     \brief Tracks changes of a scene and serialized data into GPU memory when needed.
     
     SceneTracker class is intended to keep track of CPU side scene changes and update all
     necessary GPU buffers. It essentially establishes a mapping between Scene class and 
     corresponding ClwScene class. It also pre-caches ClwScenes and speeds up loading for 
     already compiled scenes.
     */
    class SceneTracker
    {
    public:
        // Constructor
        SceneTracker(CLWContext context, int devidx);
        // Destructor
        virtual ~SceneTracker();

        // Given a scene this method produces (or loads from cache) corresponding GPU representation.
        ClwScene& CompileScene(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector) const;
        

        // Get underlying intersection API.
        RadeonRays::IntersectionApi* GetIntersectionApi() { return  m_api; }

    protected:
        // Recompile the scene from scratch, i.e. not loading from cache.
        // All the buffers are recreated and reloaded.
        void RecompileFull(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const;
        // Clear intersector and load meshes into it.
        void ReloadIntersector(Scene1 const& scene, ClwScene& inout) const;

    private:
        // Update camera data only.
        void UpdateCamera(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const;
        // Update shape data only.
        void UpdateShapes(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const;
        // Update lights data only.
        void UpdateLights(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const;
        // Update material data.
        void UpdateMaterials(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const;
        // Update texture data only.
        void UpdateTextures(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const;
        // Write out single material at data pointer.
        // Collectors are required to convert texture and material pointers into indices.
        void WriteMaterial(Material const* material, Collector& mat_collector, Collector& tex_collector, void* data) const;
        // Write out single light at data pointer.
        // Collector is required to convert texture pointers into indices.
        void WriteLight(Scene1 const& scene, Light const* light, Collector& tex_collector, void* data) const;
        // Write out single texture header at data pointer.
        // Header requires texture data offset, so it is passed in.
        void WriteTexture(Texture const* texture, std::size_t data_offset, void* data) const;
        // Write out texture data at data pointer.
        void WriteTextureData(Texture const* texture, void* data) const;
		
		// JOSH
		void UpdateEnvironmentMap(Scene const& scene, ClwScene& out) const;

    private:
        // Context
        CLWContext m_context;
        // Intersection API
        RadeonRays::IntersectionApi* m_api;
        // Current scene
        mutable Scene1 const* m_current_scene;

        // Scene cache map (CPU scene -> GPU scene mapping)
        mutable std::map<Scene1 const*, ClwScene> m_scene_cache;
    };
}
