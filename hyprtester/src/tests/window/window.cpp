#include "window.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include <print>
#include <thread>
#include <chrono>
#include <hyprutils/os/Process.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <csignal>
#include <cerrno>

static int ret = 0;

using namespace Hyprutils::OS;
using namespace Hyprutils::Memory;

#define UP CUniquePointer
#define SP CSharedPointer

static CProcess spawnKitty() {
    CProcess kitty{"/bin/bash", {"-c", std::format("WAYLAND_DISPLAY={} kitty", WLDISPLAY)}};
    kitty.runAsync();
    return kitty;
}

static bool processAlive(pid_t pid) {
    kill(pid, 0);
    return errno != ESRCH;
}

static int windowCount() {
    auto str = getFromSocket("/clients");
    int  cnt = 0;
    auto pos = str.find("focusHistoryID: ");
    while (pos != std::string::npos) {
        cnt++;
        pos = str.find("focusHistoryID: ", pos + 5);
    }

    return cnt;
}

bool testWindows() {
    std::println("{}Testing windows", Colors::GREEN);

    // test on workspace "window"
    getFromSocket("/dispatch workspace name:window");

    auto kittyProcA = spawnKitty();
    int  counter    = 0;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    while (processAlive(kittyProcA.pid()) && windowCount() != 1) {
        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (counter > 20) {
            EXPECT(windowCount(), 1);
            return !ret;
        }
    }

    EXPECT(windowCount(), 1);

    // check kitty properties. One kitty should take the entire screen, as this is smart gaps
    {
        auto str = getFromSocket("/clients");
        EXPECT(str.contains("at: 0,0"), true);
        EXPECT(str.contains("size: 1920,1080"), true);
        EXPECT(str.contains("fullscreen: 0"), true);
    }

    auto kittyProcB = spawnKitty();
    counter         = 0;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    while (processAlive(kittyProcB.pid()) && windowCount() != 2) {
        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (counter > 20) {
            EXPECT(windowCount(), 2);
            return !ret;
        }
    }

    EXPECT(windowCount(), 2);
    
    // kill both kitties
    {
        auto str = getFromSocket("/clients");
        auto pos = str.find("Window ");
        while (pos != std::string::npos) {
            auto pos2 = str.find(" -> ", pos);
            getFromSocket("/dispatch killwindow address:0x" + str.substr(pos + 7, pos2 - pos - 7));
            pos = str.find("Window ", pos + 5);
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT(windowCount(), 0);

    return !ret;
}