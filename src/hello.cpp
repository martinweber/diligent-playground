#include "Hello.h"

#include <g3log/g3log.hpp>
#include "cgr_error.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <DebugOutput.h>
#include <SwapChain.h>
#if VULKAN_SUPPORTED
#include <EngineFactoryVk.h>
#endif
#include <MapHelper.hpp>

constexpr int kWindowWidth             = 800;
constexpr int kWindowHeight            = 600;
constexpr int kDiligentValidationLevel = -1;


static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = static_cast<HelloDiligent*>(glfwGetWindowUserPointer(window));
    if (app && app->swap_chain_)
        app->swap_chain_->Resize(static_cast<Diligent::Uint32>(width), static_cast<Diligent::Uint32>(height));
}


void HelloDiligent::InitWindow()
{
    if (glfwInit() != GLFW_TRUE)
        throw CGR_FAIL("Could not initialize GLFW!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, "Hello-Diligent", nullptr, nullptr);
    if (window_ == nullptr)
        throw CGR_FAIL("Could not create GLFW window!");

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, FramebufferResizeCallback);
}


void __cdecl MyDebugMessageCallback(enum Diligent::DEBUG_MESSAGE_SEVERITY severity,
                                    const Diligent::Char*                 message,
                                    const Diligent::Char*                 function,
                                    const Diligent::Char*                 file,
                                    int                                   line)
{
    if (message == nullptr)
        return;

    LEVELS g3log_severity = INFO;
    switch (severity)
    {
        case Diligent::DEBUG_MESSAGE_SEVERITY_INFO:
            g3log_severity = INFO;
            break;
        case Diligent::DEBUG_MESSAGE_SEVERITY_WARNING:
            g3log_severity = WARNING;
            break;
        case Diligent::DEBUG_MESSAGE_SEVERITY_ERROR:
            g3log_severity = WARNING;
            break;
        case Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR:
            g3log_severity = FATAL;
            break;
        default:
            g3log_severity = INFO;
    }
    const char* file_str = file ? file : "Diligent";
    const char* func_str = function ? function : "DebugMessageCallback";
    LogCapture(file_str, line, func_str, g3log_severity).stream() << message;
}


void HelloDiligent::InitDiligent()
{
    Diligent::Win32NativeWindow window{ glfwGetWin32Window(window_) };
    Diligent::SwapChainDesc     swap_chain_desc;

#if EXPLICITLY_LOAD_ENGINE_VK_DLL
    auto* GetEngineFactoryVk = Diligent::LoadGraphicsEngineVk();
    if (GetEngineFactoryVk == nullptr)
        throw CGR_FAIL("Could not load Diligent graphics engine!");
#endif
    Diligent::EngineVkCreateInfo engine_ci;

    if constexpr (kDiligentValidationLevel >= 0)
        engine_ci.SetValidationLevel(static_cast<Diligent::VALIDATION_LEVEL>(kDiligentValidationLevel));

    engine_ci.DynamicHeapSize = 256 << 20;

    auto* factory_vk = GetEngineFactoryVk();
    factory_vk->SetMessageCallback(MyDebugMessageCallback);
    factory_vk->CreateDeviceAndContextsVk(engine_ci, &device_, &device_context_);
    factory_vk->CreateSwapChainVk(device_, device_context_, swap_chain_desc, window, &swap_chain_);
    engine_factory_ = factory_vk;

    if (device_ == nullptr || device_context_ == nullptr || swap_chain_ == nullptr)
        throw CGR_FAIL("Could not initialize Diligent engine!");
}


