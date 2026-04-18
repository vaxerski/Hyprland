#include "../shared.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include "tests.hpp"

static int         ret = 0;

static std::string setConfig(const std::string& key, const std::string& valueLuaExpr) {
    return Tests::evalLua("hl.config({ [" + Tests::luaQuote(key) + "] = " + valueLuaExpr + " })");
}

static void testFloatClamp() {
    for (auto const& win : {"a", "b", "c"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    OK(setConfig("dwindle.force_split", "2"));
    OK(Tests::evalLua("hl.monitor({ output = \"HEADLESS-2\", reserved = { top = 0, right = 20, bottom = 0, left = 20 } })"));
    OK(Tests::dispatchLua("hl.focus({ window = \"class:c\" })"));
    OK(Tests::dispatchLua("hl.window.float({ action = \"enable\", window = \"class:c\" })"));
    OK(Tests::dispatchLua("hl.window.resize({ x = 1200, y = 900, window = \"class:c\" })"));
    OK(Tests::dispatchLua("hl.window.float({ action = \"disable\", window = \"class:c\" })"));
    OK(Tests::dispatchLua("hl.window.float({ action = \"enable\", window = \"class:c\" })"));

    {
        auto str = getFromSocket("/clients");
        EXPECT(str.contains("at: 698,158") || str.contains("at: 698,178"), true);
        EXPECT_CONTAINS(str, "size: 1200,900");
    }

    OK(setConfig("dwindle.force_split", "0"));

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();

    OK(getFromSocket("/reload"));
}

static void test13349() {

    // Test if dwindle properly uses a focal point to place a new window.
    // exposed by #13349 as a regression from #12890

    for (auto const& win : {"a", "b", "c"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    OK(Tests::dispatchLua("hl.focus({ window = \"class:c\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 967,547");
        EXPECT_CONTAINS(str, "size: 931,511");
    }

    OK(Tests::dispatchLua("hl.window.move({ direction = \"l\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,547");
        EXPECT_CONTAINS(str, "size: 931,511");
    }

    OK(Tests::dispatchLua("hl.window.move({ direction = \"r\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 967,547");
        EXPECT_CONTAINS(str, "size: 931,511");
    }

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static void testSplit() {
    // Test various split methods

    Tests::spawnKitty("a");

    // these must not crash
    EXPECT_NOT(Tests::dispatchLua("hl.layout(\"swapsplit\")"), "ok");
    EXPECT_NOT(Tests::dispatchLua("hl.layout(\"splitratio 1 exact\")"), "ok");

    Tests::spawnKitty("b");

    OK(Tests::dispatchLua("hl.focus({ window = \"class:a\" })"));
    OK(Tests::dispatchLua("hl.layout(\"splitratio -0.2\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,22");
        EXPECT_CONTAINS(str, "size: 743,1036");
    }

    OK(Tests::dispatchLua("hl.layout(\"splitratio 1.6 exact\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,22");
        EXPECT_CONTAINS(str, "size: 1495,1036");
    }

    EXPECT_NOT(Tests::dispatchLua("hl.layout(\"splitratio fhne exact\")"), "ok");
    EXPECT_NOT(Tests::dispatchLua("hl.layout(\"splitratio exact\")"), "ok");
    EXPECT_NOT(Tests::dispatchLua("hl.layout(\"splitratio -....9\")"), "ok");
    EXPECT_NOT(Tests::dispatchLua("hl.layout(\"splitratio ..9\")"), "ok");
    EXPECT_NOT(Tests::dispatchLua("hl.layout(\"splitratio\")"), "ok");

    OK(Tests::dispatchLua("hl.layout(\"togglesplit\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,22");
        EXPECT_CONTAINS(str, "size: 1876,823");
    }

    OK(Tests::dispatchLua("hl.layout(\"swapsplit\")"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,859");
        EXPECT_CONTAINS(str, "size: 1876,199");
    }

    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static void testRotatesplit() {
    OK(setConfig("general.gaps_in", "0"));
    OK(setConfig("general.gaps_out", "0"));
    OK(setConfig("general.border_size", "0"));

    for (auto const& win : {"a", "b"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 0,0");
        EXPECT_CONTAINS(str, "size: 960,1080");
    }

    // test 4 repeated rotations by 90 degrees
    OK(Tests::dispatchLua("hl.layout(\"rotatesplit\")"));
    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 0,0");
        EXPECT_CONTAINS(str, "size: 1920,540");
    }

    OK(Tests::dispatchLua("hl.layout(\"rotatesplit\")"));
    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 960,0");
        EXPECT_CONTAINS(str, "size: 960,1080");
    }

    OK(Tests::dispatchLua("hl.layout(\"rotatesplit\")"));
    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 0,540");
        EXPECT_CONTAINS(str, "size: 1920,540");
    }

    OK(Tests::dispatchLua("hl.layout(\"rotatesplit\")"));
    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 0,0");
        EXPECT_CONTAINS(str, "size: 960,1080");
    }

    // test different angles
    OK(Tests::dispatchLua("hl.layout(\"rotatesplit 180\")"));
    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 960,0");
        EXPECT_CONTAINS(str, "size: 960,1080");
    }

    OK(Tests::dispatchLua("hl.layout(\"rotatesplit 270\")"));
    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 0,540");
        EXPECT_CONTAINS(str, "size: 1920,540");
    }

    OK(Tests::dispatchLua("hl.layout(\"rotatesplit 360\")"));
    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 0,0");
        EXPECT_CONTAINS(str, "size: 1920,540");
    }

    // test negative angles
    OK(Tests::dispatchLua("hl.layout(\"rotatesplit -90\")"));
    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 0,0");
        EXPECT_CONTAINS(str, "size: 960,1080");
    }

    OK(Tests::dispatchLua("hl.layout(\"rotatesplit -180\")"));
    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 960,0");
        EXPECT_CONTAINS(str, "size: 960,1080");
    }

    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();

    OK(getFromSocket("/reload"));
}

