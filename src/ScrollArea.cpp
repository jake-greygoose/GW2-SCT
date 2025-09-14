#define _USE_MATH_DEFINES
#include "ScrollArea.h"
#include "imgui.h"
#include "Common.h"
#include "Options.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

using namespace std::chrono;

GW2_SCT::ScrollArea::ScrollArea(std::shared_ptr<scroll_area_options_struct> options) : options(options) {
	paintedMessages = std::list<std::pair<MessagePrerender, time_point<steady_clock>>>();
}

void GW2_SCT::ScrollArea::receiveMessage(std::shared_ptr<EventMessage> m) {
	if (!options->enabled) return;

	auto messageData = m->getCopyOfFirstData();
	if (!messageData) return;

	for (auto& receiver : options->receivers) {
		if (receiver->enabled && m->getCategory() == receiver->category && m->getType() == receiver->type) {

			std::string skillName = messageData->skillName ? std::string(messageData->skillName) : "";
			if (receiver->isSkillFiltered(messageData->skillId, skillName, Options::get()->filterManager)) {
				continue;
			}

			receiver->transient_showCombinedHitCount = options->showCombinedHitCount;
			receiver->transient_abbreviateSkillNames = options->abbreviateSkillNames;
			receiver->transient_numberShortPrecision = options->shortenNumbersPrecision;

			std::unique_lock<std::mutex> mlock(messageQueueMutex);
			
			if (!options->disableCombining && !messageQueue.empty()) {
				if (Options::get()->combineAllMessages) {
					for (auto it = messageQueue.rbegin(); it != messageQueue.rend(); ++it) {
						if (it->options == receiver && it->message->tryToCombineWith(m)) {
							if (!receiver->isThresholdExceeded(it->message, messageData->skillId, skillName, Options::get()->filterManager)) {
								it->update();
							} else {
								messageQueue.erase(std::next(it).base());
							}
							mlock.unlock();
							return;
						}
					}
				}
				else {
					auto backMessage = messageQueue.rbegin();
					if (backMessage->options == receiver && backMessage->message->tryToCombineWith(m)) {
						if (!receiver->isThresholdExceeded(backMessage->message, messageData->skillId, skillName, Options::get()->filterManager)) {
							backMessage->update();
						} else {
							messageQueue.pop_back();
						}
						mlock.unlock();
						return;
					}
				}
			}
			
			MessagePrerender preMessage = MessagePrerender(m, receiver);
			
			if (options->textCurve == TextCurve::ANGLED) {
				if (options->angledDirection == 0) {
					preMessage.angledSign = (angledMessageCounter % 2 == 0) ? 1 : -1;
					angledMessageCounter++;
				} else {
					preMessage.angledSign = (options->angledDirection > 0) ? 1 : -1;
				}
				
				float baseDegrees = options->angleDegrees;
				float jitterRange = options->angleJitterDegrees;
				float jitter = (std::rand() / (float)RAND_MAX * 2.0f - 1.0f) * jitterRange;
				float totalDegrees = baseDegrees + jitter;
				
				totalDegrees = std::max(0.0f, std::min(45.0f, totalDegrees));
				
				preMessage.angledAngleRad = totalDegrees * (M_PI / 180.0f);
			}
			
			if (preMessage.options != nullptr) {
				messageQueue.push_back(std::move(preMessage));
			}
			mlock.unlock();
			return;
		}
	}
}