void HelloDiligent::CreatePipelineState()
{
    Diligent::GraphicsPipelineStateCreateInfo pso_ci;

    pso_ci.PSODesc.Name         = "Cube PSO";
    pso_ci.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;

    pso_ci.GraphicsPipeline.NumRenderTargets             = 1;
    pso_ci.GraphicsPipeline.RTVFormats[0]                = swap_chain_->GetDesc().ColorBufferFormat;
    pso_ci.GraphicsPipeline.DSVFormat                    = swap_chain_->GetDesc().DepthBufferFormat;
    pso_ci.GraphicsPipeline.PrimitiveTopology            = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pso_ci.GraphicsPipeline.RasterizerDesc.CullMode      = Diligent::CULL_MODE_BACK;
    pso_ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;

    Diligent::ShaderCreateInfo shader_ci;

    shader_ci.SourceLanguage                  = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
    shader_ci.Desc.UseCombinedTextureSamplers = true; // required for OpenGL backend

    Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shader_source_factory;
    engine_factory_->CreateDefaultShaderSourceStreamFactory(nullptr, &shader_source_factory);
    shader_ci.pShaderSourceStreamFactory = shader_source_factory;

    Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
    {
        shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
        shader_ci.EntryPoint      = "main";
        shader_ci.Desc.Name       = "Cube VS";
        shader_ci.FilePath        = "shaders/cube.vsh";
        device_->CreateShader(shader_ci, &vertex_shader);

        if (vertex_shader == nullptr)
            throw CGR_FAIL("Could not create vertex shader!");

        Diligent::BufferDesc cb_desc;
        cb_desc.Name           = "VS constants CB";
        cb_desc.Size           = sizeof(Diligent::float4x4);
        cb_desc.Usage          = Diligent::USAGE_DYNAMIC;
        cb_desc.BindFlags      = Diligent::BIND_UNIFORM_BUFFER;
        cb_desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        device_->CreateBuffer(cb_desc, nullptr, &vertex_shader_constants_);

        if (vertex_shader_constants_ == nullptr)
            throw CGR_FAIL("Could not create vertex shader constant buffer!");
    }

    Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader;
    {
        shader_ci.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
        shader_ci.EntryPoint      = "main";
        shader_ci.Desc.Name       = "Cube PS";
        shader_ci.FilePath        = "shaders/cube.psh";
        device_->CreateShader(shader_ci, &pixel_shader);

        if (pixel_shader == nullptr)
            throw CGR_FAIL("Could not create pixel shader!");
    }

    Diligent::LayoutElement layout_elements[] = { Diligent::LayoutElement{ 0, 0, 3, Diligent::VT_FLOAT32, false },
                                                  Diligent::LayoutElement{ 1, 0, 4, Diligent::VT_FLOAT32, false } };

    pso_ci.GraphicsPipeline.InputLayout.LayoutElements = layout_elements;
    pso_ci.GraphicsPipeline.InputLayout.NumElements    = _countof(layout_elements);

    pso_ci.pVS = vertex_shader;
    pso_ci.pPS = pixel_shader;

    pso_ci.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    device_->CreateGraphicsPipelineState(pso_ci, &pso_);
    if (pso_ == nullptr)
        throw CGR_FAIL("Could not create pipeline state object!");

    pso_->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants")->Set(vertex_shader_constants_);
    pso_->CreateShaderResourceBinding(&shader_resource_binding_, true);
    if (shader_resource_binding_ == nullptr)
        throw CGR_FAIL("Could not create shader resource binding!");
}


void HelloDiligent::CreateVertexBuffer()
{
    using float3 = Diligent::float3;
    using float4 = Diligent::float4;

    struct Vertex
    {
        float3 pos;
        float4 color;
    };

    // clang-format off
    Vertex cube_vertices[8] = {
        { float3(-1, -1, -1), float4(1, 0, 0, 1) },
        { float3(-1, +1, -1), float4(0, 1, 0, 1) },
        { float3(+1, +1, -1), float4(0, 0, 1, 1) },
        { float3(+1, -1, -1), float4(1, 1, 1, 1) },
        { float3(-1, -1, +1), float4(1, 1, 0, 1) },
        { float3(-1, +1, +1), float4(0, 1, 1, 1) },
        { float3(+1, +1, +1), float4(1, 0, 1, 1) },
        { float3(+1, -1, +1), float4(0.2f, 0.2f, 0.2f, 1) },
    };
    // clang-format on

    Diligent::BufferDesc vertex_buffer_desc;
    vertex_buffer_desc.Name      = "Cube vertex buffer";
    vertex_buffer_desc.Usage     = Diligent::USAGE_IMMUTABLE;
    vertex_buffer_desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
    vertex_buffer_desc.Size      = sizeof(cube_vertices);

    Diligent::BufferData vertex_buffer_data;
    vertex_buffer_data.pData    = cube_vertices;
    vertex_buffer_data.DataSize = sizeof(cube_vertices);

    device_->CreateBuffer(vertex_buffer_desc, &vertex_buffer_data, &cube_vertex_buffer_);
    if (cube_vertex_buffer_ == nullptr)
        throw CGR_FAIL("Could not create cube vertex buffer!");
}


