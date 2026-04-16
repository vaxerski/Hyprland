#include "ScrollMoveGesture.hpp"

#include "../../../../desktop/state/FocusState.hpp"
#include "../../../../desktop/Workspace.hpp"
#include "../../../../helpers/Monitor.hpp"
#include "../../../../layout/LayoutManager.hpp"
#include "../../../../layout/algorithm/Algorithm.hpp"
#include "../../../../layout/algorithm/tiled/scrolling/ScrollingAlgorithm.hpp"
#include "../../../../layout/space/Space.hpp"
#include "../../../../config/ConfigValue.hpp"

#include <cmath>

static Layout::Tiled::CScrollingAlgorithm* currentScrollingLayout() {
    const auto PMONITOR = Desktop::focusState()->monitor();
    if (!PMONITOR)
        return nullptr;

    const auto PWORKSPACE = PMONITOR->m_activeSpecialWorkspace ? PMONITOR->m_activeSpecialWorkspace : PMONITOR->m_activeWorkspace;
    if (!PWORKSPACE || !PWORKSPACE->m_space)
        return nullptr;

    const auto ALGORITHM = PWORKSPACE->m_space->algorithm();
    if (!ALGORITHM || !ALGORITHM->tiledAlgo())
        return nullptr;

    return dynamic_cast<Layout::Tiled::CScrollingAlgorithm*>(ALGORITHM->tiledAlgo().get());
}

static float deltaForUpdate(const ITrackpadGesture::STrackpadGestureUpdate& e) {
    if (!e.swipe)
        return 0.F;

    switch (e.direction) {
        case TRACKPAD_GESTURE_DIR_LEFT:
        case TRACKPAD_GESTURE_DIR_RIGHT:
        case TRACKPAD_GESTURE_DIR_HORIZONTAL: return e.swipe->delta.x * e.scale;
        case TRACKPAD_GESTURE_DIR_UP:
        case TRACKPAD_GESTURE_DIR_DOWN:
        case TRACKPAD_GESTURE_DIR_VERTICAL: return e.swipe->delta.y * e.scale;
        default: return std::abs(e.swipe->delta.x) > std::abs(e.swipe->delta.y) ? e.swipe->delta.x * e.scale : e.swipe->delta.y * e.scale;
    }
}

void CScrollMoveTrackpadGesture::begin(const ITrackpadGesture::STrackpadGestureBegin& e) {
    ITrackpadGesture::begin(e);

    m_wasScrollingLayout = currentScrollingLayout() != nullptr;
}

void CScrollMoveTrackpadGesture::update(const ITrackpadGesture::STrackpadGestureUpdate& e) {
    if (!m_wasScrollingLayout)
        return;

    const auto SCROLLING = currentScrollingLayout();
    if (!SCROLLING)
        return;

    const float DELTA = deltaForUpdate(e);

    if (DELTA == 0.F)
        return;

    SCROLLING->moveTape(DELTA);
}

void CScrollMoveTrackpadGesture::end(const ITrackpadGesture::STrackpadGestureEnd& e) {
    if (m_wasScrollingLayout) {
        if (const auto SCROLLING = currentScrollingLayout()) {
            static const auto PSCROLLMOVE_SNAP = CConfigValue<Hyprlang::INT>("scrolling:scrollmove_snap_on_release");
            if (*PSCROLLMOVE_SNAP)
                SCROLLING->commitScrollMoveSnap();
        }
    }
    m_wasScrollingLayout = false;
}
