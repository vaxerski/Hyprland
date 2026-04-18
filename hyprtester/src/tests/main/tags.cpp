#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include "../shared.hpp"
#include "tests.hpp"

static int  ret = 0;

static bool testTags() {
    NLog::log("{}Testing tags", Colors::GREEN);

    EXPECT(Tests::windowCount(), 0);

    NLog::log("{}Spawning kittyProcA&B on ws 1", Colors::YELLOW);
    auto kittyProcA = Tests::spawnKitty("tagged");
    auto kittyProcB = Tests::spawnKitty("untagged");

    if (!kittyProcA || !kittyProcB) {
        NLog::log("{}Error: kitty did not spawn", Colors::RED);
        return false;
    }

    NLog::log("{}Testing testTag tags", Colors::YELLOW);

    OK(Tests::evalLua("hl.window_rule({ name = \"tag-test-1\", match = { class = \"tagged\" }, tag = \"+testTag\" })"));
    OK(Tests::evalLua("hl.window_rule({ name = \"tag-test-2\", match = { tag = \"negative:testTag\" }, no_shadow = true })"));
    OK(Tests::evalLua("hl.window_rule({ name = \"tag-test-3\", match = { tag = \"testTag\" }, no_dim = true })"));

    EXPECT(Tests::windowCount(), 2);
    OK(Tests::dispatchLua("hl.focus({ window = \"class:tagged\" })"));
    NLog::log("{}Testing tagged window for no_dim 0 & no_shadow", Colors::YELLOW);
    EXPECT_CONTAINS(getFromSocket("/activewindow"), "testTag");
    EXPECT_CONTAINS(getFromSocket("/getprop activewindow no_dim"), "true");
    EXPECT_CONTAINS(getFromSocket("/getprop activewindow no_shadow"), "false");
    NLog::log("{}Testing untagged window for no_dim & no_shadow", Colors::YELLOW);
    OK(Tests::dispatchLua("hl.focus({ window = \"class:untagged\" })"));
    EXPECT_NOT_CONTAINS(getFromSocket("/activewindow"), "testTag");
    EXPECT_CONTAINS(getFromSocket("/getprop activewindow no_shadow"), "true");
    EXPECT_CONTAINS(getFromSocket("/getprop activewindow no_dim"), "false");

    Tests::killAllWindows();
    EXPECT(Tests::windowCount(), 0);

    OK(getFromSocket("/reload"));

    return ret == 0;
}

REGISTER_TEST_FN(testTags)
