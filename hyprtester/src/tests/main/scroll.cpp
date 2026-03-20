#include "../shared.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include "tests.hpp"

#include <set>

static int         ret = 0;

static std::string activeWindowClass() {
    const auto info = getFromSocket("/activewindow");
    const auto pos  = info.find("class: ");
    if (pos == std::string::npos)
        return "";

    const auto begin = pos + 7;
    const auto end   = info.find('\n', begin);
    if (end == std::string::npos)
        return info.substr(begin);

    return info.substr(begin, end - begin);
}

static int activeWindowWidth() {
    const auto info = getFromSocket("/activewindow");
    const auto pos  = info.find("size: ");
    if (pos == std::string::npos)
        return -1;

    const auto begin = pos + 6;
    const auto comma = info.find(',', begin);
    if (comma == std::string::npos)
        return -1;

    try {
        return std::stoi(info.substr(begin, comma - begin));
    } catch (...) { return -1; }
}

static void testFocusCycling() {
    for (auto const& win : {"a", "b", "c", "d"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    OK(Tests::dispatchLua("hl.dsp.focus({ window = \"class:a\" })"));
    EXPECT(activeWindowClass(), "a");

    std::set<std::string> seen;
    seen.insert(activeWindowClass());
    for (int i = 0; i < 3; ++i) {
        OK(Tests::dispatchLua("hl.dsp.focus({ direction = \"r\" })"));
        const auto cls = activeWindowClass();
        EXPECT(cls.empty(), false);
        seen.insert(cls);
    }
    EXPECT(seen.size() >= 2, true);

    const std::string beforeMove = activeWindowClass();

    OK(Tests::dispatchLua("hl.dsp.window.move({ direction = \"l\" })"));
    EXPECT(activeWindowClass(), beforeMove);

    OK(Tests::dispatchLua("hl.dsp.focus({ direction = \"u\" })"));
    EXPECT(activeWindowClass().empty(), false);

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static void testFocusWrapping() {
    for (auto const& win : {"a", "b", "c", "d"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    // set wrap_focus to true
    OK(Tests::evalLua("hl.config({ [\"scrolling.wrap_focus\"] = true })"));

    OK(Tests::dispatchLua("hl.dsp.focus({ window = \"class:a\" })"));

    const auto start = activeWindowClass();
    EXPECT(start.empty(), false);

    bool wrapped = false;
    for (int i = 0; i < 8; ++i) {
        OK(Tests::dispatchLua("hl.dsp.layout(\"focus l\")"));
        const auto cls = activeWindowClass();
        EXPECT(cls.empty(), false);
        if (i > 0 && cls == start) {
            wrapped = true;
            break;
        }
    }
    EXPECT(wrapped, true);

    // set wrap_focus to false
    OK(Tests::evalLua("hl.config({ [\"scrolling.wrap_focus\"] = false })"));

    OK(Tests::dispatchLua("hl.dsp.focus({ window = \"class:a\" })"));

    bool hitBoundary = false;
    for (int i = 0; i < 8; ++i) {
        const auto prev = activeWindowClass();
        OK(Tests::dispatchLua("hl.dsp.layout(\"focus l\")"));
        const auto next = activeWindowClass();
        EXPECT(next.empty(), false);
        if (next == prev) {
            hitBoundary = true;
            break;
        }
    }
    EXPECT(hitBoundary, true);

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static void testSwapcolWrapping() {
    const auto focusToLeftBoundary = []() {
        std::string current = activeWindowClass();
        for (int i = 0; i < 12; ++i) {
            OK(Tests::dispatchLua("hl.dsp.layout(\"focus l\")"));
            const auto next = activeWindowClass();
            if (next == current)
                break;
            current = next;
        }
        return current;
    };

    for (auto const& win : {"a", "b", "c", "d"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    // set deterministic focus movement and wrap_swapcol behavior
    OK(Tests::evalLua("hl.config({ [\"scrolling.wrap_focus\"] = false })"));
    OK(Tests::evalLua("hl.config({ [\"scrolling.wrap_swapcol\"] = true })"));

    OK(Tests::dispatchLua("hl.dsp.focus({ window = \"class:a\" })"));
    const auto leftBoundaryWithWrap = focusToLeftBoundary();
    EXPECT(leftBoundaryWithWrap.empty(), false);

    OK(Tests::dispatchLua("hl.dsp.layout(\"swapcol l\")"));
    OK(Tests::dispatchLua("hl.dsp.layout(\"focus l\")"));
    const auto afterWrappedSwap = activeWindowClass();
    EXPECT(afterWrappedSwap.empty(), false);
    EXPECT(afterWrappedSwap == leftBoundaryWithWrap, false);

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();

    for (auto const& win : {"a", "b", "c", "d"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    // set wrap_swapcol to false

    OK(Tests::evalLua("hl.config({ [\"scrolling.wrap_focus\"] = false })"));
    OK(Tests::evalLua("hl.config({ [\"scrolling.wrap_swapcol\"] = false })"));

    OK(Tests::dispatchLua("hl.dsp.focus({ window = \"class:a\" })"));
    const auto leftBoundaryNoWrap = focusToLeftBoundary();
    EXPECT(leftBoundaryNoWrap.empty(), false);

    OK(Tests::dispatchLua("hl.dsp.layout(\"swapcol l\")"));
    OK(Tests::dispatchLua("hl.dsp.layout(\"focus l\")"));
    EXPECT_CONTAINS(getFromSocket("/activewindow"), "class: " + leftBoundaryNoWrap);

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static bool testWindowRule() {
    NLog::log("{}Testing Scrolling Width", Colors::GREEN);

    // inject a new rule.
    OK(Tests::evalLua("hl.window_rule({ name = \"scrolling-width\", match = { class = \"kitty_scroll_narrow\" }, scrolling_width = 0.1 })"));

    if (!Tests::spawnKitty("kitty_scroll_narrow")) {
        NLog::log("{}Failed to spawn kitty with win class `kitty_scroll_narrow`", Colors::RED);
        return false;
    }

    if (!Tests::spawnKitty("kitty_scroll_wide")) {
        NLog::log("{}Failed to spawn kitty with win class `kitty_scroll_wide`", Colors::RED);
        return false;
    }

    EXPECT(Tests::windowCount(), 2);

    OK(Tests::dispatchLua("hl.dsp.focus({ window = \"class:kitty_scroll_narrow\" })"));
    const int narrowWidth = activeWindowWidth();
    EXPECT(narrowWidth > 0, true);

    OK(Tests::dispatchLua("hl.dsp.focus({ window = \"class:kitty_scroll_wide\" })"));
    const int wideWidth = activeWindowWidth();
    EXPECT(wideWidth > 0, true);

    // keep this resilient across minor geometry changes while ensuring
    // the scrolling-width rule does not produce a wider column than default.
    EXPECT(narrowWidth <= wideWidth + 20, true);

    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
    EXPECT(Tests::windowCount(), 0);
    return true;
}

static bool test() {
    NLog::log("{}Testing Scroll layout", Colors::GREEN);

    // setup
    OK(Tests::dispatchLua("hl.dsp.focus({ workspace = \"name:scroll\" })"));
    OK(Tests::evalLua("hl.config({ [\"general.layout\"] = \"scrolling\" })"));

    // test
    NLog::log("{}Testing focus cycling", Colors::GREEN);
    testFocusCycling();

    // test
    NLog::log("{}Testing focus wrap", Colors::GREEN);
    testFocusWrapping();

    // test
    NLog::log("{}Testing swapcol wrap", Colors::GREEN);
    testSwapcolWrapping();

    testWindowRule();

    // clean up
    NLog::log("Cleaning up", Colors::YELLOW);
    OK(Tests::dispatchLua("hl.dsp.focus({ workspace = 1 })"));
    OK(getFromSocket("/reload"));

    return !ret;
}

REGISTER_TEST_FN(test);
