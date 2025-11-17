#include "../shared.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include "tests.hpp"

static int  ret = 0;

static void testFloatClamp() {
    for (auto const& win : {"a", "b", "c"}) {
        if (!Tests::spawnKitty(win)) {
            NLog::log("{}Failed to spawn kitty with win class `{}`", Colors::RED, win);
            ++TESTS_FAILED;
            ret = 1;
            return;
        }
    }

    OK(getFromSocket("/keyword dwindle:force_split 2"));
    OK(getFromSocket("/keyword monitor HEADLESS-2, addreserved, 0, 20, 0, 20"));
    OK(getFromSocket("/dispatch focuswindow class:c"));
    OK(getFromSocket("/dispatch setfloating class:c"));
    OK(getFromSocket("/dispatch resizewindowpixel exact 1200 900,class:c"));
    OK(getFromSocket("/dispatch settiled class:c"));
    OK(getFromSocket("/dispatch setfloating class:c"));

    {
        auto str = getFromSocket("/clients");
        EXPECT_CONTAINS(str, "at: 698,158");
        EXPECT_CONTAINS(str, "size: 1200,900");
    }

    OK(getFromSocket("/keyword dwindle:force_split 0"));

    // clean up
    OK(getFromSocket("/reload"));
    OK(getFromSocket("/keyword monitor HEADLESS-2, addreserved, 0, 0, 0, 0"));
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();
}

static void testResize() {

    getFromSocket("/dispatch workspace 1");

    // force windows to split right
    OK(getFromSocket("/keyword dwindle:force_split 2"));
    OK(getFromSocket("/keyword dwindle:split_width_multiplier 0.001"));
    OK(getFromSocket("/keyword general:gaps_in 0"));
    OK(getFromSocket("/keyword general:gaps_out 0"));
    OK(getFromSocket("/keyword general:border_size 0"));

    const auto HALF = 1920 / 2;
    const auto QUARTER = 1920 / 4;

    Tests::spawnKitty("kitty_a");
    Tests::spawnKitty("kitty_b");
    Tests::spawnKitty("kitty_c");

    EXPECT(Tests::windowCount(), 3);

    {
        auto str = getFromSocket("/clients");
        EXPECT_COUNT_STRING(str, std::format("size: {},1080", HALF), 1);
        EXPECT_COUNT_STRING(str, std::format("size: {},1080", QUARTER), 2);
    }

    OK(getFromSocket("/dispatch focuswindow class:kitty_a"));
    OK(getFromSocket("/dispatch resizeactive -50 0"));

    {
        auto str = getFromSocket("/clients");
        EXPECT_COUNT_STRING(str, std::format("size: {},1080", HALF - 50), 1);
        EXPECT_COUNT_STRING(str, std::format("size: {},1080", QUARTER), 1);
        EXPECT_COUNT_STRING(str, std::format("size: {},1080", QUARTER + 50), 1);
    }

    NLog::log("{}Killing all windows", Colors::YELLOW);
    OK(getFromSocket("/reload"));
    Tests::killAllWindows();
}

static bool test() {
    NLog::log("{}Testing Dwindle layout", Colors::GREEN);

    // test
    NLog::log("{}Testing float clamp", Colors::GREEN);
    testFloatClamp();

    NLog::log("{}Testing resize", Colors::GREEN);
    testResize();

    // clean up
    NLog::log("Cleaning up", Colors::YELLOW);
    getFromSocket("/dispatch workspace 1");
    OK(getFromSocket("/reload"));

    return !ret;
}

REGISTER_TEST_FN(test);