void HelloDiligent::CreateIndexBuffer()
{
    // clang-format off
    Diligent::Uint32 indices[] = {
        2,0,1, 2,3,0,
        4,6,5, 4,7,6,
        0,7,4, 0,3,7,
        1,0,4, 1,4,5,
        1,5,2, 5,6,2,
        3,6,7, 3,2,6
    };
    // clang-format on

    Diligent::BufferDesc index_buffer_desc;
    index_buffer_desc.Name      = "Cube index buffer";
    index_buffer_desc.Usage     = Diligent::USAGE_IMMUTABLE;
    index_buffer_desc.BindFlags = Diligent::BIND_INDEX_BUFFER;
    index_buffer_desc.Size      = sizeof(indices);

    Diligent::BufferData index_buffer_data;
    index_buffer_data.pData    = indices;
    index_buffer_data.DataSize = sizeof(indices);

    device_->CreateBuffer(index_buffer_desc, &index_buffer_data, &cube_index_buffer_);
    if (cube_index_buffer_ == nullptr)
        throw CGR_FAIL("Could not create cube index buffer!");
}


Diligent::float4x4 HelloDiligent::GetSurfacePretransformMatrix(const Diligent::float3& camera_view_axis) const
{
    const auto& swap_chain_desc = swap_chain_->GetDesc();
    switch (swap_chain_desc.PreTransform)
    {
        case Diligent::SURFACE_TRANSFORM_ROTATE_90:
            // The image content is rotated 90 degrees clockwise.
            return Diligent::float4x4::RotationArbitrary(camera_view_axis, -Diligent::PI_F / 2.f);

        case Diligent::SURFACE_TRANSFORM_ROTATE_180:
            // The image content is rotated 180 degrees clockwise.
            return Diligent::float4x4::RotationArbitrary(camera_view_axis, -Diligent::PI_F);

        case Diligent::SURFACE_TRANSFORM_ROTATE_270:
            // The image content is rotated 270 degrees clockwise.
            return Diligent::float4x4::RotationArbitrary(camera_view_axis, -Diligent::PI_F * 3.f / 2.f);

        case Diligent::SURFACE_TRANSFORM_OPTIMAL:
            UNEXPECTED("SURFACE_TRANSFORM_OPTIMAL is only valid as parameter during swap chain initialization.");
            return Diligent::float4x4::Identity();

        case Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR:
        case Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90:
        case Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180:
        case Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270:
            UNEXPECTED("Mirror transforms are not supported");
            return Diligent::float4x4::Identity();

        default:
            return Diligent::float4x4::Identity();
    }
}


Diligent::float4x4 HelloDiligent::GetAdjustedProjectionMatrix(float fov, float near_plane, float far_plane) const
{
    const auto& swap_chain_desc = swap_chain_->GetDesc();

    float aspect_ratio = static_cast<float>(swap_chain_desc.Width) / static_cast<float>(swap_chain_desc.Height);
    float x_scale, y_scale;
    if (swap_chain_desc.PreTransform == Diligent::SURFACE_TRANSFORM_ROTATE_90 ||
        swap_chain_desc.PreTransform == Diligent::SURFACE_TRANSFORM_ROTATE_270 ||
        swap_chain_desc.PreTransform == Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90 ||
        swap_chain_desc.PreTransform == Diligent::SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270)
    {
        // When the screen is rotated, vertical FOV becomes horizontal FOV
        x_scale = 1.f / std::tan(fov / 2.f);
        // Aspect ratio is inversed
        y_scale = x_scale * aspect_ratio;
    }
    else
    {
        y_scale = 1.f / std::tan(fov / 2.f);
        x_scale = y_scale / aspect_ratio;
    }

    Diligent::float4x4 projection;
    projection._11 = x_scale;
    projection._22 = y_scale;
    projection.SetNearFarClipPlanes(near_plane, far_plane, device_->GetDeviceInfo().IsGLDevice());
    return projection;
}