static void testForceSplitOnMoveToWorkspace() {
    OK(Tests::dispatchLua("hl.workspace(2)"));
    EXPECT(!!Tests::spawnKitty("kitty"), true);

    OK(Tests::dispatchLua("hl.workspace(1)"));
    EXPECT(!!Tests::spawnKitty("kitty"), true);
    std::string posBefore = Tests::getWindowAttribute(getFromSocket("/activewindow"), "at:");

    OK(setConfig("dwindle.force_split", "2"));
    OK(Tests::dispatchLua("hl.cursor.move_to_corner({ corner = 3 })")); // top left
    OK(Tests::dispatchLua("hl.window.move({ workspace = \"2\" })"));

    // Should be moved to the right, so the position should change
    std::string activeWindow = getFromSocket("/activewindow");
    EXPECT(activeWindow.contains(posBefore), false);

    // clean up
    OK(getFromSocket("/reload"));
    Tests::killAllWindows();
    Tests::waitUntilWindowsN(0);
}

static bool test() {
    NLog::log("{}Testing Dwindle layout", Colors::GREEN);

    // test
    NLog::log("{}Testing float clamp", Colors::GREEN);
    testFloatClamp();

    NLog::log("{}Testing #13349", Colors::GREEN);
    test13349();

    NLog::log("{}Testing splits", Colors::GREEN);
    testSplit();

    NLog::log("{}Testing rotatesplit", Colors::GREEN);
    testRotatesplit();

    NLog::log("{}Testing force_split on move to workspace", Colors::GREEN);
    testForceSplitOnMoveToWorkspace();

    // clean up
    NLog::log("Cleaning up", Colors::YELLOW);
    Tests::dispatchLua("hl.workspace(1)");
    OK(getFromSocket("/reload"));

    return !ret;
}

REGISTER_TEST_FN(test);
