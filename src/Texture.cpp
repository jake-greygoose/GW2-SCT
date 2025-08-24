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
    static bool isInPresentCycle = false;
}

/* ---------------------------
   Texture base
----------------------------*/
GW2_SCT::Texture::Texture(int width, int height)
    : _textureWidth(width), _textureHeight(height),
    _nextCreationTry(std::chrono::system_clock::now()) {
}

void GW2_SCT::Texture::draw(ImVec2 pos, ImVec2 size, ImU32 color) {
    return draw(pos, size, ImVec2(0, 0), ImVec2(1, 1), color);
}

void GW2_SCT::Texture::draw(ImVec2 pos, ImVec2 size, ImVec2 uvStart, ImVec2 uvEnd, ImU32 color) {
    if (!_created && std::chrono::system_clock::now() > _nextCreationTry) {
#if _DEBUG
        LOG("[", getTimestamp(), "] TEXTURE: Requesting creation (deferred)");
#endif
        requestCreation();
    }

    if (_created) {
        internalDraw(pos, size, uvStart, uvEnd, color);
    }
    else {
#if _DEBUG
        static int skipCounter = 0;
        if (skipCounter++ % 100 == 0) {
            LOG("[", getTimestamp(), "] TEXTURE: Skipping draw - not created yet (", skipCounter, " times)");
        }
#endif
    }
}

void GW2_SCT::Texture::ensureCreation() {
    if (!_created && std::chrono::system_clock::now() > _nextCreationTry) {
#if _DEBUG
        LOG("[", getTimestamp(), "] TEXTURE: ensureCreation() requesting deferred creation");
#endif
        requestCreation();
    }
}

void GW2_SCT::Texture::requestCreation() {
    if (_creationRequested) return;
#if _DEBUG
    LOG("[", getTimestamp(), "] TEXTURE: Adding to pending queue for deferred creation");
#endif
    std::lock_guard<std::mutex> lock(textureCreationMutex);
    pendingTextureCreations.push(this);
    _creationRequested = true;
}

void GW2_SCT::Texture::tryCreateNow() {
    if (_created || !_creationRequested) return;

#if _DEBUG
    LOG("[", getTimestamp(), "] TEXTURE: *** ACTUALLY CREATING NOW *** (should be safe period)");
#endif

    _created = internalCreate();
    _creationRequested = false;

    if (!_created) {
        _creationTries++;
        _nextCreationTry = std::chrono::system_clock::now() + std::chrono::seconds(_creationTries);
        LOG("[", getTimestamp(), "] TEXTURE: Creation failed, will retry");
    }
    else {
#if _DEBUG
        LOG("[", getTimestamp(), "] TEXTURE: *** CREATION SUCCEEDED ***");
#endif
    }
}

void GW2_SCT::Texture::BeginPresentCycle() {
    // Present() is our guaranteed render thread; mark it.
    TextureD3D11::MarkRenderThread();
    isInPresentCycle = true;

#if _DEBUG
    if (!pendingTextureCreations.empty()) {
        LOG("[", getTimestamp(), "] TEXTURE: BeginPresentCycle - will process ",
            (int)pendingTextureCreations.size(), " pending textures");
    }
#endif

    // *** CREATE ONLY HERE (inside Present on the render thread) ***
    ProcessPendingCreations();
}

void GW2_SCT::Texture::EndPresentCycle() {
    // No creation here anymore — we only create in BeginPresentCycle()
    isInPresentCycle = false;
}

void GW2_SCT::Texture::ProcessPendingCreations() {
    std::lock_guard<std::mutex> lock(textureCreationMutex);

    // Only create on the render thread
    if (!TextureD3D11::IsOnRenderThread()) return;

    const int maxTexturesPerFrame = 5;
    int processed = 0;
    int queueSize = (int)pendingTextureCreations.size();

#if _DEBUG
    if (queueSize > 0) {
        LOG("[", getTimestamp(), "] TEXTURE: ProcessPendingCreations - queue size: ", queueSize);
    }
#endif

    while (!pendingTextureCreations.empty() && processed < maxTexturesPerFrame) {
        Texture* texture = pendingTextureCreations.front();
        pendingTextureCreations.pop();

        if (!texture) {
            processed++;
            continue;
        }

        try {
            texture->tryCreateNow();
        }
        catch (...) {
            LOG("[", getTimestamp(), "] TEXTURE: Skipped destroyed texture in ProcessPendingCreations");
        }

        processed++;
    }

#if _DEBUG
    if (processed > 0) {
        LOG("[", getTimestamp(), "] TEXTURE: ProcessPendingCreations processed ", processed, " textures");
    }
#endif
}

bool GW2_SCT::Texture::IsInPresentCycle() { return isInPresentCycle; }

/* ---------------------------
   ImmutableTexture base
----------------------------*/
GW2_SCT::ImmutableTexture::ImmutableTexture(int width, int height)
    : Texture(width, height) {
}

/* Factory: return D3D11 impl */
GW2_SCT::ImmutableTexture* GW2_SCT::ImmutableTexture::Create(int width, int height, unsigned char* data) {
    return new ImmutableTextureD3D11(width, height, data);
}

/* Factory release: delete concrete type created above */
void GW2_SCT::ImmutableTexture::Release(GW2_SCT::ImmutableTexture* tex) {
    if (!tex) return;
    auto* impl = static_cast<ImmutableTextureD3D11*>(tex);
    delete impl;
}

/* ---------------------------
   MutableTexture base
----------------------------*/
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

/* Wrap virtual internals with a guard so callers don't touch D3D11 directly */
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