void GW2_SCT::ScrollArea::paint() {
	std::unique_lock<std::mutex> mlock(messageQueueMutex);
	if (!options->enabled) { return; }
	
	const auto frameNow = std::chrono::steady_clock::now();
	double dt = 0.0;
	if (lastPaintTs.time_since_epoch().count() != 0) {
		dt = std::chrono::duration<double>(frameNow - lastPaintTs).count();
	}
	lastPaintTs = frameNow;
	
	int messagesInStack = std::max(1, Options::get()->messagesInStack);
	float queuePressure = std::max(0.0f, (float)messageQueue.size() - (float)messagesInStack) / (float)messagesInStack;
	queuePressure = std::clamp(queuePressure, 0.0f, 3.0f);
	float targetSpeedMultiplier = 1.0f + queuePressure * options->queueSpeedupFactor;
	
	if (dt > 0.0) {
		double alpha = 1.0 - std::exp(-dt / options->queueSpeedupSmoothingTau);
		currentSpeedMultiplier += alpha * (targetSpeedMultiplier - currentSpeedMultiplier);
	} else {
		currentSpeedMultiplier = targetSpeedMultiplier;
	}
	
	float scrollSpeedEff = getEffectiveScrollSpeed() * currentSpeedMultiplier;
	const float spacingEps = 0.5f;
	scrollPhasePx += scrollSpeedEff * (float)dt;

	while (!messageQueue.empty()) {
		MessagePrerender& m = messageQueue.front();
		
		if (m.options && m.message) {
			auto messageData = m.message->getCopyOfFirstData();
			std::string skillName = messageData && messageData->skillName ? std::string(messageData->skillName) : "";
			if (m.options->isThresholdExceeded(m.message, messageData ? messageData->skillId : 0, skillName, Options::get()->filterManager)) {
				messageQueue.pop_front();
				continue;
			}
		}

		if (paintedMessages.empty()) {
			paintedMessages.push_back(std::make_pair(std::move(m), frameNow));
			paintedMessages.back().first.spawnPhasePx = scrollPhasePx;
			messageQueue.pop_front();
			if (options->textCurve == TextCurve::STATIC) {
				handleStaticPlacement(paintedMessages.back().first);
			}
			break;
		}

		if (options->textCurve == TextCurve::STATIC) {
			m.ensureExtents();
			paintedMessages.push_back(std::make_pair(std::move(m), frameNow));
			paintedMessages.back().first.spawnPhasePx = scrollPhasePx;
			messageQueue.pop_front();
			handleStaticPlacement(paintedMessages.back().first);
			break;
		}

		m.ensureExtents();
		MessagePrerender& prev = paintedMessages.back().first;
		prev.ensureExtents();

		float spaceToClear = prev.messageHeight + options->minLineSpacingPx + spacingEps;
		float distPrev = scrollPhasePx - prev.spawnPhasePx;
		if (distPrev >= spaceToClear) {
			paintedMessages.push_back(std::make_pair(std::move(m), frameNow));
			paintedMessages.back().first.spawnPhasePx = scrollPhasePx;
			messageQueue.pop_front();
			if (paintedMessages.size() >= 2) {
				auto newIt = std::prev(paintedMessages.end());
				auto prevIt = std::prev(newIt);
				MessagePrerender& newMsg = newIt->first;
				MessagePrerender& prevMsg = prevIt->first;
				float distPrev2 = scrollPhasePx - prevMsg.spawnPhasePx;
				float actualGap = 0.0f;
				float prevTop2 = distPrev2 + prevMsg.messageTopInset + prevMsg.spawnYOffsetPx;
				float newBottom2 = newMsg.messageTopInset + newMsg.messageHeight + newMsg.spawnYOffsetPx;
				actualGap = prevTop2 - newBottom2;
				if (actualGap < options->minLineSpacingPx + spacingEps) {
					float missing = (options->minLineSpacingPx + spacingEps) - actualGap;
					newMsg.spawnYOffsetPx -= missing;
				}
			}
		} else {
			break;
		}
	}

	float areaOriginY = windowHeight * 0.5f + options->offsetY;
	float areaTop = areaOriginY;
	float areaHeight = options->height;
	float areaBottom = areaTop + areaHeight;
	
	float baseScrollSpeed = getEffectiveScrollSpeed();
	
	auto it = paintedMessages.begin();
	while (it != paintedMessages.end()) {
		__int64 t = duration_cast<milliseconds>(frameNow - it->second).count();
		if (paintMessage(it->first, t, currentSpeedMultiplier)) {
			it++;
		}
		else {
			it = paintedMessages.erase(it);
		}
	}
}

