// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */
#include "pch.h"
#include "engine/system/interface/interface_scene.h"
#include "engine/utils/logger.h"
#include "engine/render_manager/assets_library/model/model.h"
#include "engine/render_manager/assets_library/shader_library.h"

#include "imgui/imgui.h"
#include <DirectXMath.h>

using namespace DirectX;

void kfe::IKFESceneObject::SetVisible(bool visible)
{
    m_sceneInfo.Visible = visible;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::IsVisible() const
{
    return  m_sceneInfo.Visible;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::IsInitialized() const noexcept
{
    return m_sceneInfo.Initialized;
}

void kfe::IKFESceneObject::Update(const KFE_UPDATE_OBJECT_DESC& desc)
{
    if (!m_sceneInfo.Initialized) return;

    if (m_sceneInfo.PipelineDirty) 
    {
        if (!InitMainPipeline(m_pDevice)) 
        {
            m_sceneInfo.Initialized = false;
            LOG_INFO("Failed to rebuild Main Pipeline");
            return;
        }
    }
    m_lightManager.PackData();
   
    UpdatePrimaryConstantBuffer(desc);
    ChildUpdate(desc);
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::Build(_In_ const KFE_BUILD_OBJECT_DESC& desc)
{
    if (!desc.Device)
    {
        LOG_ERROR("Device is null.");
        return false;
    }

    if (!desc.ResourceHeap)
    {
        LOG_ERROR("ResourceHeap is null.");
        return false;
    }

    m_pDevice = desc.Device;
    m_mainPassInfo.ResourceHeap = desc.ResourceHeap;
    m_mainPassInfo.SamplerHeap  = desc.SamplerHeap;

    m_shadowPassInfo.ResourceHeap = desc.ResourceHeap;
    m_shadowPassInfo.SamplerHeap  = desc.SamplerHeap;

    //~ Main Pass
    if (!InitMainRootSignature(desc))
    {
        LOG_ERROR("Failed to build Main Root Signature!");
        return false;
    }

    if (!InitMainPipeline(desc.Device))
    {
        LOG_ERROR("Failed to build Main Pipeline!");
        return false;
    }

    if (!InitPrimaryConstantBuffer(desc))
    {
        LOG_ERROR("Failed to build Primary Constant Buffer!");
        return false;
    }

    if (!InitMainSampler(desc))
    {
        LOG_ERROR("Failed to build Main Sampler!");
        return false;
    }

    ////~ Shadow Pass
    //if (!InitShadowRootSignature(desc))
    //{
    //    LOG_ERROR("Failed to build Shadow Root Signature!");
    //    return false;
    //}

    //if (!InitShadowPipeline(desc.Device))
    //{
    //    LOG_ERROR("Failed to build Shadow Pipeline!");
    //    return false;
    //}

    //if (!InitShadowLightMetaConstantbuffer(desc))
    //{
    //    LOG_ERROR("Failed to build Shadow Light Meta Constant Buffer!");
    //    return false;
    //}

    //if (!InitShadowComparisionSampler(desc))
    //{
    //    LOG_ERROR("Failed to build Shadow Comparison Sampler!");
    //    return false;
    //}

    //~ Light Manager
    KFE_CREATE_LIGHT_MANAGER manager{};
    manager.Capacity    = 128u;
    manager.DebugName   = "LightManager_SceneObject";
    manager.Device      = desc.Device;
    manager.Heap        = desc.ResourceHeap;

    if (!m_lightManager.Initialize(manager))
    {
        LOG_ERROR("Failed to build light manager!");
        return false;
    }

    if (!ChildBuild(desc))
    {
        LOG_ERROR("Failed to build Child!");
        return false;
    }

    m_sceneInfo.Initialized = true;
    LOG_SUCCESS("Build Scene Object Successfully!");
    return true;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::Destroy()
{
    m_shadowPassInfo.FreeSamplerHeap();
    m_shadowPassInfo.FreeSample();
    m_shadowPassInfo.FreeSignature();
    m_mainPassInfo  .FreeSamplerHeap();
    m_mainPassInfo  .FreeSample();
    m_mainPassInfo  .FreeSignature();
    m_sceneInfo     .Reset();

    if (!ChildDestroy()) 
    {
        LOG_ERROR("Failed to release Scene object resource safely!");
        return false;
    }
    return true;
}

//~ Passes
void kfe::IKFESceneObject::MainPass(_In_ const KFE_RENDER_OBJECT_DESC& desc)
{
    if (!desc.CommandList) return;

    if (!m_mainPassInfo.Pipeline || !m_mainPassInfo.Pipeline->GetNative())
    {
        LOG_ERROR("Main Pass Called Even tho pipeline is null!");
        return;
    }

    m_primaryCBFrame.Step();
    desc.CommandList->SetPipelineState(m_mainPassInfo.Pipeline->GetNative());

    if (!m_mainPassInfo.RootSignature || !m_mainPassInfo.RootSignature->GetNative())
    {
        LOG_ERROR("Main Pass Called Even tho root signature is null!");
        return;
    }

    auto* rg = static_cast<ID3D12RootSignature*>(m_mainPassInfo.RootSignature->GetNative());
    desc.CommandList->SetGraphicsRootSignature(rg);

    if (!m_mainPassInfo.IsValidHeap())
    {
        LOG_ERROR("Attached Heaps arent valid!");
        return;
    }

    ID3D12DescriptorHeap* heaps[]
    {
        m_mainPassInfo.ResourceHeap->GetNative(),
    };
    desc.CommandList->SetDescriptorHeaps(1u, heaps);

    //~ Bind Primary Buffer b0
    const D3D12_GPU_VIRTUAL_ADDRESS address = m_primaryCBFrame.GetView()->GetGPUVirtualAddress();
    desc.CommandList->SetGraphicsRootConstantBufferView(0u, address);

    //~ Bind Light Data t15
    (void)m_lightManager.UpdateAndRecord(desc.CommandList);
    m_lightManager.SetDrawState(desc.CommandList, D3D12_RESOURCE_STATE_COPY_DEST);
    const std::uint32_t lightAddr = m_lightManager.GetSRVDescriptorIndex();
    const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_mainPassInfo.ResourceHeap->GetGPUHandle(lightAddr);
    desc.CommandList->SetGraphicsRootDescriptorTable(3u, srvHandle);

    //~ Primitive topology
    switch (Draw.DrawMode)
    {
    case EDrawMode::Triangle:
    case EDrawMode::WireFrame:
        desc.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        break;
    case EDrawMode::Point:
        desc.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
        break;
    }

    ChildMainPass(desc);
}

void kfe::IKFESceneObject::ShadowPass(_In_ const KFE_RENDER_OBJECT_DESC& desc)
{
    if (!m_sceneInfo.Initialized) return;
}

JsonLoader kfe::IKFESceneObject::GetJsonData() const
{
    JsonLoader root;
    std::string id        = std::to_string(GetAssignedKey());
    root["Type"]          = m_sceneInfo.SceneType;
    root["Name"]          = m_sceneInfo.SceneName;
    root["Visible"]       = m_sceneInfo.Visible;
    root["ShaderInfo"]    = m_shaderInfo.GetJsonData();
    root["DrawInfo"]      = Draw.GetJsonData();
    root["TransformInfo"] = Transform.GetJsonData();
    root["MeshInfo"]      = ChildGetJsonData();
    return root;
}

void kfe::IKFESceneObject::LoadFromJson(const JsonLoader& loader)
{
    //~ Scene info
    if (loader.Has("Type"))
        m_sceneInfo.SceneType = loader["Type"].GetValue();

    if (loader.Has("Name"))
        m_sceneInfo.SceneName = loader["Name"].GetValue();

    if (loader.Has("Visible"))
        m_sceneInfo.Visible = loader["Visible"].AsBool();

    //~ Shader / draw / transform
    if (loader.Has("ShaderInfo"))
        m_shaderInfo.LoadFromJson(loader["ShaderInfo"]);

    if (loader.Has("DrawInfo"))
        Draw.LoadFromJson(loader["DrawInfo"]);

    if (loader.Has("TransformInfo"))
        Transform.LoadFromJson(loader["TransformInfo"]);

    //~ Mesh
    if (loader.Has("MeshInfo"))
        ChildLoadFromJson(loader["MeshInfo"]);

    m_sceneInfo.PipelineDirty = true;
}

void kfe::IKFESceneObject::ImguiView(float deltaTime)
{
    //~ Quick row
    {
        bool visible = m_sceneInfo.Visible;
        if (ImGui::Checkbox("Visible", &visible))
            m_sceneInfo.Visible = visible;

        ImGui::SameLine();

        ImGui::TextDisabled("Key: %llu",
            static_cast<unsigned long long>(GetAssignedKey()));

        ImGui::SameLine();

        ImGui::TextDisabled("Type: %s", GetTypeName().c_str());

        ImGui::Separator();
    }

    //~ Header section
    if (ImGui::CollapsingHeader("Header", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        m_sceneInfo.ImguiView(deltaTime);
        ChildImguiViewHeader(deltaTime);
        ImGui::Unindent();
        ImGui::Spacing();
    }

    //~ Body tabs
    if (ImGui::BeginTabBar("##scene_object_tabs", ImGuiTabBarFlags_None))
    {
        //~ Shader
        if (ImGui::BeginTabItem("Shader"))
        {
            ImGui::Spacing();
            m_shaderInfo.ImguiView(deltaTime, m_sceneInfo);
            ImGui::EndTabItem();
        }

        //~ Draw
        if (ImGui::BeginTabItem("Draw"))
        {
            ImGui::Spacing();
            Draw.ImguiView(deltaTime, m_sceneInfo);
            ImGui::EndTabItem();
        }

        //~ Transform
        if (ImGui::BeginTabItem("Transform"))
        {
            ImGui::Spacing();
            Transform.ImguiView(deltaTime);
            ImGui::EndTabItem();
        }

        //~ Mesh
        if (ImGui::BeginTabItem("Mesh"))
        {
            ImGui::Spacing();
            ChildImguiViewBody(deltaTime);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Spacing();
    ImGui::Separator();
}

_Use_decl_annotations_
const XMMATRIX& kfe::IKFESceneObject::GetWorldMatrix() const
{
    return Transform.GetMatrix();
}

void kfe::IKFESceneObject::SetTypeName(const std::string& typeName)
{
    m_sceneInfo.SceneType = typeName;
}

void kfe::IKFESceneObject::SetObjectName(const std::string& typeName)
{
    m_sceneInfo.SceneName = typeName;
}

std::string kfe::IKFESceneObject::GetTypeName() const
{
    return m_sceneInfo.SceneType;
}

std::string kfe::IKFESceneObject::GetObjectName() const
{
    return m_sceneInfo.SceneName;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::InitMainRootSignature(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (m_mainPassInfo.RootSignature) return true;

    m_mainPassInfo.RootSignature = std::make_unique<KFERootSignature>();
    
    //~ SRV descriptor table: t0..tN-1
    D3D12_DESCRIPTOR_RANGE ranges[1]{};
    ranges[0].RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors     = static_cast<UINT>(EModelTextureSlot::Count);
    ranges[0].BaseShaderRegister = 0u; // t0
    ranges[0].RegisterSpace      = 0u;
    ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    //~ SRV descriptor table: lights t15 (StructuredBuffer of lights)
    D3D12_DESCRIPTOR_RANGE rangesLights[1]{};
    rangesLights[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangesLights[0].NumDescriptors                    = 1u;
    rangesLights[0].BaseShaderRegister                = 15u; // t15
    rangesLights[0].RegisterSpace                     = 0u;
    rangesLights[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[4]{};

    //~ b0: common (for VS and PS)
    params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    params[0].Descriptor.ShaderRegister = 0u; // b0
    params[0].Descriptor.RegisterSpace  = 0u;

    //~ SRV table: textures
    params[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
    params[1].DescriptorTable.NumDescriptorRanges = 1u;
    params[1].DescriptorTable.pDescriptorRanges   = ranges;

    //~ b1: texture meta 
    params[2].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[2].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;
    params[2].Descriptor.ShaderRegister = 1u; // b1
    params[2].Descriptor.RegisterSpace  = 0u;

    //~ SRV table: lights (t15)
    params[3].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[3].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
    params[3].DescriptorTable.NumDescriptorRanges = 1u;
    params[3].DescriptorTable.pDescriptorRanges   = rangesLights;

    //~ static sampler: s0
    D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};

    //~ s0: regular sampler
    staticSamplers[0].Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].MipLODBias       = 0.0f;
    staticSamplers[0].MaxAnisotropy    = 1;
    staticSamplers[0].ComparisonFunc   = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSamplers[0].BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSamplers[0].MinLOD           = 0.0f;
    staticSamplers[0].MaxLOD           = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister   = 0u; //~ s0
    staticSamplers[0].RegisterSpace    = 0u;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    //~ s1: shadow comparison sampler
    staticSamplers[1].Filter            = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    staticSamplers[1].AddressU          = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressV          = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressW          = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].MipLODBias        = 0.0f;
    staticSamplers[1].MaxAnisotropy     = 1;
    staticSamplers[1].ComparisonFunc    = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    staticSamplers[1].BorderColor       = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSamplers[1].MinLOD            = 0.0f;
    staticSamplers[1].MaxLOD            = D3D12_FLOAT32_MAX;
    staticSamplers[1].ShaderRegister    = 1u; //~ s1
    staticSamplers[1].RegisterSpace     = 0u;
    staticSamplers[1].ShaderVisibility  = D3D12_SHADER_VISIBILITY_PIXEL;

    KFE_RG_CREATE_DESC root{};
    root.Device             = desc.Device;
    root.Flags              = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    root.NumRootParameters  = static_cast<UINT>(_countof(params));
    root.RootParameters     = params;
    root.NumStaticSamplers  = 2u;
    root.StaticSamplers     = staticSamplers;

    if (!m_mainPassInfo.RootSignature->Initialize(root))
    {
        LOG_ERROR("Failed to Create Root Signature!");
        return false;
    }

    m_mainPassInfo.RootSignature->SetDebugName(L"KFE Scene Signature");
    LOG_SUCCESS("Scene Root Signature Created!");
    return true;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::InitMainPipeline(KFEDevice* device)
{
    //~ Validate device
    if (!device || !device->GetNative())
    {
        LOG_ERROR("InitMainPipeline: Device or native device is null.");
        return false;
    }

    //~ Skip rebuild if already valid
    if (m_mainPassInfo.Pipeline && !m_sceneInfo.PipelineDirty)
    {
        LOG_SUCCESS("Main pipeline already built.");
        return true;
    }

    //~ Root signature must exist
    if (!m_mainPassInfo.RootSignature ||
        !m_mainPassInfo.RootSignature->GetNative())
    {
        LOG_ERROR("InitMainPipeline: Main root signature not initialized.");
        return false;
    }

    if (m_shaderInfo.VertexShader.empty())
    {
        m_shaderInfo.VertexShader = "shaders/model/vertex_shader.hlsl";
    }

    if (m_shaderInfo.PixelShader.empty())
    {
        m_shaderInfo.PixelShader = "shaders/model/pixel_shader.hlsl";
    }

    //~ Validate shader paths
    if (!kfe_helpers::IsFile(m_shaderInfo.VertexShader))
    {
        LOG_ERROR("InitMainPipeline: VS not found: {}", m_shaderInfo.VertexShader);
        return false;
    }

    if (!kfe_helpers::IsFile(m_shaderInfo.PixelShader))
    {
        LOG_ERROR("InitMainPipeline: PS not found: {}", m_shaderInfo.PixelShader);
        return false;
    }

    //~ Compile or load shaders
    ID3DBlob* vsBlob = shaders::GetOrCompile(m_shaderInfo.VertexShader, "main", "vs_5_0");
    if (!vsBlob)
    {
        LOG_ERROR("InitMainPipeline: Failed to compile VS: {}", m_shaderInfo.VertexShader);
        return false;
    }

    ID3DBlob* psBlob = shaders::GetOrCompile(m_shaderInfo.PixelShader, "main", "ps_5_0");
    if (!psBlob)
    {
        LOG_ERROR("InitMainPipeline: Failed to compile PS: {}", m_shaderInfo.PixelShader);
        return false;
    }

    //~ Recreate pipeline container
    if (m_mainPassInfo.Pipeline)
    {
        m_mainPassInfo.Pipeline->Destroy();
    }
    else
    {
        m_mainPassInfo.Pipeline = std::make_unique<KFEPipelineState>();
    }

    //~ Input layout
    const auto layout = KFEMeshGeometry::GetInputLayout();
    if (layout.empty())
    {
        LOG_ERROR("InitMainPipeline: Input layout is empty.");
        return false;
    }

    m_mainPassInfo.Pipeline->SetInputLayout(
        layout.data(),
        static_cast<UINT>(layout.size()));

    //~ Shaders
    D3D12_SHADER_BYTECODE vs{};
    vs.pShaderBytecode = vsBlob->GetBufferPointer();
    vs.BytecodeLength = vsBlob->GetBufferSize();

    D3D12_SHADER_BYTECODE ps{};
    ps.pShaderBytecode = psBlob->GetBufferPointer();
    ps.BytecodeLength = psBlob->GetBufferSize();

    m_mainPassInfo.Pipeline->SetVS(vs);
    m_mainPassInfo.Pipeline->SetPS(ps);

    //~ Root signature
    m_mainPassInfo.Pipeline->SetRootSignature(
        static_cast<ID3D12RootSignature*>(
            m_mainPassInfo.RootSignature->GetNative()));

    //~ Rasterizer
    D3D12_RASTERIZER_DESC raster{};
    raster.FillMode = D3D12_FILL_MODE_SOLID;

    switch (Draw.CullMode)
    {
    case ECullMode::None:
        raster.CullMode = D3D12_CULL_MODE_NONE;
        break;

    case ECullMode::Front:
        raster.CullMode = D3D12_CULL_MODE_FRONT;
        break;

    case ECullMode::Back:
        raster.CullMode = D3D12_CULL_MODE_BACK;
        break;

    default:
        raster.CullMode = D3D12_CULL_MODE_BACK;
        break;
    }

    raster.FrontCounterClockwise = FALSE;
    raster.DepthClipEnable = TRUE;
    raster.MultisampleEnable = FALSE;
    raster.AntialiasedLineEnable = FALSE;
    raster.ForcedSampleCount = 0u;
    raster.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    raster.DepthBias = 0;
    raster.DepthBiasClamp = 0.0f;
    raster.SlopeScaledDepthBias = 0.0f;

    m_mainPassInfo.Pipeline->SetRasterizer(raster);

    //~ Depth test
    D3D12_DEPTH_STENCIL_DESC ds{};
    ds.DepthEnable      = TRUE;
    ds.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc        = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ds.StencilEnable    = FALSE;

    m_mainPassInfo.Pipeline->SetDepthStencil(ds);

    ////~ Formats
    m_mainPassInfo.Pipeline->SetNumRenderTargets(1u);
    m_mainPassInfo.Pipeline->SetRTVFormat(0u, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_mainPassInfo.Pipeline->SetDSVFormat(DXGI_FORMAT_D32_FLOAT);

    ////~ MSAA (default off)
    m_mainPassInfo.Pipeline->SetSampleMask(UINT_MAX);
    m_mainPassInfo.Pipeline->SetSampleDesc(1u, 0u);

    //~ Primitive type
    m_mainPassInfo.Pipeline->SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
     
    //~ Build pipeline
    if (!m_mainPassInfo.Pipeline->Build(device))
    {
        LOG_ERROR("InitMainPipeline: Failed to build main PSO.");
        return false;
    }

    //~ Mark clean
    m_sceneInfo.PipelineDirty = false;

    LOG_SUCCESS("Main pipeline created successfully.");
    return true;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::InitPrimaryConstantBuffer(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (m_primaryCBFrame.IsInitialized()) return true;

    KFE_FRAME_CONSTANT_BUFFER_DESC cb{};
    cb.Device       = desc.Device;
    cb.FrameCount   = m_frameCount;
    cb.ResourceHeap = desc.ResourceHeap;
    cb.SizeInBytes  = sizeof(KFE_COMMON_CB_GPU);

    if (!m_primaryCBFrame.Initialize(cb))
    {
        LOG_ERROR("Failed to build Primary Constant Buffer");
        return false;
    }

    return true;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::InitMainSampler(const KFE_BUILD_OBJECT_DESC& desc)
{
    //~ Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("InitMainSampler: Device or native device is null.");
        return false;
    }

    //~ Validate sampler heap
    if (!desc.SamplerHeap)
    {
        LOG_ERROR("InitMainSampler: SamplerHeap is null.");
        return false;
    }

    m_mainPassInfo.SamplerHeap = desc.SamplerHeap;

    //~ Create sampler object if needed
    if (!m_mainPassInfo.Sampler)
        m_mainPassInfo.Sampler = std::make_unique<KFESampler>();

    //~ If has a valid descriptor index
    if (m_mainPassInfo.SamplerIndex != KFE_INVALID_INDEX)
        return true;

    //~ Create regular sampler (s0)
    KFE_SAMPLER_CREATE_DESC sdesc{};
    sdesc.Device = desc.Device;
    sdesc.Heap = m_mainPassInfo.SamplerHeap;

    sdesc.Filter         = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sdesc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sdesc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sdesc.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sdesc.MipLODBias     = 0.0f;
    sdesc.MaxAnisotropy  = 1u;
    sdesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    sdesc.BorderColor[0] = 0.0f;
    sdesc.BorderColor[1] = 0.0f;
    sdesc.BorderColor[2] = 0.0f;
    sdesc.BorderColor[3] = 0.0f;

    sdesc.MinLOD = 0.0f;
    sdesc.MaxLOD = D3D12_FLOAT32_MAX;

    sdesc.DescriptorIndex = KFE_INVALID_INDEX;

    if (!m_mainPassInfo.Sampler->Initialize(sdesc))
    {
        LOG_ERROR("Failed to initialize main sampler.");
        m_mainPassInfo.Sampler.reset();
        m_mainPassInfo.SamplerIndex = KFE_INVALID_INDEX;
        return false;
    }

    m_mainPassInfo.SamplerIndex = m_mainPassInfo.Sampler->GetDescriptorIndex();
    if (m_mainPassInfo.SamplerIndex == KFE_INVALID_INDEX)
    {
        LOG_ERROR("Main sampler returned invalid descriptor index.");
        m_mainPassInfo.Sampler.reset();
        return false;
    }

    LOG_SUCCESS("Main sampler created. Index={}", m_mainPassInfo.SamplerIndex);
    return true;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::InitShadowRootSignature(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (!desc.Device)
    {
        LOG_ERROR("Device is null.");
        return false;
    }

    if (m_shadowPassInfo.RootSignature)
        return true;

    m_shadowPassInfo.RootSignature = std::make_unique<KFERootSignature>();

    D3D12_ROOT_PARAMETER params[2]{};

    //~ b0: CommonCB
    params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;
    params[0].Descriptor.ShaderRegister = 0u; //~ b0
    params[0].Descriptor.RegisterSpace  = 0u;

    //~ b1: Light Data
    params[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;
    params[1].Descriptor.ShaderRegister = 1u; //~ b1
    params[1].Descriptor.RegisterSpace  = 0u;

    KFE_RG_CREATE_DESC root{};
    root.Device             = desc.Device;
    root.Flags              = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    root.NumRootParameters  = static_cast<UINT>(_countof(params));
    root.RootParameters     = params;
    root.NumStaticSamplers  = 0u;
    root.StaticSamplers     = nullptr;

    if (!m_shadowPassInfo.RootSignature->Initialize(root))
    {
        LOG_ERROR("Failed to create shadow root signature.");
        m_shadowPassInfo.RootSignature.reset();
        return false;
    }

    m_shadowPassInfo.RootSignature->SetDebugName(L"KFE Common Shadow Root Signature");
    LOG_SUCCESS("Common Shadow Root Signature Created!");
    return true;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::InitShadowPipeline(KFEDevice* device)
{
    //~ Validate device
    if (!device || !device->GetNative())
    {
        LOG_ERROR("InitShadowPipeline: Device or native device is null.");
        return false;
    }

    //~ Skip rebuild if already valid
    if (m_shadowPassInfo.Pipeline && !m_sceneInfo.ShadowPipelineDirty)
    {
        LOG_SUCCESS("Shadow pipeline already built.");
        return true;
    }

    //~ Root signature must exist
    if (!m_shadowPassInfo.RootSignature ||
        !m_shadowPassInfo.RootSignature->GetNative())
    {
        LOG_ERROR("InitShadowPipeline: Shadow root signature not initialized.");
        return false;
    }

    //~ Validate shader path
    if (!kfe_helpers::IsFile(m_shaderInfo.ShadowVertexShader))
    {
        LOG_ERROR("Shadow VS not found: {}", m_shaderInfo.ShadowVertexShader);
        return false;
    }

    //~ Compile or load shadow vertex shader
    ID3DBlob* vsBlob =
        shaders::GetOrCompile(
            m_shaderInfo.ShadowVertexShader,
            "main",
            "vs_5_0");

    if (!vsBlob)
    {
        LOG_ERROR("InitShadowPipeline: Failed to compile shadow VS.");
        return false;
    }

    //~ Recreate pipeline container
    if (m_shadowPassInfo.Pipeline)
    {
        m_shadowPassInfo.Pipeline->Destroy();
    }
    else
    {
        m_shadowPassInfo.Pipeline = std::make_unique<KFEPipelineState>();
    }

    //~ Input layout
    const auto layout = KFEMeshGeometry::GetInputLayout();
    if (layout.empty())
    {
        LOG_ERROR("InitShadowPipeline: Input layout is empty.");
        return false;
    }

    m_shadowPassInfo.Pipeline->SetInputLayout(
        layout.data(),
        static_cast<UINT>(layout.size()));

    //~ Vertex shader only
    D3D12_SHADER_BYTECODE vs{};
    vs.pShaderBytecode = vsBlob->GetBufferPointer();
    vs.BytecodeLength = vsBlob->GetBufferSize();

    m_shadowPassInfo.Pipeline->SetVS(vs);
    m_shadowPassInfo.Pipeline->SetPS({});

    //~ Root signature
    m_shadowPassInfo.Pipeline->SetRootSignature(
        static_cast<ID3D12RootSignature*>(
            m_shadowPassInfo.RootSignature->GetNative()));

    //~ Rasterizer (shadow bias)
    D3D12_RASTERIZER_DESC raster{};
    raster.FillMode = D3D12_FILL_MODE_SOLID;
    raster.CullMode = D3D12_CULL_MODE_BACK;
    raster.FrontCounterClockwise = FALSE;
    raster.DepthClipEnable = TRUE;
    raster.MultisampleEnable = FALSE;
    raster.AntialiasedLineEnable = FALSE;
    raster.ForcedSampleCount = 0u;
    raster.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    raster.DepthBias = 1000;
    raster.DepthBiasClamp = 0.0f;
    raster.SlopeScaledDepthBias = 1.0f;

    m_shadowPassInfo.Pipeline->SetRasterizer(raster);

    //~ Depth-only pass
    D3D12_DEPTH_STENCIL_DESC ds{};
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ds.StencilEnable = FALSE;

    m_shadowPassInfo.Pipeline->SetDepthStencil(ds);
    m_shadowPassInfo.Pipeline->SetDSVFormat(DXGI_FORMAT_D32_FLOAT);

    //~ No color outputs
    m_shadowPassInfo.Pipeline->SetNumRenderTargets(0u);

    //~ Misc PSO state
    m_shadowPassInfo.Pipeline->SetSampleMask(UINT_MAX);
    m_shadowPassInfo.Pipeline->SetSampleDesc(1u, 0u);
    m_shadowPassInfo.Pipeline->SetPrimitiveType(
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

    //~ Build pipeline
    if (!m_shadowPassInfo.Pipeline->Build(device))
    {
        LOG_ERROR("InitShadowPipeline: Failed to build shadow PSO.");
        return false;
    }

    //~ Mark clean
    m_sceneInfo.ShadowPipelineDirty = false;

    LOG_SUCCESS("Shadow pipeline created successfully.");
    return true;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::InitShadowLightMetaConstantbuffer(const KFE_BUILD_OBJECT_DESC& desc)
{
    if (m_lightCBFrame.IsInitialized()) return true;

    KFE_FRAME_CONSTANT_BUFFER_DESC cbDesc{};
    cbDesc.Device       = desc.Device;
    cbDesc.ResourceHeap = desc.ResourceHeap;
    cbDesc.FrameCount   = m_frameCount;
    cbDesc.SizeInBytes  = sizeof(KFE_LIGHT_DATA_GPU);

    if (!m_lightCBFrame.Initialize(cbDesc)) 
    {
        LOG_ERROR("Failed to Create light Constant buffer!");
        return false;
    }

    return true;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::InitShadowComparisionSampler(const KFE_BUILD_OBJECT_DESC& desc)
{
    //~ Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("InitShadowComparisionSampler: Device or native device is null.");
        return false;
    }

    //~ Validate sampler heap
    if (!desc.SamplerHeap)
    {
        LOG_ERROR("InitShadowComparisionSampler: SamplerHeap is null.");
        return false;
    }

    m_shadowPassInfo.SamplerHeap = desc.SamplerHeap;

    //~ Create sampler object if needed
    if (!m_shadowPassInfo.Sampler)
        m_shadowPassInfo.Sampler = std::make_unique<KFESampler>();

    //~ If already has a valid descriptor index, we are done
    if (m_shadowPassInfo.SamplerIndex != KFE_INVALID_INDEX)
        return true;

    //~ Create shadow comparison sampler (s1)
    KFE_SAMPLER_CREATE_DESC sdesc{};
    sdesc.Device = desc.Device;
    sdesc.Heap = m_shadowPassInfo.SamplerHeap;

    sdesc.Filter         = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    sdesc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sdesc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sdesc.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sdesc.MipLODBias     = 0.0f;
    sdesc.MaxAnisotropy  = 1u;
    sdesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    sdesc.BorderColor[0] = 0.0f;
    sdesc.BorderColor[1] = 0.0f;
    sdesc.BorderColor[2] = 0.0f;
    sdesc.BorderColor[3] = 0.0f;

    sdesc.MinLOD = 0.0f;
    sdesc.MaxLOD = D3D12_FLOAT32_MAX;

    sdesc.DescriptorIndex = KFE_INVALID_INDEX;

    if (!m_shadowPassInfo.Sampler->Initialize(sdesc))
    {
        LOG_ERROR("Failed to initialize shadow comparison sampler.");
        m_shadowPassInfo.Sampler.reset();
        m_shadowPassInfo.SamplerIndex = KFE_INVALID_INDEX;
        return false;
    }

    m_shadowPassInfo.SamplerIndex = m_shadowPassInfo.Sampler->GetDescriptorIndex();
    if (m_shadowPassInfo.SamplerIndex == KFE_INVALID_INDEX)
    {
        LOG_ERROR("Shadow sampler returned invalid descriptor index.");
        m_shadowPassInfo.Sampler.reset();
        return false;
    }

    LOG_SUCCESS("Shadow comparison sampler created. Index={}", m_shadowPassInfo.SamplerIndex);
    return true;
}

void kfe::IKFESceneObject::UpdatePrimaryConstantBuffer(const KFE_UPDATE_OBJECT_DESC& desc)
{
    if (!m_primaryCBFrame.IsInitialized()) return;

    auto* dst = static_cast<KFE_COMMON_CB_GPU*>(m_primaryCBFrame.GetMappedData());
    if (!dst) return;

    using namespace DirectX;

    //~ World matrices
    const XMMATRIX W = Transform.GetMatrix();

    XMMATRIX W_noT = W;
    W_noT.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);

    XMVECTOR detW{};
    const XMMATRIX WInvT =
        XMMatrixTranspose(XMMatrixInverse(&detW, W_noT));

    XMStoreFloat4x4(&dst->WorldT, XMMatrixTranspose(W));
    XMStoreFloat4x4(&dst->WorldInvTransposeT, WInvT);

    //~ View / Projection (ALREADY TRANSPOSED — DO NOT TOUCH)
    XMStoreFloat4x4(&dst->ViewT, desc.ViewMatrixT);
    XMStoreFloat4x4(&dst->ProjT, desc.PerpectiveMatrixT);
    XMStoreFloat4x4(&dst->OrthoT, desc.OrthographicMatrixT);

    //~ ViewProj (build from non-transposed, then transpose ONCE)
    const XMMATRIX VP =
        XMMatrixMultiply(
            XMMatrixTranspose(desc.ViewMatrixT),
            XMMatrixTranspose(desc.PerpectiveMatrixT));

    XMStoreFloat4x4(&dst->ViewProjT, XMMatrixTranspose(VP));

    //~ Camera
    dst->CameraPosWS = desc.CameraPosition;
    dst->CameraNear = desc.ZNear;
    dst->CameraForwardWS = desc.CameraForwardWS;
    dst->CameraFar = desc.ZFar;
    dst->CameraRightWS = desc.CameraRightWS;
    dst->_PadCamRight = 0.0f;
    dst->CameraUpWS = desc.CameraUpWS;
    dst->_PadCamUp = 0.0f;

    //~ Object / Player
    dst->ObjectPosWS = desc.ObjectPosition;
    dst->_PadObjPos = 0.0f;
    dst->PlayerPosWS = desc.PlayerPosition;
    dst->_PadPlayerPos = 0.0f;

    //~ Viewport
    dst->Resolution = desc.Resolution;

    dst->InvResolution.x =
        (desc.Resolution.x != 0.0f) ? (1.0f / desc.Resolution.x) : 0.0f;
    dst->InvResolution.y =
        (desc.Resolution.y != 0.0f) ? (1.0f / desc.Resolution.y) : 0.0f;

    //~ Mouse
    dst->MousePosPixels = desc.MousePosition;

    dst->MousePosNDC.x =
        (desc.Resolution.x != 0.0f)
        ? ((desc.MousePosition.x / desc.Resolution.x) * 2.0f - 1.0f)
        : 0.0f;

    dst->MousePosNDC.y =
        (desc.Resolution.y != 0.0f)
        ? (1.0f - (desc.MousePosition.y / desc.Resolution.y) * 2.0f)
        : 0.0f;

    //~ Time
    dst->Time = desc.Time;
    dst->DeltaTime = desc.DeltaTime;
    dst->_PadTime0 = 0.0f;
    dst->_PadTime1 = 0.0f;

    //~ Lights / Flags
    dst->NumTotalLights = m_lightManager.GetPackedCount();
    dst->RenderFlags = 0u;
    dst->_PadFlags0 = 0u;
    dst->_PadFlags1 = 0u;
}

_Use_decl_annotations_
void kfe::IKFESceneObject::UpdateLightConstantBuffer(const KFE_LIGHT_DATA_GPU& desc)
{
    if (!m_lightCBFrame.IsInitialized()) return;
    auto* dst = static_cast<KFE_LIGHT_DATA_GPU*>(m_lightCBFrame.GetMappedData());
    if (!dst) return;
    *dst = desc;
}

void kfe::IKFESceneObject::AttachLight(IKFELight* light)
{
    if (!light) return;
    if (light->CanCullByDistance())
    {
        if (light->IsInCullRadius(Transform.Position))
        {
            m_lightManager.AttachLight(light);
        }
    }
    else
    {
        m_lightManager.AttachLight(light);
    }
}

void kfe::IKFESceneObject::DetachLight(IKFELight* light)
{
    if (!light) return;
    DetachLight(light->GetAssignedKey());
}

void kfe::IKFESceneObject::DetachLight(KID id)
{
    m_lightManager.DetachLight(id);
}
