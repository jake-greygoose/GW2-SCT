#pragma once
#include "Texture.h"
#include "OptionsStructures.h"
#include <memory>
#include <queue>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <imgui.h>
#include <atomic>
namespace GW2_SCT {
	class SkillIcon {
	public:
		ImVec2 draw(ImVec2 pos, ImVec2 size, ImU32 color = 0xFFFFFFFF);
		SkillIcon(std::shared_ptr<std::vector<BYTE>> fileData, uint32_t skillID);
		~SkillIcon();

		static void ProcessPendingIconTextures();

	private:
		void createTextureNow(SkillIconDisplayType displayType);
		void requestTextureCreation(SkillIconDisplayType displayType);

		std::unordered_map<SkillIconDisplayType, ImmutableTexture*> textures;
		std::shared_ptr<std::vector<BYTE>> fileData;
		uint32_t skillID;
		std::unordered_map<SkillIconDisplayType, bool> texturesCreated = {};
		std::unordered_map<SkillIconDisplayType, bool> textureCreationRequested = {};
	};
	class SkillIconManager {
	public:
		static void init();
		static void cleanup();
		static SkillIcon* getIcon(uint32_t skillID);
	private:
		static void internalInit();
		static void spawnLoadThread();
		static void loadThreadCycle();
		static int requestsPerMinute;
		// Replaces the old object_threadsafe safe_ptr wrappers with explicit locking
		static std::shared_mutex dataMutex;
		static std::unordered_map<uint32_t, std::pair<std::string, std::string>> staticFiles;
		static std::unordered_map<uint32_t, bool> checkedIDs;
		static std::list<uint32_t> requestedIDs;
		static std::unordered_map<uint32_t, SkillIcon> loadedIcons;
		static std::unordered_set<uint32_t> embeddedIconIds;
		static std::thread loadThread;
		static std::atomic<bool> keepLoadThreadRunning;
		static long skillIconsEnabledCallbackId;
		static long preloadAllSkillIconsId;
	};
}
