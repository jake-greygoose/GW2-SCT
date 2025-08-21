#include "Texture.h"
#include "Common.h"
#include <queue>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>


// Global queue for pending texture creations
namespace GW2_SCT {
    static std::queue<Texture*> pendingTextureCreations;
    static std::mutex textureCreationMutex;
    static bool isInPresentCycle = false;
}

GW2_SCT::Texture::Texture(int width, int height) : _textureWidth(width), _textureHeight(height), _nextCreationTry(std::chrono::system_clock::now()) {}

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
    isInPresentCycle = true;
    
    #if _DEBUG
        if (!pendingTextureCreations.empty()) {
            LOG("[", getTimestamp(), "] TEXTURE: BeginPresentCycle - safe period started (", pendingTextureCreations.size(), " pending)");
        }
    #endif 
}

void GW2_SCT::Texture::EndPresentCycle() {
    isInPresentCycle = false;

    // Process all pending texture creations
    std::lock_guard<std::mutex> lock(textureCreationMutex);
    int processed = 0;
    
    #if _DEBUG
        int queueSize = pendingTextureCreations.size();

        if (queueSize > 0) {
            LOG("[", getTimestamp(), "] TEXTURE: EndPresentCycle - processing ", queueSize, " pending textures");
        }
    #endif

    while (!pendingTextureCreations.empty()) {
        Texture* texture = pendingTextureCreations.front();
        pendingTextureCreations.pop();

        if (!texture) {
            processed++;
            continue;
        }

        try {
            texture->tryCreateNow();
            processed++;
        }
        catch (...) {
            LOG("[", getTimestamp(), "] TEXTURE: Skipped destroyed texture in EndPresentCycle");
            processed++;
        }
    }

    #if _DEBUG
        if (processed > 0) {
            LOG("[", getTimestamp(), "] TEXTURE: EndPresentCycle processed ", processed, " textures");
        }
    #endif
}

void GW2_SCT::Texture::ProcessPendingCreations() {
    std::lock_guard<std::mutex> lock(textureCreationMutex);

    // Process a limited number of textures per frame to avoid stalling
    const int maxTexturesPerFrame = 5;
    int processed = 0;
    int queueSize = pendingTextureCreations.size();

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

GW2_SCT::ImmutableTexture* GW2_SCT::ImmutableTexture::Create(int width, int height, unsigned char* data) {
    if (d3Device11 != nullptr) {
        return new ImmutableTextureD3D11(width, height, data);
    }
    return nullptr;
}

void GW2_SCT::ImmutableTexture::Release(ImmutableTexture* tex) {
    if (tex == nullptr) return;
    if (d3Device11 != nullptr) {
        ImmutableTextureD3D11* res = (ImmutableTextureD3D11*)tex;
        delete res;
    }
}

GW2_SCT::ImmutableTexture::ImmutableTexture(int width, int height) : Texture(width, height) {}

GW2_SCT::MutableTexture* GW2_SCT::MutableTexture::Create(int width, int height) {
    if (d3Device11 != nullptr) {
        return new MutableTextureD3D11(width, height);
    }
    return nullptr;
}

void GW2_SCT::MutableTexture::Release(MutableTexture* tex) {
    if (tex == nullptr) return;
    if (d3Device11 != nullptr) {
        MutableTextureD3D11* res = (MutableTextureD3D11*)tex;
        delete res;
    }
}

GW2_SCT::MutableTexture::MutableTexture(int width, int height) : Texture(width, height) {}

bool GW2_SCT::MutableTexture::startUpdate(ImVec2 pos, ImVec2 size, UpdateData* out) {
    if (_isCurrentlyUpdating || !_created) return false;
    _isCurrentlyUpdating = true;
    if (internalStartUpdate(pos, size, out)) {
        return true;
    }
    else {
        _isCurrentlyUpdating = false;
        return false;
    }
}

bool GW2_SCT::MutableTexture::endUpdate() {
    if (!_isCurrentlyUpdating) return false;
    _isCurrentlyUpdating = false;
    if (internalEndUpdate()) {
        return true;
    }
    else {
        _isCurrentlyUpdating = true;
        return false;
    }
}