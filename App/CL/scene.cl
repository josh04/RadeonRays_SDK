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
#ifndef SCENE_CL
#define SCENE_CL

#include "../App/CL/utils.cl"
#include "../App/CL/payload.cl"

typedef struct _Scene
    {
        // Vertices
        __global float3 const* vertices;
        // Normals
        __global float3 const* normals;
        // UVs
        __global float2 const* uvs;
        // Indices
        __global int const* indices;
        // Shapes
        __global Shape const* shapes;
        // Material IDs
        __global int const* materialids;
        // Materials
        __global Material const* materials;
        // Emissive objects
        __global Light const* lights;
        // Envmap idx
        int envmapidx;
        // Envmap multiplier
        float envmapmul;
        // Number of emissive objects
        int num_lights;
    } Scene;

/// Fill DifferentialGeometry structure based on intersection info from RadeonRays
void DifferentialGeometry_Fill(// Scene
                              Scene const* scene,
                              // RadeonRays intersection
                              Intersection const* isect,
                              // Differential geometry
                              DifferentialGeometry* diffgeo
                              )
{
    // Determine shape and polygon
    int shapeid = isect->shapeid - 1;
    int primid = isect->primid;

    // Get barycentrics
    float2 uv = isect->uvwt.xy;

    // Extract shape data
    Shape shape = scene->shapes[shapeid];
    //float3 linearvelocity = shape.linearvelocity;
    //float4 angularvelocity = shape.angularvelocity;

    // Fetch indices starting from startidx and offset by primid
    int i0 = scene->indices[shape.startidx + 3 * primid];
    int i1 = scene->indices[shape.startidx + 3 * primid + 1];
    int i2 = scene->indices[shape.startidx + 3 * primid + 2];

    // Fetch normals
    float3 n0 = scene->normals[shape.startvtx + i0];
    float3 n1 = scene->normals[shape.startvtx + i1];
    float3 n2 = scene->normals[shape.startvtx + i2];

    // Fetch positions
    float3 v0 = scene->vertices[shape.startvtx + i0];
    float3 v1 = scene->vertices[shape.startvtx + i1];
    float3 v2 = scene->vertices[shape.startvtx + i2];

    // Fetch UVs
    float2 uv0 = scene->uvs[shape.startvtx + i0];
    float2 uv1 = scene->uvs[shape.startvtx + i1];
    float2 uv2 = scene->uvs[shape.startvtx + i2];

    // Calculate barycentric position and normal
    diffgeo->n = normalize(transform_vector((1.f - uv.x - uv.y) * n0 + uv.x * n1 + uv.y * n2, shape.m0, shape.m1, shape.m2, shape.m3));
    diffgeo->p = transform_point((1.f - uv.x - uv.y) * v0 + uv.x * v1 + uv.y * v2, shape.m0, shape.m1, shape.m2, shape.m3);
    diffgeo->uv = (1.f - uv.x - uv.y) * uv0 + uv.x * uv1 + uv.y * uv2;

    diffgeo->ng = normalize(cross(v1 - v0, v2 - v0));

    if (dot(diffgeo->ng, diffgeo->n) < 0.f)
        diffgeo->ng = -diffgeo->ng;

    // Get material at shading point
    int matidx = scene->materialids[shape.startidx / 3 + primid];
    diffgeo->mat = scene->materials[matidx];

    /// From PBRT book
    float du1 = uv0.x - uv2.x;
    float du2 = uv1.x - uv2.x;
    float dv1 = uv0.y - uv2.y;
    float dv2 = uv1.y - uv2.y;

    float3 dp1 = v0 - v2;
    float3 dp2 = v1 - v2;

    float det = du1 * dv2 - dv1 * du2;

    if (0 && det != 0.f)
    {
        float invdet = 1.f / det;
        diffgeo->dpdu = normalize( (dv2 * dp1 - dv1 * dp2) * invdet );
        diffgeo->dpdv = normalize( (-du2 * dp1 + du1 * dp2) * invdet );
    }
    else
    {
        diffgeo->dpdu = normalize(GetOrthoVector(diffgeo->n));
        diffgeo->dpdv = normalize(cross(diffgeo->n, diffgeo->dpdu));
    }



    // Fix all to be orthogonal
    //diffgeo->dpdv = normalize(cross(diffgeo->ng, diffgeo->dpdu));
    //diffgeo->dpdu = normalize(cross(diffgeo->dpdv, diffgeo->ng));

    float3 p0 = transform_point(v0, shape.m0, shape.m1, shape.m2, shape.m3);
    float3 p1 = transform_point(v1, shape.m0, shape.m1, shape.m2, shape.m3);
    float3 p2 = transform_point(v2, shape.m0, shape.m1, shape.m2, shape.m3);

    diffgeo->area = 0.5f * length(cross(p2 - p0, p2 - p1));
}

void DifferentialGeometry_CalculateTangentTransforms(DifferentialGeometry* diffgeo)
{
    diffgeo->world_to_tangent = matrix_from_rows3(
        diffgeo->dpdu,
        diffgeo->n,
        diffgeo->dpdv);

    diffgeo->world_to_tangent.rows.m0.w = -dot(diffgeo->dpdu, diffgeo->p);
    diffgeo->world_to_tangent.rows.m1.w = -dot(diffgeo->n, diffgeo->p);
    diffgeo->world_to_tangent.rows.m2.w = -dot(diffgeo->dpdv, diffgeo->p);

    diffgeo->tangent_to_world = matrix_from_cols3(
        diffgeo->world_to_tangent.rows.m0.xyz,
        diffgeo->world_to_tangent.rows.m1.xyz,
        diffgeo->world_to_tangent.rows.m2.xyz);

    diffgeo->tangent_to_world.rows.m0.w = diffgeo->p.x;
    diffgeo->tangent_to_world.rows.m1.w = diffgeo->p.y;
    diffgeo->tangent_to_world.rows.m2.w = diffgeo->p.z;
}

int Scene_SampleLight(Scene const* scene, float sample, float* pdf)
{
    int num_lights = scene->num_lights;
    int light_idx = clamp((int)(sample * num_lights), 0, num_lights - 1);
    *pdf = 1.f / num_lights;
    return light_idx;
}


#endif
