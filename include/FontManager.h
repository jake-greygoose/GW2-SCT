#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <unordered_map>
#include <imgui.h>
#include "stb_truetype.h"
#include "Texture.h"

namespace GW2_SCT {

    class Glyph {
    public:
        static Glyph* GetGlyph(const stbtt_fontinfo* font, float scale, int codepoint, int ascent);
        static void cleanup();

        int   getX1();
        int   getX2();
        int   getY1();
        int   getY2();
        size_t getWidth();
        size_t getHeight();
        int   getCodepoint();
        int   getOffsetTop();
        float getLeftSideBearing();
        unsigned char* getBitmap();
        float getAdvanceAndKerning(int nextCodepoint);

    private:
        static std::unordered_map<const stbtt_fontinfo*, std::unordered_map<float, std::unordered_map<int, Glyph*>>> _knownGlyphs;

        Glyph(const stbtt_fontinfo* font, float scale, int codepoint, int ascent);
        ~Glyph();
        float getRealAdvanceAndKerning(int nextCodepoint);

        const stbtt_fontinfo* _font = nullptr;
        float  _scale = 0.f;
        int    _codepoint = 0;

        int    _x1 = 0, _y1 = 0, _x2 = 0, _y2 = 0;
        size_t _width = 0, _height = 0;
        int    _offsetTop = 0;

        int    _advance = 0;
        float  _lsb = 0.f;

        unsigned char* _bitmap = nullptr;
        std::unordered_map<int, float> _advanceAndKerningCache;
    };

    class FontType {
    public:
        FontType(unsigned char* data, size_t size);

        static void ensureAtlasCreation();
        static void cleanup();
        static void ProcessPendingAtlasUpdates();

        ImVec2 calcRequiredSpaceForTextAtSize(std::string text, float fontSize);
        void bakeGlyphsAtSize(std::string text, float fontSize);
        void drawAtSize(std::string text, float fontSize, ImVec2 position, ImU32 color);
#if _DEBUG
        void drawAtlas();
#endif

    private:
        stbtt_fontinfo _info{};
        int _ascent = 0, _descent = 0, _lineGap = 0;

        std::unordered_map<float, float> _cachedScales;
        std::unordered_map<float, bool>  _isCachedScaleExact;
        std::unordered_map<float, float> _cachedRealScales;

        float getCachedScale(float fontSize);
        bool  isCachedScaleExactForSize(float fontSize);
        float getRealScale(float fontSize);

        struct GlyphAtlas {
            struct GlyphAtlasLineDefinition {
                float lineHeight;
                ImVec2 nextGlyphPosition;
            };
            MutableTexture* texture = nullptr;
            std::vector<GlyphAtlasLineDefinition> linesWithSpaces;
            GlyphAtlas();
            ~GlyphAtlas();
        };

        struct GlyphPositionDefinition {
            size_t atlasID = 0;
            int x = 0, y = 0;
            ImVec2 uvStart{}, uvEnd{};
            Glyph* glyph = nullptr;

            GlyphPositionDefinition() = default;
            GlyphPositionDefinition(size_t atlasID, int x, int y, Glyph* glyph);

            ImVec2 getPos();
            ImVec2 getSize();
            ImVec2 getSize(float scale);
            ImVec2 offsetFrom(ImVec2 origin);
            ImVec2 offsetFrom(ImVec2 origin, float scale);
        };

        struct PendingAtlasUpdate {
            float scale;
            int codePoint;
            size_t atlasId;
            ImVec2 atlasPosition;
            Glyph* glyph;
        };

        std::unordered_map<float, std::unordered_map<int, GlyphPositionDefinition>> _glyphPositionsAtSizes;

        static std::vector<GlyphAtlas*> _allocatedAtlases;
        static std::mutex _allocatedAtlassesMutex;

        static std::vector<PendingAtlasUpdate> pendingAtlasUpdates;
        static std::mutex pendingAtlasUpdatesMutex;
    };

    class FontManager {
    public:
        static void init();
        static void cleanup();
        static FontType* getDefaultFont();

    private:
        static FontType* loadFontTypeFromFile(std::string path);
        static FontType* DEFAULT_FONT;
    };

}
