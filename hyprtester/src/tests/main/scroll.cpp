#include "../shared.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include "tests.hpp"

static int  ret = 0;

static void testFocusCycling() {
    for (auto const& win : {"a", "b", "c", "d"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    OK(Tests::dispatchLua("hl.focus({ window = \"class:a\" })"));

    OK(Tests::dispatchLua("hl.focus({ direction = \"r\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: b");
    }

    OK(Tests::dispatchLua("hl.focus({ direction = \"r\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: c");
    }

    OK(Tests::dispatchLua("hl.focus({ direction = \"r\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: d");
    }

    OK(Tests::dispatchLua("hl.window.move({ direction = \"l\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: d");
    }

    OK(Tests::dispatchLua("hl.focus({ direction = \"u\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: c");
    }

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

    OK(Tests::dispatchLua("hl.focus({ window = \"class:a\" })"));

    OK(Tests::dispatchLua("hl.layout(\"focus l\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: d");
    }

    OK(Tests::dispatchLua("hl.layout(\"focus r\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: a");
    }

    // set wrap_focus to false
    OK(Tests::evalLua("hl.config({ [\"scrolling.wrap_focus\"] = false })"));

    OK(Tests::dispatchLua("hl.focus({ window = \"class:a\" })"));

    OK(Tests::dispatchLua("hl.layout(\"focus l\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: a");
    }

    OK(Tests::dispatchLua("hl.focus({ window = \"class:d\" })"));

    OK(Tests::dispatchLua("hl.layout(\"focus r\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: d");
    }

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static void testSwapcolWrapping() {
    for (auto const& win : {"a", "b", "c", "d"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    // set wrap_swapcol to true
    OK(Tests::evalLua("hl.config({ [\"scrolling.wrap_swapcol\"] = true })"));

    OK(Tests::dispatchLua("hl.focus({ window = \"class:a\" })"));

    OK(Tests::dispatchLua("hl.layout(\"swapcol l\")"));
    OK(Tests::dispatchLua("hl.layout(\"focus l\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: c");
    }

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

    OK(Tests::dispatchLua("hl.focus({ window = \"class:d\" })"));
    OK(Tests::dispatchLua("hl.layout(\"swapcol r\")"));
    OK(Tests::dispatchLua("hl.layout(\"focus r\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: b");
    }

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
    OK(Tests::evalLua("hl.config({ [\"scrolling.wrap_swapcol\"] = false })"));

    OK(Tests::dispatchLua("hl.focus({ window = \"class:a\" })"));

    OK(Tests::dispatchLua("hl.layout(\"swapcol l\")"));
    OK(Tests::dispatchLua("hl.layout(\"focus r\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: b");
    }

    OK(Tests::dispatchLua("hl.focus({ window = \"class:d\" })"));

    OK(Tests::dispatchLua("hl.layout(\"swapcol r\")"));
    OK(Tests::dispatchLua("hl.layout(\"focus l\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: c");
    }

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static bool testWindowRule() {
    NLog::log("{}Testing Scrolling Width", Colors::GREEN);

    // inject a new rule.
    OK(Tests::evalLua("hl.window_rule({ name = \"scrolling-width\", match = { class = \"kitty_scroll\" }, scrolling_width = 0.1 })"));

    if (!Tests::spawnKitty("kitty_scroll")) {
        NLog::log("{}Failed to spawn kitty with win class `kitty_scroll`", Colors::RED);
        return false;
    }

    if (!Tests::spawnKitty("kitty_scroll")) {
        NLog::log("{}Failed to spawn kitty with win class `kitty_scroll`", Colors::RED);
        return false;
    }

    EXPECT(Tests::windowCount(), 2);

    // not the greatest test, but as long as res and gaps don't change, we good.
    EXPECT_CONTAINS(getFromSocket("/activewindow"), "size: 174,1036");

    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
    EXPECT(Tests::windowCount(), 0);
    return true;
}

static bool test() {
    NLog::log("{}Testing Scroll layout", Colors::GREEN);

    // setup
    OK(Tests::dispatchLua("hl.workspace(\"name:scroll\")"));
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
    OK(Tests::dispatchLua("hl.workspace(1)"));
    OK(getFromSocket("/reload"));

    return !ret;
}

REGISTER_TEST_FN(test);