bool GW2_SCT::ScrollArea::paintMessage(MessagePrerender& m, __int64 time, float globalSpeedMultiplier) {
	if (m.forceExpire) {
		return false;
	}
	
    float alpha = 1.f;
    float fadeLength = 0.2f;	
    float percentage = 0.0f;
	
	if (options->textCurve == TextCurve::STATIC) {
		float effectiveTime = (float)time;
		percentage = effectiveTime / options->staticDisplayTimeMs;
		if (percentage > 1.f) return false;
		else if (percentage > 1.f - fadeLength) {
			alpha = 1.f - (percentage - 1.f + fadeLength) / fadeLength;
		}
	} else {
		// Use globally integrated phase for consistent spacing across speed changes
		float animatedHeight = scrollPhasePx - m.spawnPhasePx;
		percentage = animatedHeight / options->height;
		
		if (percentage > 1.f) return false;
		else if (percentage > 1.f - fadeLength) {
			alpha = 1.f - (percentage - 1.f + fadeLength) / fadeLength;
		}
	}

    if (m.prerenderNeeded) m.prerenderText();

    float globalOpacity = 1.0f;
    if (auto prof = GW2_SCT::Options::get()) {
        globalOpacity = std::clamp(prof->globalOpacity, 0.0f, 1.0f);
    }
    float areaOpacity = std::clamp(options->opacity, 0.0f, 1.0f);
    float finalOpacity = options->opacityOverrideEnabled ? areaOpacity : globalOpacity;
    float effectiveAlpha = std::clamp(alpha * finalOpacity, 0.0f, 1.0f);

	m.ensureExtents();
	float messageHeight = m.messageHeight;

	ImVec2 pos(windowWidth * 0.5f + options->offsetX,
		windowHeight * 0.5f + options->offsetY);

	if (options->textCurve == GW2_SCT::TextCurve::STATIC) {
		if (!std::isnan(m.staticY)) {
			pos.y += m.staticY;
		}
	} else {
		float animatedHeight = scrollPhasePx - m.spawnPhasePx;
		if (options->scrollDirection == ScrollDirection::DOWN) {
			pos.y += animatedHeight + m.spawnYOffsetPx;
		}
		else { // UP
			pos.y += options->height - animatedHeight - messageHeight + m.spawnYOffsetPx;
		}
	}
	
	float yDistanceForAngled = 0.0f;
	if (options->textCurve != TextCurve::STATIC) {
		float animatedHeight = scrollPhasePx - m.spawnPhasePx;
		yDistanceForAngled = animatedHeight;
		// Keep angled horizontal offset dependent only on travel distance
	}

	switch (options->textCurve) {
	case GW2_SCT::TextCurve::LEFT:
		pos.x += options->width * (2 * percentage - 1) * (2 * percentage - 1);
		break;
	case GW2_SCT::TextCurve::RIGHT:
		pos.x += options->width * (1 - (2 * percentage - 1) * (2 * percentage - 1));
		break;
	case GW2_SCT::TextCurve::STRAIGHT:
		break;
	case GW2_SCT::TextCurve::STATIC:
		break;
	case GW2_SCT::TextCurve::ANGLED:
		if (m.angledSign != 0) {
			float xOffset = yDistanceForAngled * std::tan(m.angledAngleRad) * m.angledSign;
			pos.x += xOffset;
		}
		break;
	}

	if (options->textAlign != TextAlign::LEFT) {
		switch (options->textAlign) {
		case GW2_SCT::TextAlign::CENTER:
			pos.x -= 0.5f * m.interpretedTextWidth;
			break;
		case GW2_SCT::TextAlign::RIGHT:
			pos.x -= m.interpretedTextWidth;
			break;
		default: break;
		}
	}

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    for (TemplateInterpreter::InterpretedText text : m.interpretedText) {
        ImVec2 curPos = ImVec2(pos.x + text.offset.x, pos.y + text.offset.y);
        if (text.icon == nullptr) {
            ImU32 blackWithAlpha = ImGui::GetColorU32(ImVec4(0, 0, 0, effectiveAlpha));
            if (GW2_SCT::Options::get()->dropShadow) {
                m.font->drawAtSize(text.str, m.fontSize, ImVec2(curPos.x + 2, curPos.y + 2), blackWithAlpha);
            }
            ImU32 actualCol = text.color & ImGui::GetColorU32(ImVec4(1, 1, 1, effectiveAlpha));
            m.font->drawAtSize(text.str, m.fontSize, curPos, actualCol);
        }
        else {
            text.icon->draw(curPos, text.size, ImGui::GetColorU32(ImVec4(1, 1, 1, effectiveAlpha)));
        }
    }

	return true;
}

