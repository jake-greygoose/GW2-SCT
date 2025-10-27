#include "Texture.h"
#include "Common.h"
#include <thread>

static std::thread::id g_renderThreadId; // set during Present()

void GW2_SCT::TextureD3D11::MarkRenderThread() {
    g_renderThreadId = std::this_thread::get_id();
}

bool GW2_SCT::TextureD3D11::IsOnRenderThread() {
    return g_renderThreadId != std::thread::id{} &&
        std::this_thread::get_id() == g_renderThreadId;
}

GW2_SCT::TextureD3D11::TextureD3D11(int width, int height, unsigned char* data) : width(width), height(height) {
    if (data == nullptr) {
        this->data = nullptr;
    }
    else {
        size_t size = sizeof(unsigned char) * width * height * 4;
        this->data = (unsigned char*)malloc(size);
        if (this->data != nullptr) {
            memcpy(this->data, data, size);
        }
    }
}

GW2_SCT::TextureD3D11::~TextureD3D11() {
    if (_texture11View != nullptr) _texture11View->Release();
    if (_texture11 != nullptr) _texture11->Release();
    if (data != nullptr) {
        free(data);
        data = nullptr;
    }
}

bool GW2_SCT::TextureD3D11::create() {
    // Enforce render thread only
    if (!IsOnRenderThread()) {
        LOG("ERROR: D3D11 texture creation attempted off render thread - will defer");
        return false;
    }

    if (d3Device11 == nullptr) {
        LOG("Cannot create D3D11 texture: d3Device11 is null");
        return false;
    }

    if (_texture11 != nullptr) {
        LOG("D3D11 texture already created, skipping recreation");
        return true;
    }

#if _DEBUG
    LOG("Creating D3D11 texture on render thread - size: ", width, "x", height);
#endif

    HRESULT res;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    if (data != nullptr) {
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;

        if (FAILED(res = d3Device11->CreateTexture2D(&desc, &subResource, &_texture11))) {
            LOG("d3Device11->CreateTexture2D failed: " + std::to_string(res));
            return false;
        }
#if _DEBUG
        LOG("D3D11 texture created successfully with data");
#endif
    }
    else {
        if (FAILED(res = d3Device11->CreateTexture2D(&desc, NULL, &_texture11))) {
            LOG("d3Device11->CreateTexture2D failed: " + std::to_string(res));
            return false;
        }
#if _DEBUG
        LOG("D3D11 texture created successfully without data");
#endif
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    if (FAILED(res = d3Device11->CreateShaderResourceView(_texture11, &srvDesc, &_texture11View))) {
        LOG("d3Device11->CreateShaderResourceView failed: " + std::to_string(res));
        _texture11->Release();
        _texture11 = nullptr;
        return false;
    }

    if (data != nullptr) {
        free(data);
        data = nullptr;
    }

    LOG("D3D11 texture and shader resource view created successfully");
    return true;
}

GW2_SCT::ImmutableTextureD3D11::ImmutableTextureD3D11(int width, int height, unsigned char* data)
    : TextureD3D11(width, height, data), ImmutableTexture(width, height) {
}

void GW2_SCT::ImmutableTextureD3D11::internalDraw(ImVec2 pos, ImVec2 size, ImVec2 uvStart, ImVec2 uvEnd, ImU32 color) {
    if (_texture11View != nullptr) {
        ImGui::GetForegroundDrawList()->AddImage(_texture11View, pos, ImVec2(pos.x + size.x, pos.y + size.y), uvStart, uvEnd, color);
    }
    else {
        LOG("WARNING: Attempting to draw with null texture view");
    }
}

bool GW2_SCT::ImmutableTextureD3D11::internalCreate() {
#if _DEBUG
    LOG("ImmutableTextureD3D11: Calling create()");
#endif
    return create();
}

GW2_SCT::MutableTextureD3D11::MutableTextureD3D11(int w, int h)
    : TextureD3D11(w, h, nullptr), MutableTexture(w, h) {
}

GW2_SCT::MutableTextureD3D11::~MutableTextureD3D11() {
    if (_texture11Staging != nullptr) _texture11Staging->Release();
}

void GW2_SCT::MutableTextureD3D11::internalDraw(ImVec2 pos, ImVec2 size, ImVec2 uvStart, ImVec2 uvEnd, ImU32 color) {
    if (_stagingChanged && d3D11Context != nullptr && _texture11View != nullptr) {
        std::lock_guard<std::mutex> lock(_stagingMutex);
        if (_stagingChanged) {
            d3D11Context->CopyResource(_texture11, _texture11Staging);
            _stagingChanged = false;
        }
    }
    if (_texture11View != nullptr) {
        ImGui::GetForegroundDrawList()->AddImage(_texture11View, pos, ImVec2(pos.x + size.x, pos.y + size.y), uvStart, uvEnd, color);
    }
}

bool GW2_SCT::MutableTextureD3D11::internalCreate() {
    if (!create()) return false;

    if (_texture11Staging != nullptr) _texture11Staging->Release();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

    HRESULT res;
    if (FAILED(res = d3Device11->CreateTexture2D(&desc, NULL, &_texture11Staging))) {
        LOG("d3Device11->CreateTexture2D (staging) failed: " + std::to_string(res));
        return false;
    }
    return true;
}

bool GW2_SCT::MutableTextureD3D11::internalStartUpdate(ImVec2 pos, ImVec2 size, UpdateData* out) {
    if (_texture11Staging == nullptr) return false;
    if (d3D11Context == nullptr) return false;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT res = d3D11Context->Map(_texture11Staging, 0, D3D11_MAP_WRITE, 0, &mapped);
    if (FAILED(res)) {
        LOG("Failed to map staging texture for update: " + std::to_string(res));
        return false;
    }

    std::lock_guard<std::mutex> lock(_stagingMutex);
    out->id = 0;
    out->data = (unsigned char*)mapped.pData + (size_t)pos.y * mapped.RowPitch + (size_t)pos.x * 4;
    out->rowPitch = (int)mapped.RowPitch;
    out->bytePerPixel = 4;
    _stagingChanged = true;
    return true;
}

bool GW2_SCT::MutableTextureD3D11::internalEndUpdate() {
    if (d3D11Context == nullptr) return false;
    d3D11Context->Unmap(_texture11Staging, 0);
    return true;
}
