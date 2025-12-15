#include "pch.h"
#include "engine/render_manager/post/post_effect_fullscreen_quad.h"

#include "engine/utils/logger.h"
#include "imgui/imgui.h"

#include "engine/render_manager/assets_library/shader_library.h"

void kfe::KFEPostEffect_FullscreenQuad::Update(const KFEWindows* window)
{
    if (!window)
        return;

    const KFEMouseInputs& mouse = window->Mouse;

    const auto winSize = window->GetWinSize().As<float>();
    const float w = std::max(winSize.Width, 1.0f);
    const float h = std::max(winSize.Height, 1.0f);

    m_cbData.Resolution[0] = w;
    m_cbData.Resolution[1] = h;
    m_cbData.InvResolution[0] = 1.0f / w;
    m_cbData.InvResolution[1] = 1.0f / h;

    int mx = 0, my = 0;
    mouse.GetMousePosition(mx, my);

    const float fx = std::clamp(static_cast<float>(mx), 0.0f, w);
    const float fy = std::clamp(static_cast<float>(my), 0.0f, h);

    m_cbData.MousePosPixels[0] = fx;
    m_cbData.MousePosPixels[1] = fy;

    m_cbData.MousePosUV[0] = fx * m_cbData.InvResolution[0];
    m_cbData.MousePosUV[1] = fy * m_cbData.InvResolution[1];

    m_cbData.MouseButtons[0] = mouse.IsMouseButtonPressed(0) ? 1.0f : 0.0f;
    m_cbData.MouseButtons[1] = mouse.IsMouseButtonPressed(1) ? 1.0f : 0.0f;
    m_cbData.MouseButtons[2] = mouse.IsMouseButtonPressed(2) ? 1.0f : 0.0f;

    m_cbData.MouseWheel = static_cast<float>(mouse.GetMouseWheelDelta());

    m_cbData.Gamma = std::max(m_cbData.Gamma, 0.001f);
}

_Use_decl_annotations_
bool kfe::KFEPostEffect_FullscreenQuad::Initialize(const KFE_POST_EFFECT_INIT_DESC& desc)
{
    if (m_initialized)
    {
        LOG_WARNING("KFEPostEffect_FullscreenQuad::Initialize: Already initialized.");
        return true;
    }

    if (desc.Device == nullptr || desc.Device->GetNative() == nullptr)
    {
        LOG_ERROR("Device is null.");
        return false;
    }

    if (desc.ResourceHeap == nullptr)
    {
        LOG_ERROR("ResourceHeap is null.");
        return false;
    }

    m_device = desc.Device;
    m_resourceHeap = desc.ResourceHeap;
    m_cbData.Exposure = 1.0f;
    m_cbData.Invert = 0.0f;
    m_cbData.Time = 0.0f;

    KFE_FRAME_CONSTANT_BUFFER_DESC cb{};
    cb.Device = desc.Device;
    cb.FrameCount = 3u;
    cb.ResourceHeap = desc.ResourceHeap;
    cb.SizeInBytes = sizeof(FullQuadPostEffect_CB);

    if (!m_constantBuffer.Initialize(cb))
    {
        LOG_ERROR("Failed to create cb!");
        Destroy();
        return false;
    }

    if (!CreatePSO(desc.OutputFormat))
    {
        LOG_ERROR("CreatePSO failed.");
        Destroy();
        return false;
    }

    m_reservedIndex = desc.ResourceHeap->Allocate();
    if (m_reservedIndex == KFE_INVALID_INDEX)
    {
        LOG_ERROR("Failed to allocate reserved SRV index.");
        return false;
    }

    m_initialized = true;
    LOG_SUCCESS("KFEPostEffect_FullscreenQuad::Initialize: Initialized.");
    return true;
}

void kfe::KFEPostEffect_FullscreenQuad::Destroy() noexcept
{
    m_initialized = false;
    m_device      = nullptr;
    m_resourceHeap = nullptr;

    if (m_reservedIndex != KFE_INVALID_INDEX && m_resourceHeap)
        (void)m_resourceHeap->Free(m_reservedIndex);

    m_pso.Destroy();
    (void)m_root.Destroy();
    (void)m_vertexBuffer.Destroy();
    (void)m_indexBuffer.Destroy();
    (void)m_constantBuffer.Destroy();

    LOG_SUCCESS("KFEPostEffect_FullscreenQuad::Destroy: Destroyed.");
}