void GW2_SCT::ScrollArea::applySimpleSpacing(float globalSpeedMultiplier, double dt, std::chrono::time_point<std::chrono::steady_clock> frameNow) {
	if (options->minLineSpacingPx <= 0.0f) return;
	if (paintedMessages.size() < 2) return;
	
	float baseScrollSpeed = getEffectiveScrollSpeed() * globalSpeedMultiplier;
	if (baseScrollSpeed <= 0.0f) return;
	
	auto it = paintedMessages.begin();
	while (it != paintedMessages.end()) {
		auto nextIt = std::next(it);
		if (nextIt == paintedMessages.end()) break;
		
		it->first.ensureExtents();
		nextIt->first.ensureExtents();
		
		float baseTime1 = (float)std::chrono::duration_cast<std::chrono::milliseconds>(frameNow - it->second).count();
		float baseTime2 = (float)std::chrono::duration_cast<std::chrono::milliseconds>(frameNow - nextIt->second).count();
		
		float elapsed1 = std::max(0.0f, baseTime1 - it->first.liveOffsetMs);
		float elapsed2 = std::max(0.0f, baseTime2 - nextIt->first.liveOffsetMs);
		
		float pos1, pos2, gap;
		if (options->scrollDirection == ScrollDirection::DOWN) {
			pos1 = elapsed1 * 0.001f * baseScrollSpeed + it->first.messageTopInset + it->first.messageHeight;
			pos2 = elapsed2 * 0.001f * baseScrollSpeed + nextIt->first.messageTopInset;
			gap = pos2 - pos1;
		} else {
			pos1 = elapsed1 * 0.001f * baseScrollSpeed + it->first.messageTopInset;
			pos2 = elapsed2 * 0.001f * baseScrollSpeed + nextIt->first.messageTopInset + nextIt->first.messageHeight;
			gap = pos1 - pos2;
		}
		
		if (gap < options->minLineSpacingPx) {
			float neededGap = options->minLineSpacingPx - gap;
			float offsetMs = (neededGap / baseScrollSpeed) * 1000.0f;
			nextIt->first.liveOffsetMs += std::min(offsetMs, 100.0f);
		}
		
		++it;
	}
}

void GW2_SCT::ScrollArea::handleStaticPlacement(MessagePrerender& m) {
	if (options->textCurve != TextCurve::STATIC) return;
	if (!std::isnan(m.staticY)) return;
	
	m.ensureExtents();
	
	MessagePrerender* latestStatic = nullptr;
	for (auto it = paintedMessages.rbegin(); it != paintedMessages.rend(); ++it) {
		if (!std::isnan(it->first.staticY)) {
			latestStatic = &it->first;
			break;
		}
	}
	
	if (latestStatic == nullptr) {
		if (options->scrollDirection == ScrollDirection::DOWN) {
			m.staticY = options->height - m.messageHeight;
		} else {
			m.staticY = 0.0f;
		}
	} else {
		if (options->scrollDirection == ScrollDirection::DOWN) {
			m.staticY = latestStatic->staticY - latestStatic->messageHeight - options->minLineSpacingPx;
			if (m.staticY < 0) {
				m.staticY = options->height - m.messageHeight;
			}
		} else {
			m.staticY = latestStatic->staticY + latestStatic->messageHeight + options->minLineSpacingPx;
			if (m.staticY + m.messageHeight > options->height) {
				m.staticY = 0.0f;
			}
		}
	}
	
	float newTop = m.staticY;
	float newBottom = m.staticY + m.messageHeight;
	
	for (auto it = paintedMessages.begin(); it != paintedMessages.end();) {
		if (&it->first == &m) {
			++it;
			continue;
		}
		
		if (!std::isnan(it->first.staticY)) {
			float oldTop = it->first.staticY;
			float oldBottom = it->first.staticY + it->first.messageHeight;
			
			bool intersects = !(newBottom <= oldTop || newTop >= oldBottom);
			if (intersects) {
				it = paintedMessages.erase(it);
				continue;
			}
		}
		++it;
	}
}


GW2_SCT::ScrollArea::MessagePrerender::MessagePrerender(std::shared_ptr<EventMessage> message, std::shared_ptr<message_receiver_options_struct> options)
	: message(message), options(options) {
	category = message->getCategory();
	type = message->getType();
	update();
	templateObserverId = options->outputTemplate.onAssign([this](const std::string& oldVal, const std::string& newVal) { this->update(); });
	colorObserverId = options->color.onAssign([this](const std::string& oldVal, const std::string& newVal) { this->update(); });
	fontObserverId = options->font.onAssign([this](const FontId& oldVal, const FontId& newVal) { this->update(); });
	fontSizeObserverId = options->fontSize.onAssign([this](const float& oldVal, const float& newVal) { this->update(); });
}

