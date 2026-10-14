#include "../globals.hpp"
#include "../config/ConfigManager.hpp"
#include "Click.hpp"

#include <algorithm>
#include <cstdio>
#include <linux/input-event-codes.h>
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

static void registerBeziers() {
    auto pressCurve = parseCurve(CONFIG(clickPressCurve), {Vector2D(0.25, 0.1), Vector2D(0.25, 1.0)});
    Animation::mgr()->addBezierWithName("dynamic-cursors-click-press", pressCurve.first, pressCurve.second);

    auto releaseCurve = parseCurve(CONFIG(clickReleaseCurve), {Vector2D(0.25, 0.1), Vector2D(0.25, 1.0)});
    Animation::mgr()->addBezierWithName("dynamic-cursors-click-release", releaseCurve.first, releaseCurve.second);
}

CClick::CClick() {
    registerBeziers();
    static const auto LISTENER = Event::bus()->m_events.config.reloaded.listen([]() -> void { registerBeziers(); });

    m_pConfig = makeShared<SAnimationPropertyConfig>();
    m_pConfig->internalBezier  = "dynamic-cursors-click-press";
    m_pConfig->internalSpeed   = 1.0f;
    m_pConfig->internalEnabled = 1;
    m_pConfig->pValues         = m_pConfig;

    Animation::mgr()->createAnimation(1.0f, scale, m_pConfig, AVARDAMAGE_NONE);
}

float CClick::update() {
    if (!CONFIG(clickEnabled))
        return 1.0f;
    return std::clamp(scale->value(), 0.1f, 1.0f);
}

void CClick::onButton(const IPointer::SButtonEvent& e) {
    if (!CONFIG(clickEnabled)) {
        *scale    = 1.0f;
        g_buttons = 0;
        return;
    }

    if (e.button != BTN_LEFT && e.button != BTN_MIDDLE)
        return;

    if (e.state == WL_POINTER_BUTTON_STATE_PRESSED) {
        g_buttons++;
        if (g_buttons == 1) {
            m_pConfig->internalBezier = "dynamic-cursors-click-press";
            m_pConfig->internalSpeed  = std::max(0.1f, (float)CONFIG(clickPressDuration) / 100.0f);
            *scale                    = std::clamp((float)CONFIG(clickScale), 0.5f, 1.0f);
        }
    } else if (e.state == WL_POINTER_BUTTON_STATE_RELEASED) {
        if (g_buttons > 0)
            g_buttons--;
        if (g_buttons == 0) {
            m_pConfig->internalBezier = "dynamic-cursors-click-release";
            m_pConfig->internalSpeed  = std::max(0.1f, (float)CONFIG(clickReleaseDuration) / 100.0f);
            *scale                    = 1.0f;
        }
    }
}

void CClick::takeoverAndPress(float fromScale) {
    if (!CONFIG(clickEnabled)) {
        *scale    = 1.0f;
        g_buttons = 0;
        return;
    }

    g_buttons++;
    if (g_buttons == 1) {
        float targetClickScale = std::clamp((float)CONFIG(clickScale), 0.5f, 1.0f);
        float startScale       = std::clamp(fromScale, 0.5f, 1.0f);

        // Seamlessly inherit the current visual scale as the animation origin
        scale->setValueAndWarp(startScale);

        // Continue downward press smoothly into click depth
        m_pConfig->internalBezier = "dynamic-cursors-click-press";
        m_pConfig->internalSpeed  = std::max(0.1f, (float)CONFIG(clickPressDuration) / 100.0f);
        *scale                    = targetClickScale;
    }
}
