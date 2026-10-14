#include "../globals.hpp"
#include "../config/ConfigManager.hpp"
#include "ShapeTransition.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/animation/AnimationManager.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprutils/animation/AnimationConfig.hpp>

static std::pair<Vector2D, Vector2D> parseCurve(const std::string& str, const std::pair<Vector2D, Vector2D>& def) {
    if (str == "default" || str == "apple_ease" || str.empty())
        return {Vector2D(0.25, 0.1), Vector2D(0.25, 1.0)};
    if (str == "fluid_jelly")
        return {Vector2D(0.34, 0.65), Vector2D(0.25, 1.0)};
    if (str == "natural_cubic")
        return {Vector2D(0.33, 1.0), Vector2D(0.68, 1.0)};
    if (str == "smooth_s")
        return {Vector2D(0.42, 0.0), Vector2D(0.58, 1.0)};
    if (str == "snappy")
        return {Vector2D(0.20, 0.0), Vector2D(0.0, 1.0)};

    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    if (sscanf(str.c_str(), "%f,%f,%f,%f", &x1, &y1, &x2, &y2) == 4 ||
        sscanf(str.c_str(), "%f %f %f %f", &x1, &y1, &x2, &y2) == 4) {
        return {Vector2D(std::clamp(x1, 0.0f, 1.0f), y1), Vector2D(std::clamp(x2, 0.0f, 1.0f), y2)};
    }
    return def;
}

void CShapeTransition::registerBeziers() {
    // Snappy anticipation shrink
    Animation::mgr()->addBezierWithName("dynamic-cursors-shape-shrink", Vector2D(0.25, 0.1), Vector2D(0.25, 1.0));

    // Configurable elastic restore
    auto restoreCurve = parseCurve(CONFIG(shapeTransitionRestoreCurve), {Vector2D(0.42, 0.0), Vector2D(0.58, 1.0)});
    Animation::mgr()->addBezierWithName("dynamic-cursors-shape-restore", restoreCurve.first, restoreCurve.second);
}

CShapeTransition::CShapeTransition() {
    registerBeziers();
    static const auto LISTENER = Event::bus()->m_events.config.reloaded.listen([this]() -> void { registerBeziers(); });

    m_pConfig = makeShared<SAnimationPropertyConfig>();
    m_pConfig->internalBezier  = "dynamic-cursors-shape-restore";
    m_pConfig->internalSpeed   = 1.0f;
    m_pConfig->internalEnabled = 1;
    m_pConfig->pValues         = m_pConfig;

    Animation::mgr()->createAnimation(1.0f, scale, m_pConfig, AVARDAMAGE_NONE);
}

void CShapeTransition::reset() {
    m_phase = EShapeTransitionPhase::IDLE;
    m_pendingShape.clear();
    m_pendingThisptr = nullptr;
    if (scale)
        *scale = 1.0f;
}

bool CShapeTransition::onShapeRequest(void* thisptr, const std::string& newShape, const std::function<void(void*, const std::string&)>& applyShape) {
    m_applyShapeCallback = applyShape;

    if (!CONFIG(shapeTransitionEnabled) || newShape == m_currentShape || newShape.empty() || m_currentShape.empty()) {
        m_currentShape = newShape;
        reset();
        return true;
    }

    float targetMin = std::clamp((float)CONFIG(shapeTransitionScale), 0.5f, 1.0f);

    if (m_phase == EShapeTransitionPhase::IDLE) {
        m_pendingShape   = newShape;
        m_pendingThisptr = thisptr;
        m_phase          = EShapeTransitionPhase::SHRINKING;
        m_phaseStartTime = steady_clock::now();

        m_pConfig->internalBezier = "dynamic-cursors-shape-shrink";
        m_pConfig->internalSpeed  = std::max(0.1f, (float)CONFIG(shapeTransitionShrinkDuration) / 100.0f);
        *scale                    = targetMin;
        return false;
    }

    if (m_phase == EShapeTransitionPhase::SHRINKING) {
        m_pendingShape   = newShape;
        m_pendingThisptr = thisptr;
        return false;
    }

    if (m_phase == EShapeTransitionPhase::EXPANDING) {
        m_pendingShape   = newShape;
        m_pendingThisptr = thisptr;
        m_phase          = EShapeTransitionPhase::SHRINKING;
        m_phaseStartTime = steady_clock::now();

        m_pConfig->internalBezier = "dynamic-cursors-shape-shrink";
        m_pConfig->internalSpeed  = std::max(0.1f, (float)CONFIG(shapeTransitionShrinkDuration) / 100.0f);
        *scale                    = targetMin;
        return false;
    }

    return true;
}

float CShapeTransition::update() {
    if (!CONFIG(shapeTransitionEnabled)) {
        if (m_phase != EShapeTransitionPhase::IDLE)
            reset();
        return 1.0f;
    }

    auto now = steady_clock::now();

    if (m_phase == EShapeTransitionPhase::SHRINKING) {
        int  shrinkDurationMs = CONFIG(shapeTransitionShrinkDuration);
        auto elapsed          = duration_cast<milliseconds>(now - m_phaseStartTime).count();
        float targetMin       = std::clamp((float)CONFIG(shapeTransitionScale), 0.5f, 1.0f);

        if (elapsed >= shrinkDurationMs || scale->value() <= targetMin + 0.015f) {
            // Texture swap moment!
            if (m_applyShapeCallback && m_pendingThisptr && !m_pendingShape.empty()) {
                m_currentShape = m_pendingShape;
                m_applyShapeCallback(m_pendingThisptr, m_pendingShape);
            }

            m_phase          = EShapeTransitionPhase::EXPANDING;
            m_phaseStartTime = now;

            m_pConfig->internalBezier = "dynamic-cursors-shape-restore";
            m_pConfig->internalSpeed  = std::max(0.1f, (float)CONFIG(shapeTransitionRestoreDuration) / 100.0f);
            *scale                    = 1.0f;
        }
    } else if (m_phase == EShapeTransitionPhase::EXPANDING) {
        int  restoreDurationMs = CONFIG(shapeTransitionRestoreDuration);
        auto elapsed           = duration_cast<milliseconds>(now - m_phaseStartTime).count();

        if (elapsed >= restoreDurationMs || (!scale->isBeingAnimated() && std::abs(scale->value() - 1.0f) < 0.005f)) {
            m_phase = EShapeTransitionPhase::IDLE;
            *scale  = 1.0f;
        }
    }

    return std::clamp(scale->value(), 0.5f, 1.0f);
}