GW2_SCT::ScrollArea::MessagePrerender::MessagePrerender(const MessagePrerender& copy) {
	message = copy.message;
	options = copy.options;
	category = copy.category;
	type = copy.type;
	if (copy.prerenderNeeded) {
		update();
	}
	else {
		str = copy.str;
		font = copy.font;
		fontSize = copy.fontSize;
		options = copy.options;
		interpretedText = std::vector<TemplateInterpreter::InterpretedText>(copy.interpretedText);
		interpretedTextWidth = copy.interpretedTextWidth;
		prerenderNeeded = copy.prerenderNeeded;
	}
	
	angledSign = copy.angledSign;
	angledAngleRad = copy.angledAngleRad;
	spawnYOffsetPx = copy.spawnYOffsetPx;
	spawnPhasePx = copy.spawnPhasePx;
	
	templateObserverId = options->outputTemplate.onAssign([this](const std::string& oldVal, const std::string& newVal) { this->update(); });
	colorObserverId = options->color.onAssign([this](const std::string& oldVal, const std::string& newVal) { this->update(); });
	fontObserverId = options->font.onAssign([this](const FontId& oldVal, const FontId& newVal) { this->update(); });
	fontSizeObserverId = options->fontSize.onAssign([this](const float& oldVal, const float& newVal) { this->update(); });
}

GW2_SCT::ScrollArea::MessagePrerender::~MessagePrerender() {
	options->outputTemplate.removeOnAssign(templateObserverId);
	options->color.removeOnAssign(colorObserverId);
	options->font.removeOnAssign(fontObserverId);
	options->fontSize.removeOnAssign(fontSizeObserverId);
}

void GW2_SCT::ScrollArea::MessagePrerender::update() {
	if (message.get() == nullptr) {
		LOG("ERROR: calling update on pre-render without message");
		str = "";
		prerenderNeeded = false;
		return;
	}
	str = message->getStringForOptions(options);
	font = getFontType(options->font);
	fontSize = options->fontSize;
	if (fontSize < 0) {
		if (floatEqual(fontSize, -1.f)) fontSize = GW2_SCT::Options::get()->defaultFontSize;
		else if (floatEqual(fontSize, -2.f)) fontSize = GW2_SCT::Options::get()->defaultCritFontSize;
	}
	font->bakeGlyphsAtSize(str, fontSize);
	prerenderNeeded = true;
	messageHeight = -1.0f;
	messageTopInset = 0.0f;
}

void GW2_SCT::ScrollArea::MessagePrerender::prerenderText() {
	if (options != nullptr) {
		interpretedText = TemplateInterpreter::interpret(font, fontSize, stoc(options->color), str);
	}
	else {
		interpretedText = {};
	}
	if (interpretedText.size() > 0) {
		interpretedTextWidth = interpretedText.back().offset.x + interpretedText.back().size.x;
	}
	prerenderNeeded = false;
}

void GW2_SCT::ScrollArea::MessagePrerender::ensureExtents() {
	if (messageHeight > 0.0f && !prerenderNeeded) return;
	
	if (prerenderNeeded) prerenderText();
	
	if (!interpretedText.empty()) {
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		
		for (const auto& chunk : interpretedText) {
			float chunkTop = chunk.offset.y;
			float chunkBottom = chunk.offset.y + chunk.size.y;
			minY = std::min(minY, chunkTop);
			maxY = std::max(maxY, chunkBottom);
		}
		
		if (minY != std::numeric_limits<float>::max()) {
			messageTopInset = minY;
			messageHeight = maxY - minY;
		} else {
			messageTopInset = 0.0f;
			messageHeight = getTextSize(str.c_str(), font, fontSize).y;
		}
	} else {
		messageTopInset = 0.0f;
		messageHeight = getTextSize(str.c_str(), font, fontSize).y;
	}
}

float GW2_SCT::ScrollArea::getEffectiveScrollSpeed() const {
	return (options->customScrollSpeed > 0.0f) ? options->customScrollSpeed : Options::get()->scrollSpeed;
}
