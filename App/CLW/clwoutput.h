#pragma once

#include "Core/output.h"
#include "CLW.h"

namespace Baikal
{
    class ClwOutput : public Output
    {
    public:
        ClwOutput(std::uint32_t w, std::uint32_t h, CLWContext context)
        : Output(w, h)
        , m_context(context)
        , m_data(context.CreateBuffer<RadeonRays::float3>(w*h, CL_MEM_READ_WRITE))
        , m_depth_data(context.CreateBuffer<float>(w*h, CL_MEM_READ_WRITE))
        , m_normals_data(context.CreateBuffer<RadeonRays::float3>(w*h, CL_MEM_READ_WRITE))
        {
        }

        void GetData(RadeonRays::float3* data) const
        {
            m_context.ReadBuffer(0, m_data, data, m_data.GetElementCount()).Wait();
        }
        
        void GetDepthData(float* data) const
        {
            m_context.ReadBuffer(0, m_depth_data, data, m_depth_data.GetElementCount()).Wait();
        }
        
        void GetNormalsData(RadeonRays::float3* data) const
        {
            m_context.ReadBuffer(0, m_normals_data, data, m_normals_data.GetElementCount()).Wait();
        }
        
        void Clear(RadeonRays::float3 const& val)
        {
            m_context.FillBuffer(0, m_data, val, m_data.GetElementCount()).Wait();
            m_context.FillBuffer(0, m_depth_data, 0.0f, m_depth_data.GetElementCount()).Wait();
            m_context.FillBuffer(0, m_normals_data, val, m_normals_data.GetElementCount()).Wait();
            clear_depth_count();
        }

        CLWBuffer<RadeonRays::float3> data() const { return m_data; }
        CLWBuffer<float> depth_data() const { return m_depth_data; }
        
        void increment_depth_count() {
            depth_count++;
        }
        
        void clear_depth_count() {
            depth_count = 0;
        }
        
        int get_depth_count() const {
            return depth_count;
        }
        
        CLWBuffer<RadeonRays::float3> normals_data() const { return m_normals_data; }

    private:
        CLWBuffer<RadeonRays::float3> m_data;
        // JOSH
        CLWBuffer<float> m_depth_data;
        int depth_count = 0;
        CLWBuffer<RadeonRays::float3> m_normals_data;
        
        CLWContext m_context;
    };
}
