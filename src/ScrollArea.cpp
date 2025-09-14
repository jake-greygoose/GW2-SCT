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
	paintedMessages = std::list<std::pair<MessagePrerender, time_point<system_clock>>>();
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
	
	const auto frameNow = std::chrono::system_clock::now();
	double dt = 0.0;
	if (lastPaintTs.time_since_epoch().count() != 0) {
		dt = std::chrono::duration<double>(frameNow - lastPaintTs).count();
	}
	lastPaintTs = frameNow;
	
	float speedFactor = prevSpeedFactor;
	float scrollSpeedEff = getEffectiveScrollSpeed() * speedFactor;

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
			m.personalFactor = speedFactor;
			paintedMessages.push_back(std::make_pair(std::move(m), frameNow));
			messageQueue.pop_front();
			
			if (options->textCurve == TextCurve::STATIC) {
				handleStaticPlacement(paintedMessages.back().first);
			}
			break;
		}
		else {
			m.ensureExtents();
			float spaceRequiredForNextMessage = m.messageHeight + options->minLineSpacingPx;
			float msForSpaceToClear = spaceRequiredForNextMessage / scrollSpeedEff * 1000;
			__int64 msSinceLastPaintedMessage = duration_cast<milliseconds>(frameNow - paintedMessages.back().second).count();
			__int64 msUntilNextMessageCanBePainted = (__int64)msForSpaceToClear - msSinceLastPaintedMessage;
			if (msUntilNextMessageCanBePainted > 0) {
				bool highBacklog = messageQueue.size() > Options::get()->messagesInStack;
				bool areaBusy = prevSpeedFactor > 1.0f;
				if (highBacklog || areaBusy) {
					m.personalFactor = speedFactor;
					paintedMessages.push_back(std::make_pair(std::move(m), frameNow));
					messageQueue.pop_front();
					
					if (options->textCurve == TextCurve::STATIC) {
						handleStaticPlacement(paintedMessages.back().first);
					}
				}
				break;
			}
			else {
				m.personalFactor = speedFactor;
				paintedMessages.push_back(std::make_pair(std::move(m), frameNow));
				messageQueue.pop_front();
				
				if (options->textCurve == TextCurve::STATIC) {
					handleStaticPlacement(paintedMessages.back().first);
				}
				
				break;
			}
		}
	}

	if (options->textCurve != TextCurve::STATIC) {
		applyLiveSpacing(dt, frameNow);
		applyHardSpacing(dt, frameNow);
	}

	if (dt > 0.0 && !paintedMessages.empty()) {
		float areaOriginY = windowHeight * 0.5f + options->offsetY;
		float areaTop = areaOriginY;
		float areaHeight = options->height;
		
		float fillTop = std::numeric_limits<float>::max();
		float fillBot = std::numeric_limits<float>::lowest();
		
		float baseScrollSpeed = getEffectiveScrollSpeed();
		
		for (auto& pair : paintedMessages) {
			if (std::isnan(pair.first.staticY)) {
				float baseTime = (float)std::chrono::duration_cast<std::chrono::milliseconds>(frameNow - pair.second).count();
				float effectiveElapsedMs = std::max(0.0f, baseTime - pair.first.liveOffsetMs);
				
				pair.first.ensureExtents(); // MUST be first
				float messageY, messageTop, messageBot;
				if (options->scrollDirection == ScrollDirection::DOWN) {
					messageY = areaOriginY + effectiveElapsedMs * 0.001f * (baseScrollSpeed * pair.first.personalFactor);
					messageTop = messageY + pair.first.messageTopInset;
					messageBot = messageTop + pair.first.messageHeight;
				} else { // UP
					float animatedHeight = effectiveElapsedMs * 0.001f * (baseScrollSpeed * pair.first.personalFactor);
					messageY = areaOriginY + areaHeight - animatedHeight - pair.first.messageHeight;
					messageTop = messageY + pair.first.messageTopInset;
					messageBot = messageTop + pair.first.messageHeight;
				}
				
				fillTop = std::min(fillTop, messageTop);
				fillBot = std::max(fillBot, messageBot);
			}
		}
		
		float occupancy = 0.0f;
		if (fillTop != std::numeric_limits<float>::max()) {
			if (options->scrollDirection == ScrollDirection::DOWN) {
				occupancy = std::clamp((fillBot - areaTop) / areaHeight, 0.0f, 1.0f);
			} else { // UP
				float areaBottom = areaTop + areaHeight;
				occupancy = std::clamp((areaBottom - fillTop) / areaHeight, 0.0f, 1.0f);
			}
		}
		
		double alpha = 1.0 - std::exp(-dt / options->overflowSmoothingTau);
		occupancyEma += alpha * (occupancy - occupancyEma);
		
		float x = 0.0f;
		if (occupancyEma > options->occupancyStart) {
			x = (float(occupancyEma) - options->occupancyStart) / std::max(0.001f, options->occupancyEnd - options->occupancyStart);
			x = std::clamp(x, 0.0f, 1.0f);
			x = x * x * (3.0f - 2.0f * x);
		}
		float desiredFactor = 1.0f + x * (options->overflowMaxFactor - 1.0f);
		
		float maxDeltaPerSec = 1.0f;
		float maxStep = maxDeltaPerSec * (float)dt;
		float nextSpeedFactor = std::clamp(desiredFactor, prevSpeedFactor - maxStep, prevSpeedFactor + maxStep);
		prevSpeedFactor = nextSpeedFactor;
	} else {
		prevSpeedFactor = 1.0f;
		occupancyEma = 0.0;
	}

	float areaOriginY = windowHeight * 0.5f + options->offsetY;
	float areaTop = areaOriginY;
	float areaHeight = options->height;
	float areaBottom = areaTop + areaHeight;
	
	float baseScrollSpeed = getEffectiveScrollSpeed();
	
	for (auto& pair : paintedMessages) {
		if (!std::isnan(pair.first.staticY)) continue;
		
		float target = prevSpeedFactor;
		float accelUpPerSec = 1.0f;  
		float decelDownPerSec = 0.8f;
		
		pair.first.ensureExtents();
		
		float baseTime = (float)std::chrono::duration_cast<std::chrono::milliseconds>(frameNow - pair.second).count();
		float effectiveElapsedMs = std::max(0.0f, baseTime - pair.first.liveOffsetMs);
		
		float msgTop, msgBottom;
		if (options->scrollDirection == ScrollDirection::DOWN) {
			float messageY = areaOriginY + effectiveElapsedMs * 0.001f * baseScrollSpeed * pair.first.personalFactor;
			msgTop = messageY + pair.first.messageTopInset;
			msgBottom = msgTop + pair.first.messageHeight;
			
			float distToExitPx = areaBottom - msgBottom;
			if (distToExitPx < 0.12f * areaHeight && target < pair.first.personalFactor) {
				target = pair.first.personalFactor;
			}
		} else { // UP
			float animatedHeight = effectiveElapsedMs * 0.001f * baseScrollSpeed * pair.first.personalFactor;
			float messageY = areaOriginY + areaHeight - animatedHeight - pair.first.messageHeight;
			msgTop = messageY + pair.first.messageTopInset;
			msgBottom = msgTop + pair.first.messageHeight;
			
			float distToExitPx = msgTop - areaTop;
			if (distToExitPx < 0.12f * areaHeight && target < pair.first.personalFactor) {
				target = pair.first.personalFactor;
			}
		}
		
		float delta = target - pair.first.personalFactor;
		float limit = (delta > 0 ? accelUpPerSec : decelDownPerSec) * (float)dt;
		pair.first.personalFactor += std::clamp(delta, -limit, limit);
	}

	auto it = paintedMessages.begin();
	while (it != paintedMessages.end()) {
		__int64 t = duration_cast<milliseconds>(frameNow - it->second).count();
		if (paintMessage(it->first, t)) {
			it++;
		}
		else {
			it = paintedMessages.erase(it);
		}
	}
}

