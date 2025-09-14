#pragma once
#include <deque>
#include <list>
#include <chrono>
#include <memory>
#include <mutex>
#include <limits>
#include "OptionsStructures.h"
#include "Message.h"
#include "TemplateInterpreter.h"

namespace GW2_SCT {
	class ScrollArea {
	public:
		ScrollArea(std::shared_ptr<scroll_area_options_struct> options);
		void receiveMessage(std::shared_ptr<EventMessage> m);
		void paint();
		std::shared_ptr<scroll_area_options_struct> getOptions() { return options; }
	private:
		struct MessagePrerender {
			std::shared_ptr<EventMessage> message;
			std::string str;
			FontType* font;
			float fontSize;
			MessageCategory category;
			MessageType type;
			std::shared_ptr<message_receiver_options_struct> options;
			std::vector<TemplateInterpreter::InterpretedText> interpretedText;
			float interpretedTextWidth;
			bool prerenderNeeded = true;
			long templateObserverId, colorObserverId, fontObserverId, fontSizeObserverId;
			
			float liveOffsetMs = 0.0f;
			// One-shot vertical offset applied at spawn to guarantee min spacing
			float spawnYOffsetPx = 0.0f;
			// Phase position in pixels at spawn (for globally-integrated motion)
			float spawnPhasePx = 0.0f;
			float staticY = std::numeric_limits<float>::quiet_NaN();
			float messageHeight = 0.0f;
			float messageTopInset = 0.0f;
			bool forceExpire = false;
				
			// Angled animation fields
			int angledSign = 0;
			float angledAngleRad = 0.0f;
		public:
			MessagePrerender(std::shared_ptr<EventMessage> message, std::shared_ptr<message_receiver_options_struct> options);
			MessagePrerender(const MessagePrerender& copy);
			~MessagePrerender();
			void update();
			void prerenderText();
			void ensureExtents();
		};

		std::mutex messageQueueMutex;
		std::deque<MessagePrerender> messageQueue = std::deque<MessagePrerender>();

		bool paintMessage(MessagePrerender& m, __int64 time, float globalSpeedMultiplier);
		float getEffectiveScrollSpeed() const;
		void applySimpleSpacing(float globalSpeedMultiplier, double dt, std::chrono::time_point<std::chrono::steady_clock> frameNow);
		void handleStaticPlacement(MessagePrerender& m);

		std::list<std::pair<MessagePrerender, std::chrono::time_point<std::chrono::steady_clock>>> paintedMessages;

		std::chrono::time_point<std::chrono::steady_clock> lastPaintTs = {};
		
		int angledMessageCounter = 0;
		
		// Smooth speed multiplier for less jarring transitions
		float currentSpeedMultiplier = 1.0f;
		// Globally integrated scroll phase in pixels (monotonic)
		float scrollPhasePx = 0.0f;

		std::shared_ptr<scroll_area_options_struct> options;
	};
}