void HelloDiligent::Update(const TimeValueType current_time, const TimeValueType delta_time)
{
    using float4x4 = Diligent::float4x4;

    float4x4 cube_model_transform = float4x4::RotationY(static_cast<float>(current_time) / static_cast<float>(std::micro::den)) *
                                    float4x4::RotationX(-Diligent::PI_F * 0.1f);

    float4x4 view = float4x4::Translation(0.f, 0.f, 5.f);

    auto surface_pre_transform = GetSurfacePretransformMatrix(Diligent::float3{ 0, 0, 1 });
    auto projection            = GetAdjustedProjectionMatrix(Diligent::PI_F / 4.0f, 01.f, 100.f);

    world_view_projection_matrix_ = cube_model_transform * view * surface_pre_transform * projection;
}


void HelloDiligent::Draw()
{
    auto*       render_target_view = swap_chain_->GetCurrentBackBufferRTV();
    auto*       depth_stencil_view = swap_chain_->GetDepthBufferDSV();

    device_context_->SetRenderTargets(1, &render_target_view, depth_stencil_view,
                                      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Clear the back buffer
    const float clear_color[]      = { 0.350f, 0.350f, 0.350f, 1.0f };
    device_context_->ClearRenderTarget(render_target_view, clear_color,
                                       Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    device_context_->ClearDepthStencil(depth_stencil_view, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0,
                                       Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        // Map the buffer and write current world-view-projection matrix
        Diligent::MapHelper<Diligent::float4x4> cb_constants(device_context_, vertex_shader_constants_,
                                                            Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
        *cb_constants = world_view_projection_matrix_.Transpose();
    }

    // Bind vertex and index buffers
    const Diligent::Uint64 offset   = 0;
    Diligent::IBuffer*     buffers[] = { cube_vertex_buffer_ };
    device_context_->SetVertexBuffers(0, 1, buffers, &offset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                      Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
    device_context_->SetIndexBuffer(cube_index_buffer_, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state
    device_context_->SetPipelineState(pso_);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    device_context_->CommitShaderResources(shader_resource_binding_,
                                           Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    Diligent::DrawIndexedAttribs draw_attributes;     // This is an indexed draw call
    draw_attributes.IndexType  = Diligent::VT_UINT32; // Index type
    draw_attributes.NumIndices = 36;
    // Verify the state of vertex and index buffers
    draw_attributes.Flags      = Diligent::DRAW_FLAG_VERIFY_ALL;
    device_context_->DrawIndexed(draw_attributes);

    swap_chain_->Present();
}


int HelloDiligent::MainLoop()
{
    last_update_ = Clock::now();
    while (true)
    {
        if (glfwWindowShouldClose(window_))
            break;

        glfwPollEvents();

        const auto time       = Clock::now();
        const auto delta_time = std::chrono::duration_cast<TimeUnitType>(time - last_update_).count();
        last_update_          = time;

        auto time_usec = std::chrono::duration_cast<TimeUnitType>(time.time_since_epoch()).count();
        Update(time_usec, delta_time);

        int width, height;
        glfwGetWindowSize(window_, &width, &height);

        if (width > 0 && height > 0)
            Draw();
    }

    return 0;
}


void HelloDiligent::Initialize()
{
    CreatePipelineState();
    CreateVertexBuffer();
    CreateIndexBuffer();
}


int HelloDiligent::Run()
{
    InitWindow();
    InitDiligent();
    Initialize();
    return MainLoop();
}
