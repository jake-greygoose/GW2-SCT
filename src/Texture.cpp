#include "Texture.h"
#include "Common.h"
#include <queue>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace GW2_SCT {
    static std::queue<Texture*> pendingTextureCreations;
    static std::mutex textureCreationMutex;
}


GW2_SCT::Texture::Texture(int width, int height)
    : _textureWidth(width), _textureHeight(height),
    _nextCreationTry(std::chrono::system_clock::now()) {
}

void GW2_SCT::Texture::draw(ImVec2 pos, ImVec2 size, ImU32 color) {
    return draw(pos, size, ImVec2(0, 0), ImVec2(1, 1), color);
}

void GW2_SCT::Texture::draw(ImVec2 pos, ImVec2 size, ImVec2 uvStart, ImVec2 uvEnd, ImU32 color) {
    if (!_created && std::chrono::system_clock::now() > _nextCreationTry) {
        requestCreation();
    }

    if (_created) {
        internalDraw(pos, size, uvStart, uvEnd, color);
    }
}


void GW2_SCT::Texture::ensureCreation() {
    if (!_created && std::chrono::system_clock::now() > _nextCreationTry) {
        requestCreation();
    }
}

void GW2_SCT::Texture::requestCreation() {
    if (_creationRequested) return;
    std::lock_guard<std::mutex> lock(textureCreationMutex);
    pendingTextureCreations.push(this);
    _creationRequested = true;
}

void GW2_SCT::Texture::tryCreateNow() {
    if (_created || !_creationRequested) return;

    _created = internalCreate();
    _creationRequested = false;

    if (!_created) {
        _creationTries++;
        _nextCreationTry = std::chrono::system_clock::now() + std::chrono::seconds(_creationTries);
        LOG("[", getTimestamp(), "] TEXTURE: Creation failed, will retry");
    }
}

void GW2_SCT::Texture::BeginPresentCycle() {

    TextureD3D11::MarkRenderThread();
    ProcessPendingCreations();
}

void GW2_SCT::Texture::ProcessPendingCreations() {
    std::lock_guard<std::mutex> lock(textureCreationMutex);

    if (!TextureD3D11::IsOnRenderThread()) return;

    const int maxTexturesPerFrame = 5;
    int processed = 0;

    while (!pendingTextureCreations.empty() && processed < maxTexturesPerFrame) {
        Texture* texture = pendingTextureCreations.front();
        pendingTextureCreations.pop();

        if (texture) {
            try {
                texture->tryCreateNow();
            }
            catch (...) {
                LOG("[", getTimestamp(), "] TEXTURE: Skipped destroyed texture in ProcessPendingCreations");
            }
        }
        processed++;
    }
}


GW2_SCT::ImmutableTexture::ImmutableTexture(int width, int height)
    : Texture(width, height) {
}

GW2_SCT::ImmutableTexture* GW2_SCT::ImmutableTexture::Create(int width, int height, unsigned char* data) {
    return new ImmutableTextureD3D11(width, height, data);
}

void GW2_SCT::ImmutableTexture::Release(GW2_SCT::ImmutableTexture* tex) {
    if (!tex) return;
    auto* impl = static_cast<ImmutableTextureD3D11*>(tex);
    delete impl;
}

GW2_SCT::MutableTexture::MutableTexture(int width, int height)
    : Texture(width, height) {
}

GW2_SCT::MutableTexture* GW2_SCT::MutableTexture::Create(int width, int height) {
    return new MutableTextureD3D11(width, height);
}

void GW2_SCT::MutableTexture::Release(GW2_SCT::MutableTexture* tex) {
    if (!tex) return;
    auto* impl = static_cast<MutableTextureD3D11*>(tex);
    delete impl;
}

bool GW2_SCT::MutableTexture::startUpdate(ImVec2 pos, ImVec2 size, UpdateData* out) {
    if (_isCurrentlyUpdating) return false;
    bool ok = internalStartUpdate(pos, size, out);
    _isCurrentlyUpdating = ok;
    return ok;
}

bool GW2_SCT::MutableTexture::endUpdate() {
    if (!_isCurrentlyUpdating) return false;
    bool ok = internalEndUpdate();
    _isCurrentlyUpdating = false;
    return ok;
}
