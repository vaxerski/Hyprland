#include "tests.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include <print>
#include <thread>
#include <chrono>
#include <format>
#include <hyprutils/os/Process.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <csignal>
#include <cerrno>
#include "../shared.hpp"

static int ret = 0;

using namespace Hyprutils::OS;
using namespace Hyprutils::Memory;

#define UP CUniquePointer
#define SP CSharedPointer

static std::string setConfig(const std::string& key, const std::string& valueLuaExpr) {
    return Tests::evalLua("hl.config({ [" + Tests::luaQuote(key) + "] = " + valueLuaExpr + " })");
}

static std::string setFullscreenMode(const std::string& mode, const std::string& action = "toggle") {
    return Tests::dispatchLua("hl.window.fullscreen({ mode = " + Tests::luaQuote(mode) + ", action = " + Tests::luaQuote(action) + " })");
}

static std::string setFullscreenState(const int internal, const int client) {
    return Tests::dispatchLua("hl.window.fullscreen_state({ internal = " + std::to_string(internal) + ", client = " + std::to_string(client) + " })");
}

static std::string toggleFullscreenState(const int internal, const int client) {
    return Tests::dispatchLua("hl.window.fullscreen_state({ internal = " + std::to_string(internal) + ", client = " + std::to_string(client) + ", action = \"toggle\" })");
}

// Uncomment once test vm can run hyprland-dialog
// static void testAnrDialogs() {
//     NLog::log("{}Testing ANR dialogs", Colors::YELLOW);
//
//     OK(getFromSocket("/keyword misc:enable_anr_dialog true"));
//     OK(getFromSocket("/keyword misc:anr_missed_pings 1"));
//
//     NLog::log("{}ANR dialog: regular workspaces", Colors::YELLOW);
//     {
//         OK(getFromSocket("/dispatch workspace 2"));
//
//         auto kitty = Tests::spawnKitty("bad_kitty");
//
//         if (!kitty) {
//             ret = 1;
//             return;
//         }
//
//         {
//             auto str = getFromSocket("/activewindow");
//             EXPECT_CONTAINS(str, "workspace: 2");
//         }
//
//         OK(getFromSocket("/dispatch workspace 1"));
//
//         ::kill(kitty->pid(), SIGSTOP);
//         Tests::waitUntilWindowsN(2);
//
//         {
//             auto str = getFromSocket("/activeworkspace");
//             EXPECT_CONTAINS(str, "windows: 0");
//         }
//
//         {
//             OK(getFromSocket("/dispatch focuswindow class:hyprland-dialog"))
//             auto str = getFromSocket("/activewindow");
//             EXPECT_CONTAINS(str, "workspace: 2");
//         }
//     }
//
//     Tests::killAllWindows();
//
//     NLog::log("{}ANR dialog: named workspaces", Colors::YELLOW);
//     {
//         OK(getFromSocket("/dispatch workspace name:yummy"));
//
//         auto kitty = Tests::spawnKitty("bad_kitty");
//
//         if (!kitty) {
//             ret = 1;
//             return;
//         }
//
//         {
//             auto str = getFromSocket("/activewindow");
//             EXPECT_CONTAINS(str, "yummy");
//         }
//
//         OK(getFromSocket("/dispatch workspace 1"));
//
//         ::kill(kitty->pid(), SIGSTOP);
//         Tests::waitUntilWindowsN(2);
//
//         {
//             auto str = getFromSocket("/activeworkspace");
//             EXPECT_CONTAINS(str, "windows: 0");
//         }
//
//         {
//             OK(getFromSocket("/dispatch focuswindow class:hyprland-dialog"))
//             auto str = getFromSocket("/activewindow");
//             EXPECT_CONTAINS(str, "yummy");
//         }
//     }
//
//     Tests::killAllWindows();
//
//     NLog::log("{}ANR dialog: special workspaces", Colors::YELLOW);
//     {
//         OK(getFromSocket("/dispatch workspace special:apple"));
//
//         auto kitty = Tests::spawnKitty("bad_kitty");
//
//         if (!kitty) {
//             ret = 1;
//             return;
//         }
//
//         {
//             auto str = getFromSocket("/activewindow");
//             EXPECT_CONTAINS(str, "special:apple");
//         }
//
//         OK(getFromSocket("/dispatch togglespecialworkspace apple"));
//         OK(getFromSocket("/dispatch workspace 1"));
//
//         ::kill(kitty->pid(), SIGSTOP);
//         Tests::waitUntilWindowsN(2);
//
//         {
//             auto str = getFromSocket("/activeworkspace");
//             EXPECT_CONTAINS(str, "windows: 0");
//         }
//
//         {
//             OK(getFromSocket("/dispatch focuswindow class:hyprland-dialog"))
//             auto str = getFromSocket("/activewindow");
//             EXPECT_CONTAINS(str, "special:apple");
//         }
//     }
//
//     OK(getFromSocket("/reload"));
//     Tests::killAllWindows();
// }

