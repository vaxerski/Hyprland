#include "../shared.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include "tests.hpp"

static int         ret = 0;

static std::string setConfig(const std::string& key, const std::string& valueLuaExpr) {
    return Tests::evalLua("hl.config({ [" + Tests::luaQuote(key) + "] = " + valueLuaExpr + " })");
}

// reqs 1 master 3 slaves
static void testOrientations() {
    OK(setConfig("master.orientation", Tests::luaQuote("top")));

    // top
    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,22");
        EXPECT_CONTAINS(str, "size: 1876");
    }

    // cycle = top, right, bottom, center, left

    // right
    OK(Tests::dispatchLua("hl.layout(\"orientationnext\")"));
    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 873,22");
        EXPECT_CONTAINS(str, "size: 1025,1036");
    }

    // bottom
    OK(Tests::dispatchLua("hl.layout(\"orientationnext\")"));
    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,495");
        EXPECT_CONTAINS(str, "size: 1876");
    }

    // center
    OK(Tests::dispatchLua("hl.layout(\"orientationnext\")"));
    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 450,22");
        EXPECT_CONTAINS(str, "size: 1020,1036");
    }

    // left
    OK(Tests::dispatchLua("hl.layout(\"orientationnext\")"));
    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,22");
        EXPECT_CONTAINS(str, "size: 1025,1036");
    }
}

static void focusMasterPrevious() {
    // setup
    NLog::log("{}Spawning 1 master and 3 slave windows", Colors::YELLOW);
    // order of windows set according to new_status = master (set in test.lua)
    for (auto const& win : {"slave1", "slave2", "slave3", "master"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }
    NLog::log("{}Ensuring focus is on master before testing", Colors::YELLOW);
    OK(Tests::dispatchLua("hl.layout(\"focusmaster master\")"));
    EXPECT_CONTAINS(getFromSocket("/activewindow"), "class: master");

    // test
    NLog::log("{}Testing fallback to focusmaster auto", Colors::YELLOW);

    OK(Tests::dispatchLua("hl.layout(\"focusmaster previous\")"));
    EXPECT_CONTAINS(getFromSocket("/activewindow"), "class: slave1");

    NLog::log("{}Testing focusing from slave to master", Colors::YELLOW);

    OK(Tests::dispatchLua("hl.layout(\"cyclenext noloop\")"));
    EXPECT_CONTAINS(getFromSocket("/activewindow"), "class: slave2");
    OK(Tests::dispatchLua("hl.layout(\"focusmaster previous\")"));
    EXPECT_CONTAINS(getFromSocket("/activewindow"), "class: master");

    NLog::log("{}Testing focusing on previous window", Colors::YELLOW);

    OK(Tests::dispatchLua("hl.layout(\"focusmaster previous\")"));
    EXPECT_CONTAINS(getFromSocket("/activewindow"), "class: slave2");

    NLog::log("{}Testing focusing back to master", Colors::YELLOW);

    OK(Tests::dispatchLua("hl.layout(\"focusmaster previous\")"));
    EXPECT_CONTAINS(getFromSocket("/activewindow"), "class: master");

    testOrientations();

    // clean up
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static void testFsBehavior() {
    // Master will re-send data to fullscreen / maximized windows, which can interfere with misc:on_focus_under_fullscreen
    // check that it doesn't.

    for (auto const& win : {"master", "slave1", "slave2"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    OK(Tests::dispatchLua("hl.focus({ window = \"class:master\" })"));
    OK(Tests::dispatchLua("hl.window.fullscreen({ mode = \"maximized\" })"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,22");
        EXPECT_CONTAINS(str, "size: 1876,1036");
        EXPECT_CONTAINS(str, "class: master");
    }

    OK(setConfig("misc.on_focus_under_fullscreen", "1"));

    Tests::spawnKitty("new_master");

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,22");
        EXPECT_CONTAINS(str, "size: 1876,1036");
        EXPECT_CONTAINS(str, "class: new_master");
        EXPECT_CONTAINS(str, "fullscreen: 1");
    }

    OK(setConfig("misc.on_focus_under_fullscreen", "0"));

    Tests::spawnKitty("ignored");

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "at: 22,22");
        EXPECT_CONTAINS(str, "size: 1876,1036");
        EXPECT_CONTAINS(str, "class: new_master");
        EXPECT_CONTAINS(str, "fullscreen: 1");
    }

    OK(setConfig("misc.on_focus_under_fullscreen", "2"));

    Tests::spawnKitty("vaxwashere");

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "class: vaxwashere");
        EXPECT_CONTAINS(str, "fullscreen: 0");
    }

    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static bool test() {
    NLog::log("{}Testing Master layout", Colors::GREEN);

    // setup
    OK(Tests::dispatchLua("hl.workspace(\"name:master\")"));
    OK(setConfig("general.layout", Tests::luaQuote("master")));

    // test
    NLog::log("{}Testing `focusmaster previous` layoutmsg", Colors::GREEN);
    focusMasterPrevious();

    NLog::log("{}Testing fs behavior", Colors::GREEN);
    testFsBehavior();

    // clean up
    NLog::log("Cleaning up", Colors::YELLOW);
    OK(Tests::dispatchLua("hl.workspace(1)"));
    OK(getFromSocket("/reload"));

    return !ret;
}

REGISTER_TEST_FN(test);