_Use_decl_annotations_
void kfe::KFEPostEffect_FullscreenQuad::Render(const KFE_POST_EFFECT_RENDER_DESC& desc)
{
    if (!m_initialized)
        return;

    if (!desc.Cmd)
        return;

    if (desc.OutputRTV.ptr == 0)
    {
        LOG_WARNING("PostEffect Render: OutputRTV is null.");
        return;
    }

    if (!m_device || !m_device->GetNative())
    {
        LOG_WARNING("PostEffect Render: device is null.");
        return;
    }

    if (!m_resourceHeap)
    {
        LOG_WARNING("PostEffect Render: resource heap  is null");
        return;
    }

    if (m_reservedIndex == KFE_INVALID_INDEX) 
    {
        LOG_WARNING("PostEffect Render:m_reservedIndex is not valid!");
        return;
    }

    if (desc.InputSceneSRVIndex == KFE_INVALID_INDEX || !m_resourceHeap->IsValidIndex(desc.InputSceneSRVIndex))
    {
        LOG_WARNING("PostEffect Render: InputSceneSRVIndex invalid.");
        return;
    }

    // Update CB
    if (m_constantBuffer.IsInitialized())
    {
        auto* data = static_cast<FullQuadPostEffect_CB*>(m_constantBuffer.GetMappedData());
        if (data)
        {
            *data = m_cbData;
        }
    }

    ID3D12GraphicsCommandList* cmd = desc.Cmd;

    if (desc.Viewport) cmd->RSSetViewports(1u, desc.Viewport);
    if (desc.Scissor)  cmd->RSSetScissorRects(1u, desc.Scissor);

    cmd->OMSetRenderTargets(1u, &desc.OutputRTV, FALSE, nullptr);

    ID3D12RootSignature* rs = static_cast<ID3D12RootSignature*>(m_root.GetNative());
    ID3D12PipelineState* pso = m_pso.GetNative();

    if (!rs || !pso)
    {
        LOG_WARNING("PostEffect Render: RootSig/PSO missing.");
        return;
    }

    cmd->SetGraphicsRootSignature(rs);
    cmd->SetPipelineState(pso);

    // Copy SRV descriptor contents into reserved slot (t0)
    const D3D12_CPU_DESCRIPTOR_HANDLE srcCPU = m_resourceHeap->GetHandle(desc.InputSceneSRVIndex);
    const D3D12_CPU_DESCRIPTOR_HANDLE dstCPU = m_resourceHeap->GetHandle(m_reservedIndex);

    m_device->GetNative()->CopyDescriptorsSimple(
        1u,
        dstCPU,
        srcCPU,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Bind reserved slot as t0
    const D3D12_GPU_DESCRIPTOR_HANDLE reservedGPU = m_resourceHeap->GetGPUHandle(m_reservedIndex);
    cmd->SetGraphicsRootDescriptorTable(desc.RootParam_SceneSRV, reservedGPU);

    // Bind b0
    cmd->SetGraphicsRootConstantBufferView(
        0u,
        m_constantBuffer.GetView()->GetGPUVirtualAddress());

    // Fullscreen triangle
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->DrawInstanced(3u, 1u, 0u, 0u);

    m_constantBuffer.Step();
}

void kfe::KFEPostEffect_FullscreenQuad::ImguiView(float deltaTime)
{
    if (!m_initialized)
        return;

    m_cbData.Time += deltaTime;

    ImGui::Text("Post Effect: %s", GetPostName().c_str());

    // Enable toggle
    bool enabled = IsEnable();
    if (ImGui::Checkbox("Enabled", &enabled))
    {
        enabled ? Enable() : Disable();
    }

    ImGui::BeginDisabled(!IsEnable());

    ImGui::SeparatorText("Color");
    ImGui::SliderFloat("Exposure", &m_cbData.Exposure, 0.0f, 4.0f);
    ImGui::SliderFloat("Gamma", &m_cbData.Gamma, 0.5f, 3.5f);
    ImGui::SliderFloat("Contrast", &m_cbData.Contrast, 0.0f, 2.0f);
    ImGui::SliderFloat("Saturation", &m_cbData.Saturation, 0.0f, 2.0f);

    ImGui::SeparatorText("Toggles");

    bool invert = (m_cbData.Invert >= 0.5f);
    if (ImGui::Checkbox("Invert", &invert))
        m_cbData.Invert = invert ? 1.0f : 0.0f;

    ImGui::SliderFloat("Grayscale", &m_cbData.Grayscale, 0.0f, 1.0f);

    ImGui::SeparatorText("Vignette");
    ImGui::SliderFloat("Vignette", &m_cbData.Vignette, 0.0f, 1.0f);
    ImGui::SliderFloat("Vignette Power", &m_cbData.VignettePower, 0.5f, 8.0f);

    ImGui::SeparatorText("Lens / Screen");
    ImGui::SliderFloat("Chromatic Aberration", &m_cbData.ChromAbStrength, 0.0f, 2.0f);
    ImGui::SliderFloat("Scanlines", &m_cbData.ScanlineStrength, 0.0f, 2.0f);
    ImGui::SliderFloat("Grain", &m_cbData.GrainStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Dither", &m_cbData.DitherStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Fade", &m_cbData.Fade, 0.0f, 1.0f);

    ImGui::SeparatorText("Mouse Fun");
    ImGui::SliderFloat("Radial Blur Strength", &m_cbData.RadialBlurStrength, 0.0f, 2.0f);
    ImGui::SliderFloat("Radial Blur Radius", &m_cbData.RadialBlurRadius, 0.01f, 1.0f);

    ImGui::SeparatorText("Debug (Read-only)");
    ImGui::Text("Resolution: %.0f x %.0f", m_cbData.Resolution[0], m_cbData.Resolution[1]);
    ImGui::Text("Mouse Pixels: %.1f, %.1f", m_cbData.MousePosPixels[0], m_cbData.MousePosPixels[1]);
    ImGui::Text("Mouse UV: %.3f, %.3f", m_cbData.MousePosUV[0], m_cbData.MousePosUV[1]);
    ImGui::Text("Buttons L/R/M: %.0f %.0f %.0f",
        m_cbData.MouseButtons[0], m_cbData.MouseButtons[1], m_cbData.MouseButtons[2]);
    ImGui::Text("Wheel: %.2f", m_cbData.MouseWheel);

    if (ImGui::Button("Reset"))
    {
        m_cbData.Exposure = 1.0f;
        m_cbData.Gamma = 2.2f;
        m_cbData.Contrast = 1.0f;
        m_cbData.Saturation = 1.0f;
        m_cbData.Invert = 0.0f;
        m_cbData.Grayscale = 0.0f;
        m_cbData.Vignette = 0.0f;
        m_cbData.VignettePower = 2.0f;
    }

    ImGui::EndDisabled();
}

JsonLoader kfe::KFEPostEffect_FullscreenQuad::GetJsonData() const
{
    JsonLoader j;
    j["Type"] = GetPEClassName();
    j["PostName"] = GetPostName();
    j["Enabled"] = IsEnable();

    // Color
    j["Exposure"] = std::to_string(m_cbData.Exposure);
    j["Gamma"] = std::to_string(m_cbData.Gamma);
    j["Contrast"] = std::to_string(m_cbData.Contrast);
    j["Saturation"] = std::to_string(m_cbData.Saturation);

    // Toggles / misc
    j["Invert"] = std::to_string(m_cbData.Invert);
    j["Grayscale"] = std::to_string(m_cbData.Grayscale);
    j["Fade"] = std::to_string(m_cbData.Fade);

    // Vignette
    j["Vignette"] = std::to_string(m_cbData.Vignette);
    j["VignettePower"] = std::to_string(m_cbData.VignettePower);

    // Screen FX
    j["BlurStrength"] = std::to_string(m_cbData.BlurStrength);
    j["SharpenStrength"] = std::to_string(m_cbData.SharpenStrength);
    j["GrainStrength"] = std::to_string(m_cbData.GrainStrength);
    j["ChromAbStrength"] = std::to_string(m_cbData.ChromAbStrength);
    j["ScanlineStrength"] = std::to_string(m_cbData.ScanlineStrength);
    j["DitherStrength"] = std::to_string(m_cbData.DitherStrength);

    // Color grading and tonemap
    j["Temperature"] = std::to_string(m_cbData.Temperature);
    j["Tint"] = std::to_string(m_cbData.Tint);
    j["HueShift"] = std::to_string(m_cbData.HueShift);
    j["TonemapType"] = std::to_string(m_cbData.TonemapType);
    j["WhitePoint"] = std::to_string(m_cbData.WhitePoint);

    //  Bloom
    j["BloomStrength"] = std::to_string(m_cbData.BloomStrength);
    j["BloomThreshold"] = std::to_string(m_cbData.BloomThreshold);
    j["BloomKnee"] = std::to_string(m_cbData.BloomKnee);

    // Lens and presentation
    j["LensDistortion"] = std::to_string(m_cbData.LensDistortion);
    j["Letterbox"] = std::to_string(m_cbData.Letterbox);
    j["LetterboxSoftness"] = std::to_string(m_cbData.LetterboxSoftness);

    j["RadialBlurStrength"] = std::to_string(m_cbData.RadialBlurStrength);
    j["RadialBlurRadius"] = std::to_string(m_cbData.RadialBlurRadius);

    return j;
}

void kfe::KFEPostEffect_FullscreenQuad::LoadFromJson(const JsonLoader& loader)
{
    if (loader.Has("PostName"))
        SetPostName(loader["PostName"].GetValue());

    if (loader.Has("Enabled"))
        loader["Enabled"].AsBool() ? Enable() : Disable();

    // Color
    if (loader.Has("Exposure"))   m_cbData.Exposure = loader["Exposure"].AsFloat();
    if (loader.Has("Gamma"))      m_cbData.Gamma = loader["Gamma"].AsFloat();
    if (loader.Has("Contrast"))   m_cbData.Contrast = loader["Contrast"].AsFloat();
    if (loader.Has("Saturation")) m_cbData.Saturation = loader["Saturation"].AsFloat();

    // Toggles / misc
    if (loader.Has("Invert"))    m_cbData.Invert = loader["Invert"].AsFloat();
    if (loader.Has("Grayscale")) m_cbData.Grayscale = loader["Grayscale"].AsFloat();
    if (loader.Has("Fade"))      m_cbData.Fade = loader["Fade"].AsFloat();

    // Vignette
    if (loader.Has("Vignette"))      m_cbData.Vignette = loader["Vignette"].AsFloat();
    if (loader.Has("VignettePower")) m_cbData.VignettePower = loader["VignettePower"].AsFloat();

    // Screen FX
    if (loader.Has("BlurStrength"))     m_cbData.BlurStrength = loader["BlurStrength"].AsFloat();
    if (loader.Has("SharpenStrength"))  m_cbData.SharpenStrength = loader["SharpenStrength"].AsFloat();
    if (loader.Has("GrainStrength"))    m_cbData.GrainStrength = loader["GrainStrength"].AsFloat();
    if (loader.Has("ChromAbStrength"))  m_cbData.ChromAbStrength = loader["ChromAbStrength"].AsFloat();
    if (loader.Has("ScanlineStrength")) m_cbData.ScanlineStrength = loader["ScanlineStrength"].AsFloat();
    if (loader.Has("DitherStrength"))   m_cbData.DitherStrength = loader["DitherStrength"].AsFloat();

    // Color grading / tonemap
    if (loader.Has("Temperature")) m_cbData.Temperature = loader["Temperature"].AsFloat();
    if (loader.Has("Tint"))        m_cbData.Tint = loader["Tint"].AsFloat();
    if (loader.Has("HueShift"))    m_cbData.HueShift = loader["HueShift"].AsFloat();
    if (loader.Has("TonemapType")) m_cbData.TonemapType = loader["TonemapType"].AsFloat();
    if (loader.Has("WhitePoint"))  m_cbData.WhitePoint = loader["WhitePoint"].AsFloat();

    // Bloom
    if (loader.Has("BloomStrength"))  m_cbData.BloomStrength = loader["BloomStrength"].AsFloat();
    if (loader.Has("BloomThreshold")) m_cbData.BloomThreshold = loader["BloomThreshold"].AsFloat();
    if (loader.Has("BloomKnee"))      m_cbData.BloomKnee = loader["BloomKnee"].AsFloat();

    // Lens / presentation
    if (loader.Has("LensDistortion"))    m_cbData.LensDistortion = loader["LensDistortion"].AsFloat();
    if (loader.Has("Letterbox"))         m_cbData.Letterbox = loader["Letterbox"].AsFloat();
    if (loader.Has("LetterboxSoftness")) m_cbData.LetterboxSoftness = loader["LetterboxSoftness"].AsFloat();

    // Mouse fun controls
    if (loader.Has("RadialBlurStrength")) m_cbData.RadialBlurStrength = loader["RadialBlurStrength"].AsFloat();
    if (loader.Has("RadialBlurRadius"))   m_cbData.RadialBlurRadius = loader["RadialBlurRadius"].AsFloat();

    m_cbData.Exposure   = std::max(m_cbData.Exposure, 0.0f);
    m_cbData.Gamma      = std::max(m_cbData.Gamma, 0.001f);
    m_cbData.Contrast   = std::max(m_cbData.Contrast, 0.0f);
    m_cbData.Saturation = std::max(m_cbData.Saturation, 0.0f);

    m_cbData.Invert     = (m_cbData.Invert >= 0.5f) ? 1.0f : 0.0f;
    m_cbData.Grayscale  = std::clamp(m_cbData.Grayscale, 0.0f, 1.0f);
    m_cbData.Fade       = std::clamp(m_cbData.Fade, 0.0f, 1.0f);

    m_cbData.Vignette       = std::clamp(m_cbData.Vignette, 0.0f, 1.0f);
    m_cbData.VignettePower  = std::max(m_cbData.VignettePower, 0.001f);

    m_cbData.BlurStrength     = std::max(m_cbData.BlurStrength, 0.0f);
    m_cbData.SharpenStrength  = std::max(m_cbData.SharpenStrength, 0.0f);
    m_cbData.GrainStrength    = std::clamp(m_cbData.GrainStrength, 0.0f, 1.0f);
    m_cbData.ChromAbStrength  = std::max(m_cbData.ChromAbStrength, 0.0f);
    m_cbData.ScanlineStrength = std::max(m_cbData.ScanlineStrength, 0.0f);
    m_cbData.DitherStrength   = std::clamp(m_cbData.DitherStrength, 0.0f, 1.0f);

    m_cbData.Temperature = std::clamp(m_cbData.Temperature, -1.0f, 1.0f);
    m_cbData.Tint        = std::clamp(m_cbData.Tint, -1.0f, 1.0f);
    m_cbData.HueShift    = std::clamp(m_cbData.HueShift, -0.5f, 0.5f);
    m_cbData.TonemapType = std::clamp(m_cbData.TonemapType, 0.0f, 3.0f);
    m_cbData.WhitePoint  = std::max(m_cbData.WhitePoint, 0.001f);

    m_cbData.BloomStrength  = std::max(m_cbData.BloomStrength, 0.0f);
    m_cbData.BloomThreshold = std::max(m_cbData.BloomThreshold, 0.0f);
    m_cbData.BloomKnee      = std::max(m_cbData.BloomKnee, 0.0f);

    m_cbData.LensDistortion     = std::clamp(m_cbData.LensDistortion, -1.0f, 1.0f);
    m_cbData.Letterbox          = std::clamp(m_cbData.Letterbox, 0.0f, 1.0f);
    m_cbData.LetterboxSoftness  = std::clamp(m_cbData.LetterboxSoftness, 0.0f, 1.0f);

    m_cbData.RadialBlurStrength = std::max(m_cbData.RadialBlurStrength, 0.0f);
    m_cbData.RadialBlurRadius   = std::clamp(m_cbData.RadialBlurRadius, 0.001f, 1.0f);
}

bool kfe::KFEPostEffect_FullscreenQuad::CreatePSO(DXGI_FORMAT outputFormat)
{
    if (!m_device || !m_device->GetNative())
    {
        LOG_ERROR("Device is null.");
        return false;
    }

    D3D12_DESCRIPTOR_RANGE srvRange[1]{};
    srvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange[0].NumDescriptors = 1u;
    srvRange[0].BaseShaderRegister = 0u; // t0
    srvRange[0].RegisterSpace = 0u;
    srvRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[2]{};

    // b0
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    params[0].Descriptor.ShaderRegister = 0u; // b0
    params[0].Descriptor.RegisterSpace = 0u;

    // t0 table
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[1].DescriptorTable.NumDescriptorRanges = 1u;
    params[1].DescriptorTable.pDescriptorRanges = srvRange;

    D3D12_STATIC_SAMPLER_DESC staticSampler{};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 1;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0u; // s0
    staticSampler.RegisterSpace = 0u;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Build root signature
    {
        KFE_RG_CREATE_DESC root{};
        root.Device = m_device;
        root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        root.NumRootParameters = static_cast<UINT>(_countof(params));
        root.RootParameters = params;
        root.NumStaticSamplers = 1u;
        root.StaticSamplers = &staticSampler;

        // Rebuild safe
        (void)m_root.Destroy();

        if (!m_root.Initialize(root))
        {
            LOG_ERROR("KFEPostEffect_FullscreenQuad::CreatePSO: Failed to create root signature.");
            return false;
        }

        m_root.SetDebugName(L"KFE PostEffect FullscreenQuad RootSig");
        LOG_SUCCESS("Post root signature created.");
    }

    if (!kfe_helpers::IsFile(m_vsPath))
    {
        LOG_ERROR("KFEPostEffect_FullscreenQuad::CreatePSO: VS not found: {}", m_vsPath);
        return false;
    }

    if (!kfe_helpers::IsFile(m_psPath))
    {
        LOG_ERROR("KFEPostEffect_FullscreenQuad::CreatePSO: PS not found: {}", m_psPath);
        return false;
    }

    ID3DBlob* vsBlob = shaders::GetOrCompile(m_vsPath, "main", "vs_5_0");
    if (!vsBlob)
    {
        LOG_ERROR("KFEPostEffect_FullscreenQuad::CreatePSO: Failed to compile VS: {}", m_vsPath);
        return false;
    }

    ID3DBlob* psBlob = shaders::GetOrCompile(m_psPath, "main", "ps_5_0");
    if (!psBlob)
    {
        LOG_ERROR("KFEPostEffect_FullscreenQuad::CreatePSO: Failed to compile PS: {}", m_psPath);
        return false;
    }

    m_pso.Destroy();
    m_pso.SetInputLayout(nullptr, 0u);

    D3D12_SHADER_BYTECODE vs{};
    vs.pShaderBytecode = vsBlob->GetBufferPointer();
    vs.BytecodeLength = vsBlob->GetBufferSize();

    D3D12_SHADER_BYTECODE ps{};
    ps.pShaderBytecode = psBlob->GetBufferPointer();
    ps.BytecodeLength = psBlob->GetBufferSize();

    m_pso.SetVS(vs);
    m_pso.SetPS(ps);

    m_pso.SetRootSignature(static_cast<ID3D12RootSignature*>(m_root.GetNative()));

    // Rasterizer
    D3D12_RASTERIZER_DESC raster{};
    raster.FillMode = D3D12_FILL_MODE_SOLID;
    raster.CullMode = D3D12_CULL_MODE_NONE;
    raster.FrontCounterClockwise = FALSE;
    raster.DepthClipEnable = TRUE;
    m_pso.SetRasterizer(raster);

    // Depth OFF
    D3D12_DEPTH_STENCIL_DESC ds{};
    ds.DepthEnable = FALSE;
    ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    ds.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    ds.StencilEnable = FALSE;
    m_pso.SetDepthStencil(ds);

    // Formats
    m_pso.SetNumRenderTargets(1u);
    m_pso.SetRTVFormat(0u, outputFormat);
    m_pso.SetDSVFormat(DXGI_FORMAT_UNKNOWN);

    // MSAA off
    m_pso.SetSampleMask(UINT_MAX);
    m_pso.SetSampleDesc(1u, 0u);

    // Primitive
    m_pso.SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

    if (!m_pso.Build(m_device))
    {
        LOG_ERROR("KFEPostEffect_FullscreenQuad::CreatePSO: Failed to build PSO.");
        return false;
    }

    LOG_SUCCESS("Post PSO created successfully.");
    return true;
}