static bool test() {
    NLog::log("{}Testing config: misc:", Colors::GREEN);

    NLog::log("{}Testing close_special_on_empty", Colors::YELLOW);

    OK(setConfig("misc.close_special_on_empty", "false"));
    OK(Tests::dispatchLua("hl.workspace({ special = \"test\" })"));

    Tests::spawnKitty();

    {
        auto str = getFromSocket("/monitors");
        EXPECT_CONTAINS(str, "special workspace: -");
    }

    Tests::killAllWindows();

    {
        auto str = getFromSocket("/monitors");
        EXPECT_CONTAINS(str, "special workspace: -");
    }

    Tests::spawnKitty();

    OK(setConfig("misc.close_special_on_empty", "true"));

    Tests::killAllWindows();

    {
        auto str = getFromSocket("/monitors");
        EXPECT_NOT_CONTAINS(str, "special workspace: -");
    }

    NLog::log("{}Testing new_window_takes_over_fullscreen", Colors::YELLOW);

    OK(setConfig("misc.on_focus_under_fullscreen", "0"));

    Tests::spawnKitty("kitty_A");

    OK(setFullscreenMode("fullscreen"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
        EXPECT_CONTAINS(str, "kitty_A");
    }

    Tests::spawnKitty("kitty_B");

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
        EXPECT_CONTAINS(str, "kitty_A");
    }

    OK(Tests::dispatchLua("hl.focus({ window = \"class:kitty_B\" })"));

    {
        // should be ignored as per focus_under_fullscreen 0
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
        EXPECT_CONTAINS(str, "kitty_A");
    }

    OK(setConfig("misc.on_focus_under_fullscreen", "1"));

    Tests::spawnKitty("kitty_C");

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
        EXPECT_CONTAINS(str, "kitty_C");
    }

    OK(setConfig("misc.on_focus_under_fullscreen", "2"));

    Tests::spawnKitty("kitty_D");

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 0");
        EXPECT_CONTAINS(str, "fullscreenClient: 0");
        EXPECT_CONTAINS(str, "kitty_D");
    }

    OK(setConfig("misc.on_focus_under_fullscreen", "0"));

    Tests::killAllWindows();

    NLog::log("{}Testing exit_window_retains_fullscreen", Colors::YELLOW);

    OK(setConfig("misc.exit_window_retains_fullscreen", "false"));

    Tests::spawnKitty("kitty_A");
    Tests::spawnKitty("kitty_B");

    OK(setFullscreenMode("fullscreen"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
    }

    OK(Tests::dispatchLua("hl.window.kill({ window = \"activewindow\" })"));
    Tests::waitUntilWindowsN(1);

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 0");
        EXPECT_CONTAINS(str, "fullscreenClient: 0");
    }

    Tests::spawnKitty("kitty_B");
    OK(setFullscreenMode("fullscreen"));
    OK(setConfig("misc.exit_window_retains_fullscreen", "true"));

    OK(Tests::dispatchLua("hl.window.kill({ window = \"activewindow\" })"));
    Tests::waitUntilWindowsN(1);

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
    }

    Tests::killAllWindows();

    NLog::log("{}Testing fullscreen and fullscreenstate dispatcher", Colors::YELLOW);

    Tests::spawnKitty("kitty_A");
    Tests::spawnKitty("kitty_B");

    OK(Tests::dispatchLua("hl.focus({ window = \"class:kitty_A\" })"));
    OK(setFullscreenMode("fullscreen", "set"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
    }

    OK(setFullscreenMode("fullscreen", "unset"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 0");
        EXPECT_CONTAINS(str, "fullscreenClient: 0");
    }

    OK(setFullscreenMode("maximized", "toggle"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 1");
        EXPECT_CONTAINS(str, "fullscreenClient: 1");
    }

    OK(setFullscreenMode("maximized", "toggle"));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 0");
        EXPECT_CONTAINS(str, "fullscreenClient: 0");
    }

    OK(setFullscreenState(3, 3));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 3");
        EXPECT_CONTAINS(str, "fullscreenClient: 3");
    }

    OK(setFullscreenState(3, 3));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 3");
        EXPECT_CONTAINS(str, "fullscreenClient: 3");
    }

    OK(setFullscreenState(2, 2));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
    }

    OK(setFullscreenState(2, 2));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
    }

    OK(toggleFullscreenState(2, 2));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 0");
        EXPECT_CONTAINS(str, "fullscreenClient: 0");
    }

    OK(toggleFullscreenState(2, 2));

    {
        auto str = getFromSocket("/activewindow");
        EXPECT_CONTAINS(str, "fullscreen: 2");
        EXPECT_CONTAINS(str, "fullscreenClient: 2");
    }

    // Ensure that the process autostarted in the config does not
    // become a zombie even if it terminates very quickly.
    EXPECT(Tests::execAndGet("pgrep -f 'sleep 0'").empty(), true);

    // kill all
    NLog::log("{}Killing all windows", Colors::YELLOW);
    Tests::killAllWindows();

    NLog::log("{}Expecting 0 windows", Colors::YELLOW);
    EXPECT(Tests::windowCount(), 0);

    return !ret;
}

REGISTER_TEST_FN(test);
