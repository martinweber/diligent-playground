#pragma once

#include <chrono>

// #define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <EngineFactory.h>
#include <RenderDevice.h>
#include <DeviceContext.h>
#include <SwapChain.h>
#include <RefCntAutoPtr.hpp>
#include <BasicMath.hpp>

class HelloDiligent
{
public:
    using Clock         = std::chrono::high_resolution_clock;
    using TimeUnitType  = std::chrono::microseconds;
    using TimeValueType = std::chrono::microseconds::rep;

public:
    void InitWindow();
    void InitDiligent();
    void Initialize();
    void CreatePipelineState();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    int  Run();
    int  MainLoop();
    void Update(TimeValueType current_time, TimeValueType delta_time);
    void Draw();

private:
    Diligent::float4x4 GetSurfacePretransformMatrix(const Diligent::float3& camera_view_axis) const;
    Diligent::float4x4 GetAdjustedProjectionMatrix(float fov, float near_plane, float far_plane) const;

public:
    GLFWwindow*               window_         = nullptr;
    Diligent::IEngineFactory* engine_factory_ = nullptr;
    Diligent::IRenderDevice*  device_         = nullptr;
    Diligent::IDeviceContext* device_context_ = nullptr;
    Diligent::ISwapChain*     swap_chain_     = nullptr;
    Clock::time_point         last_update_    = {};

    Diligent::RefCntAutoPtr<Diligent::IPipelineState>         pso_;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shader_resource_binding_;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>                vertex_shader_constants_;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>                cube_vertex_buffer_;
    Diligent::RefCntAutoPtr<Diligent::IBuffer>                cube_index_buffer_;
    Diligent::float4x4                                        world_view_projection_matrix_;
};
