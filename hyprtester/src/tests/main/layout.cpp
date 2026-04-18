#include "../shared.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include "tests.hpp"

static int  ret = 0;

static void swar() {
    OK(Tests::evalLua("hl.config({ [\"layout.single_window_aspect_ratio\"] = \"1 1\" })"));

    Tests::spawnKitty();

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 442,22");
        EXPECT_CONTAINS(str, "size: 1036,1036");
    }

    Tests::spawnKitty();

    OK(Tests::dispatchLua("hl.window.kill({ window = \"activewindow\" })"));

    Tests::waitUntilWindowsN(1);

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 442,22");
        EXPECT_CONTAINS(str, "size: 1036,1036");
    }

    // don't use swar on maximized
    OK(Tests::dispatchLua("hl.window.fullscreen({ mode = \"maximized\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,22");
        EXPECT_CONTAINS(str, "size: 1876,1036");
    }

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

// Don't crash when focus after global geometry changes
static void testCrashOnGeomUpdate() {
    Tests::spawnKitty();
    Tests::spawnKitty();
    Tests::spawnKitty();

    // move the layout
    OK(Tests::evalLua("hl.monitor({ output = \"HEADLESS-2\", mode = \"1920x1080@60\", position = \"1000x0\", scale = \"1\" })"));

    // shouldnt crash
    OK(Tests::dispatchLua("hl.focus({ direction = \"r\" })"));

    OK(getFromSocket("/reload"));

    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

// Test if size + pos is preserved after fs cycle
static void testPosPreserve() {
    Tests::spawnKitty();

    OK(Tests::dispatchLua("hl.window.float({ action = \"enable\", window = \"class:kitty\" })"));
    OK(Tests::dispatchLua("hl.window.resize({ window = \"class:kitty\", x = 1337, y = 69 })"));
    OK(Tests::dispatchLua("hl.window.move({ window = \"class:kitty\", x = 420, y = 420 })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 420,420");
        EXPECT_CONTAINS(str, "size: 1337,69");
    }

    OK(Tests::dispatchLua("hl.window.fullscreen({ mode = \"fullscreen\" })"));
    OK(Tests::dispatchLua("hl.window.fullscreen({ mode = \"fullscreen\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "size: 1337,69");
    }

    OK(Tests::dispatchLua("hl.window.move({ direction = \"r\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 581,420");
        EXPECT_CONTAINS(str, "size: 1337,69");
    }

    OK(Tests::dispatchLua("hl.window.fullscreen({ mode = \"fullscreen\" })"));
    OK(Tests::dispatchLua("hl.window.fullscreen({ mode = \"fullscreen\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 581,420");
        EXPECT_CONTAINS(str, "size: 1337,69");
    }

    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static bool testFocusMRUAfterClose() {
    NLog::log("{}Testing focus after close (MRU order)", Colors::GREEN);

    OK(getFromSocket("/reload"));
    OK(Tests::evalLua("hl.config({ [\"dwindle.default_split_ratio\"] = 1.25 })"));
    OK(Tests::evalLua("hl.config({ [\"input.focus_on_close\"] = 2 })"));

    EXPECT(!!Tests::spawnKitty("kitty_A"), true);
    EXPECT(!!Tests::spawnKitty("kitty_B"), true);
    EXPECT(!!Tests::spawnKitty("kitty_C"), true);

    OK(Tests::dispatchLua("hl.focus({ window = \"class:kitty_A\" })"));
    OK(Tests::dispatchLua("hl.focus({ window = \"class:kitty_B\" })"));
    OK(Tests::dispatchLua("hl.focus({ window = \"class:kitty_C\" })"));

    OK(Tests::dispatchLua("hl.window.close()"));
    Tests::waitUntilWindowsN(2);

    {
        auto str = getFromSocket("/activewindow");
        EXPECT(str.contains("class: kitty_B"), true);
    }

    OK(Tests::dispatchLua("hl.window.close()"));
    Tests::waitUntilWindowsN(1);

    {
        auto str = getFromSocket("/activewindow");
        EXPECT(str.contains("class: kitty_A"), true);
    }

    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
    EXPECT(Tests::windowCount(), 0);
    return true;
}

static bool testFocusPreservedLayoutChange() {
    NLog::log("{}Testing focus is preserved on layout change", Colors::GREEN);

    OK(Tests::evalLua("hl.config({ [\"general.layout\"] = \"master\" })"));

    EXPECT(!!Tests::spawnKitty("kitty_A"), true);
    EXPECT(!!Tests::spawnKitty("kitty_B"), true);
    EXPECT(!!Tests::spawnKitty("kitty_C"), true);
    EXPECT(!!Tests::spawnKitty("kitty_D"), true);

    OK(Tests::dispatchLua("hl.focus({ window = \"class:kitty_C\" })"));

    OK(Tests::evalLua("hl.config({ [\"general.layout\"] = \"monocle\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT(str.contains("class: kitty_C"), true);
    }

    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
    EXPECT(Tests::windowCount(), 0);
    return true;
}

static bool test() {
    NLog::log("{}Testing layout generic", Colors::GREEN);

    // setup
    OK(Tests::dispatchLua("hl.workspace(10)"));

    // test
    NLog::log("{}Testing `single_window_aspect_ratio`", Colors::GREEN);
    swar();

    testCrashOnGeomUpdate();
    testPosPreserve();
    testFocusMRUAfterClose();
    testFocusPreservedLayoutChange();

    // clean up
    NLog::log("Cleaning up", Colors::YELLOW);
    OK(Tests::dispatchLua("hl.workspace(1)"));
    OK(getFromSocket("/reload"));

    return !ret;
}

REGISTER_TEST_FN(test);