bool GW2_SCT::ScrollArea::paintMessage(MessagePrerender& m, __int64 time) {
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
		float effectiveElapsedMs = std::max(0.0f, (float)time - m.liveOffsetMs);
		float baseScrollSpeed = getEffectiveScrollSpeed();
		float animatedHeight = effectiveElapsedMs * 0.001f * (baseScrollSpeed * m.personalFactor);
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
		float effectiveElapsedMs = std::max(0.0f, (float)time - m.liveOffsetMs);
		float baseScrollSpeed = getEffectiveScrollSpeed();
		float animatedHeight = effectiveElapsedMs * 0.001f * (baseScrollSpeed * m.personalFactor);
		
		if (options->scrollDirection == ScrollDirection::DOWN) {
			pos.y += animatedHeight;
		}
		else { // UP
			pos.y += options->height - animatedHeight - messageHeight;
		}
	}
	
	float yDistanceForAngled = 0.0f;
	if (options->textCurve != TextCurve::STATIC) {
		float effectiveElapsedMs = std::max(0.0f, (float)time - m.liveOffsetMs);
		float baseScrollSpeed = getEffectiveScrollSpeed();
		yDistanceForAngled = effectiveElapsedMs * 0.001f * (baseScrollSpeed * m.personalFactor);
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

void GW2_SCT::ScrollArea::applyLiveSpacing(double dt, std::chrono::time_point<std::chrono::system_clock> frameNow) {
	if (options->textCurve == TextCurve::STATIC) return;
	if (options->minLineSpacingPx <= 0.0f) return;
	if (paintedMessages.size() < 2) return;
	if (options->maxNudgeMsPerSecond <= 0.0f) return;
	
	if (getEffectiveScrollSpeed() <= 0.0f) return;
	
	std::vector<std::pair<MessagePrerender*, std::chrono::time_point<std::chrono::system_clock>*>> messages;
	messages.reserve(paintedMessages.size());
	for (auto& pair : paintedMessages) {
		messages.push_back({&pair.first, &pair.second});
	}
	
	float baseScrollSpeed = getEffectiveScrollSpeed();
	float travelMsBase = options->height / baseScrollSpeed * 1000.0f;
	
	for (int pass = 0; pass < 4; pass++) {
		bool anyAdjustment = false;
		
		for (size_t i = 0; i < messages.size() - 1; i++) {
			auto& older = *messages[i].first;
			auto& newer = *messages[i + 1].first;
			
			if (std::isnan(older.staticY) == false || std::isnan(newer.staticY) == false) {
				continue;
			}
			
			float olderBaseTime = (float)std::chrono::duration_cast<std::chrono::milliseconds>(frameNow - *messages[i].second).count();
			float newerBaseTime = (float)std::chrono::duration_cast<std::chrono::milliseconds>(frameNow - *messages[i + 1].second).count();
			
			float olderElapsed = std::max(0.0f, olderBaseTime - older.liveOffsetMs);
			float newerElapsed = std::max(0.0f, newerBaseTime - newer.liveOffsetMs);
			
			older.ensureExtents();
			newer.ensureExtents();
			
			float personalOlder = baseScrollSpeed * older.personalFactor;
			float personalNewer = baseScrollSpeed * newer.personalFactor;
			float olderY, newerY;
			if (options->scrollDirection == ScrollDirection::DOWN) {
				olderY = olderElapsed * 0.001f * personalOlder;
				newerY = newerElapsed * 0.001f * personalNewer;
				
				float olderTop = olderY + older.messageTopInset;
				float newerBottom = newerY + newer.messageTopInset + newer.messageHeight;
				float gap = olderTop - newerBottom;
				
				if (gap < (options->minLineSpacingPx - 1.0f)) {
					float needPx = (options->minLineSpacingPx - gap);
					float deltaMs = (needPx / personalOlder) * 1000.0f;
					float appliedMs = std::min(deltaMs, options->maxNudgeMsPerSecond * (float)dt);

					older.liveOffsetMs -= appliedMs;
					anyAdjustment = true;

					if (older.liveOffsetMs < -travelMsBase) older.liveOffsetMs = -travelMsBase;
					if (older.liveOffsetMs > travelMsBase * 0.5f) older.liveOffsetMs = travelMsBase * 0.5f;
				}
			} else { // UP direction
				olderY = options->height - olderElapsed * 0.001f * personalOlder - older.messageHeight;
				newerY = options->height - newerElapsed * 0.001f * personalNewer - newer.messageHeight;
				
				float olderBottom = olderY + older.messageTopInset + older.messageHeight;
				float newerTop = newerY + newer.messageTopInset;
				float gap = newerTop - olderBottom;
				
				if (gap < (options->minLineSpacingPx - 1.0f)) {
					float needPx = (options->minLineSpacingPx - gap);
					float deltaMs = (needPx / personalOlder) * 1000.0f;
					float appliedMs = std::min(deltaMs, options->maxNudgeMsPerSecond * (float)dt);

					older.liveOffsetMs -= appliedMs;
					anyAdjustment = true;

					if (older.liveOffsetMs < -travelMsBase) older.liveOffsetMs = -travelMsBase;
					if (older.liveOffsetMs > travelMsBase * 0.5f) older.liveOffsetMs = travelMsBase * 0.5f;
				}
			}
		}
		
		if (!anyAdjustment) {
			break;
		}
	}
}

void GW2_SCT::ScrollArea::applyHardSpacing(double dt, std::chrono::time_point<std::chrono::system_clock> frameNow) {
	if (options->textCurve == TextCurve::STATIC) return;
	if (options->minLineSpacingPx <= 0.0f) return;
	if (paintedMessages.size() < 2) return;
	
	if (getEffectiveScrollSpeed() <= 0.0f) return;
	
	std::vector<std::pair<MessagePrerender*, std::chrono::time_point<std::chrono::system_clock>*>> movingMessages;
	movingMessages.reserve(paintedMessages.size());
	for (auto& pair : paintedMessages) {
		if (std::isnan(pair.first.staticY)) {
			movingMessages.push_back({&pair.first, &pair.second});
		}
	}
	
	if (movingMessages.size() < 2) return;
	
	for (auto& msgPair : movingMessages) {
		msgPair.first->ensureExtents();
	}
	
	float baseScrollSpeed = getEffectiveScrollSpeed();
	float travelMsBase = options->height / baseScrollSpeed * 1000.0f;
	
	float areaOriginY = windowHeight * 0.5f + options->offsetY;
	
	if (options->scrollDirection == ScrollDirection::DOWN) {

		std::vector<std::pair<MessagePrerender*, float>> messageBasePositions;
		messageBasePositions.reserve(movingMessages.size());
		for (auto& msgPair : movingMessages) {
			float baseTime = (float)std::chrono::duration_cast<std::chrono::milliseconds>(frameNow - *msgPair.second).count();
			float elapsedMs = std::max(0.0f, baseTime - msgPair.first->liveOffsetMs);
			float baseY = areaOriginY + elapsedMs * 0.001f * (baseScrollSpeed * msgPair.first->personalFactor);
			messageBasePositions.push_back({msgPair.first, baseY});
		}
		
		std::sort(messageBasePositions.begin(), messageBasePositions.end(), 
			[](const std::pair<MessagePrerender*, float>& a, const std::pair<MessagePrerender*, float>& b) {
				float aTop = a.second + a.first->messageTopInset;
				float bTop = b.second + b.first->messageTopInset;
				return aTop < bTop;
			});
		
		float cursorTop = areaOriginY;
		
		for (auto& msgPair : messageBasePositions) {
			MessagePrerender* m = msgPair.first;
			float baseY = msgPair.second;
			
			float baseTop = baseY + m->messageTopInset;
			float baseBottom = baseTop + m->messageHeight;
			
			if (baseTop < cursorTop) {
				float placedBottom = cursorTop + m->messageHeight;
				bool wouldCrossExit = (placedBottom >= areaOriginY + options->height - 0.5f);
				
				if (wouldCrossExit) {
					m->forceExpire = true;
					continue;
				}
				
				float shiftPx = cursorTop - baseTop;
				if (shiftPx > 0.5f) {
					float personalSpeed = baseScrollSpeed * m->personalFactor;
					float advanceMs = (shiftPx / personalSpeed) * 1000.0f;
					m->liveOffsetMs -= advanceMs;
				}
				
				m->liveOffsetMs = std::clamp(m->liveOffsetMs, -travelMsBase, 0.5f * travelMsBase);
				
				cursorTop = cursorTop + m->messageHeight + options->minLineSpacingPx;
			} else {
				cursorTop = baseBottom + options->minLineSpacingPx;
			}
		}
		
	} else { // UP direction

		std::vector<std::pair<MessagePrerender*, float>> messageBasePositions;
		messageBasePositions.reserve(movingMessages.size());
		for (auto& msgPair : movingMessages) {
			float baseTime = (float)std::chrono::duration_cast<std::chrono::milliseconds>(frameNow - *msgPair.second).count();
			float elapsedMs = std::max(0.0f, baseTime - msgPair.first->liveOffsetMs);
			float animatedHeight = elapsedMs * 0.001f * (baseScrollSpeed * msgPair.first->personalFactor);
			float baseY = areaOriginY + options->height - animatedHeight - msgPair.first->messageHeight;
			messageBasePositions.push_back({msgPair.first, baseY});
		}
		
		std::sort(messageBasePositions.begin(), messageBasePositions.end(), 
			[](const std::pair<MessagePrerender*, float>& a, const std::pair<MessagePrerender*, float>& b) {
				float aBottom = a.second + a.first->messageTopInset + a.first->messageHeight;
				float bBottom = b.second + b.first->messageTopInset + b.first->messageHeight;
				return aBottom > bBottom;
			});
		
		float cursorBottom = areaOriginY + options->height;
		
		for (auto& msgPair : messageBasePositions) {
			MessagePrerender* m = msgPair.first;
			float baseY = msgPair.second;
			
			float baseTop = baseY + m->messageTopInset;
			float baseBottom = baseTop + m->messageHeight;
			
			if (baseBottom > cursorBottom) {
				float placedTop = cursorBottom - m->messageHeight;
				bool wouldCrossExit = (placedTop <= areaOriginY + 0.5f);
				
				if (wouldCrossExit) {
					m->forceExpire = true;
					continue;
				}
				
				float shiftPx = baseBottom - cursorBottom;
				if (shiftPx > 0.5f) {
					float personalSpeed = baseScrollSpeed * m->personalFactor;
					float advanceMs = (shiftPx / personalSpeed) * 1000.0f;
					m->liveOffsetMs -= advanceMs;
				}
				
				m->liveOffsetMs = std::clamp(m->liveOffsetMs, -travelMsBase, 0.5f * travelMsBase);

				cursorBottom = cursorBottom - m->messageHeight - options->minLineSpacingPx;
			} else {
				cursorBottom = baseTop - options->minLineSpacingPx;
			}
		}
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
