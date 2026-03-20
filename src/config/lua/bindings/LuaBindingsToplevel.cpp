#include "LuaBindingsInternal.hpp"

#include "../objects/LuaEventSubscription.hpp"
#include "../objects/LuaKeybind.hpp"
#include "../objects/LuaTimer.hpp"

#include "../../supplementary/executor/Executor.hpp"

#include "../../../devices/IKeyboard.hpp"
#include "../../../managers/SeatManager.hpp"
#include "../../../managers/eventLoop/EventLoopManager.hpp"

#include <hyprutils/string/String.hpp>
#include <hyprutils/string/VarList.hpp>

using namespace Config;
using namespace Config::Lua;
using namespace Config::Lua::Bindings;
using namespace Hyprutils::String;

namespace CA = Config::Actions;

static std::optional<eKeyboardModifiers> modFromSv(std::string_view sv) {
    if (sv == "SHIFT")
        return HL_MODIFIER_SHIFT;
    if (sv == "CAPS")
        return HL_MODIFIER_CAPS;
    if (sv == "CTRL" || sv == "CONTROL")
        return HL_MODIFIER_CTRL;
    if (sv == "ALT" || sv == "MOD1")
        return HL_MODIFIER_ALT;
    if (sv == "MOD2")
        return HL_MODIFIER_MOD2;
    if (sv == "MOD3")
        return HL_MODIFIER_MOD3;
    if (sv == "SUPER" || sv == "WIN" || sv == "LOGO" || sv == "MOD4" || sv == "META")
        return HL_MODIFIER_META;
    if (sv == "MOD5")
        return HL_MODIFIER_MOD5;

    return std::nullopt;
}

static bool isSymSpecial(std::string_view sv) {
    if (sv == "mouse_down" || sv == "mouse_up" || sv == "mouse_left" || sv == "mouse_right")
        return true;

    return sv.starts_with("switch:") || sv.starts_with("mouse:");
}

static std::expected<void, std::string> parseKeyString(SKeybind& kb, std::string_view sv) {
    bool                      modsEnded = false, specialSym = false;
    CVarList2                 vl(sv, 0, '+', true);

    uint32_t                  modMask = 0;
    std::vector<xkb_keysym_t> keysyms;

    for (const auto& a : vl) {
        auto arg = Hyprutils::String::trim(a);

        auto mask = modFromSv(arg);

        if (!mask)
            modsEnded = true;

        if (modsEnded && mask)
            return std::unexpected("Modifiers must come first in the list");

        if (mask) {
            modMask |= *mask;
            continue;
        }

        if (specialSym)
            return std::unexpected("Cannot combine special syms (e.g. mouse_down + Q)");

        if (isSymSpecial(arg)) {
            if (!keysyms.empty())
                return std::unexpected("Cannot combine special syms (e.g. mouse_down + Q)");

            specialSym = true;
            kb.key     = arg;
            continue;
        }

        auto sym = xkb_keysym_from_name(std::string{arg}.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);

        if (sym == XKB_KEY_NoSymbol) {
            if (arg.contains(' '))
                return std::unexpected(std::format("Unknown keysym: \"{}\", did you forget a +?", arg));

            if (arg == "Enter")
                return std::unexpected(std::format(R"(Unknown keysym: "{}", did you mean "Return"?)", arg));

            return std::unexpected(std::format("Unknown keysym: \"{}\"", arg));
        }

        keysyms.emplace_back(sym);
    }

    kb.modmask = modMask;
    kb.sMkKeys = std::move(keysyms);
    return {};
}

static int hlBind(lua_State* L) {
    auto*            mgr = static_cast<CConfigManager*>(lua_touserdata(L, lua_upvalueindex(1)));

    std::string_view keys = luaL_checkstring(L, 1);

    SKeybind         kb;
    kb.submap.name = mgr->m_currentSubmap;

    if (auto res = parseKeyString(kb, keys); !res)
        return luaL_error(L, std::format("hl.bind: failed to parse key string: {}", res.error()).c_str());

    if (!lua_isfunction(L, 2))
        return luaL_error(L, "hl.bind: dispatcher must be a function (e.g. hl.window.close()) or a lua function");

    lua_pushvalue(L, 2);
    int ref       = luaL_ref(L, LUA_REGISTRYINDEX);
    kb.handler    = "__lua";
    kb.arg        = std::to_string(ref);
    kb.displayKey = keys;

    int optsIdx = 3;

    if (lua_istable(L, optsIdx)) {
        auto getBool = [&](const char* field) -> bool {
            lua_getfield(L, optsIdx, field);
            const bool v = lua_toboolean(L, -1);
            lua_pop(L, 1);
            return v;
        };

        auto readOptString = [&](const char* field) -> std::optional<std::string> {
            lua_getfield(L, optsIdx, field);

            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return std::nullopt;
            }

            if (!lua_isstring(L, -1))
                luaL_error(L, "hl.bind: opts.%s must be a string", field);

            std::string result = lua_tostring(L, -1);
            lua_pop(L, 1);
            return result;
        };

        kb.repeat          = getBool("repeating");
        kb.locked          = getBool("locked");
        kb.release         = getBool("release");
        kb.nonConsuming    = getBool("non_consuming");
        kb.transparent     = getBool("transparent");
        kb.ignoreMods      = getBool("ignore_mods");
        kb.dontInhibit     = getBool("dont_inhibit");
        kb.longPress       = getBool("long_press");
        kb.submapUniversal = getBool("submap_universal");

        if (auto description = readOptString("description"); description.has_value()) {
            kb.description    = *description;
            kb.hasDescription = true;
        } else if (auto desc = readOptString("desc"); desc.has_value()) {
            kb.description    = *desc;
            kb.hasDescription = true;
        }

        bool click = false;
        bool drag  = false;

        if (getBool("click")) {
            click      = true;
            kb.release = true;
        }

        if (getBool("drag")) {
            drag       = true;
            kb.release = true;
        }

        if (click && drag)
            luaL_error(L, "hl.bind: click and drag are exclusive");

        if ((kb.longPress || kb.release) && kb.repeat)
            return luaL_error(L, "hl.bind: long_press / release is incompatible with repeat");

        if (kb.mouse && (kb.repeat || kb.release || kb.locked))
            return luaL_error(L, "hl.bind: mouse is exclusive");

        kb.click = click;
        kb.drag  = drag;

        lua_getfield(L, optsIdx, "device");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "inclusive");
            kb.deviceInclusive = lua_isnil(L, -1) ? true : lua_toboolean(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, -1, "list");
            if (lua_istable(L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2)) {
                    if (lua_isstring(L, -1))
                        kb.devices.emplace(lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }

    const auto BIND = g_pKeybindManager->addKeybind(kb);
    Objects::CLuaKeybind::push(L, BIND);
    return 1;
}

static int hlDefineSubmap(lua_State* L) {
    auto*       mgr  = static_cast<CConfigManager*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    std::string prev     = mgr->m_currentSubmap;
    mgr->m_currentSubmap = name;

    lua_pushvalue(L, 2);
    if (mgr->guardedPCall(0, 0, 0, CConfigManager::LUA_TIMEOUT_DISPATCH_MS, std::format("hl.define_submap(\"{}\")", name)) != LUA_OK) {
        mgr->addError(std::string("hl.define_submap: error in submap \"") + name + "\": " + lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    mgr->m_currentSubmap = prev;
    return 0;
}

static int dsp_execCmd(lua_State* L) {
    auto proc = Config::Supplementary::executor()->spawn(lua_tostring(L, lua_upvalueindex(1)));
    if (!proc.has_value())
        return luaL_error(L, "Failed to start process");
    return 0;
}

static int dsp_execRaw(lua_State* L) {
    auto proc = Config::Supplementary::executor()->spawnRaw(lua_tostring(L, lua_upvalueindex(1)));
    if (!proc || !*proc)
        return luaL_error(L, "Failed to start process");
    return 0;
}

static int dsp_exit(lua_State* L) {
    Internal::checkResult(L, CA::exit());
    return 0;
}

static int dsp_submap(lua_State* L) {
    Internal::checkResult(L, CA::setSubmap(lua_tostring(L, lua_upvalueindex(1))));
    return 0;
}

static int dsp_pass(lua_State* L) {
    const auto PWINDOW = g_pCompositor->getWindowByRegex(lua_tostring(L, lua_upvalueindex(1)));
    if (!PWINDOW)
        return luaL_error(L, "hl.pass: window not found");
    Internal::checkResult(L, CA::pass(PWINDOW));
    return 0;
}

static int dsp_layoutMsg(lua_State* L) {
    Internal::checkResult(L, CA::layoutMessage(lua_tostring(L, lua_upvalueindex(1))));
    return 0;
}

static int dsp_dpms(lua_State* L) {
    auto                      action = sc<CA::eTogglableAction>((int)lua_tonumber(L, lua_upvalueindex(1)));
    std::optional<PHLMONITOR> mon    = std::nullopt;

    if (!lua_isnil(L, lua_upvalueindex(2))) {
        auto m = g_pCompositor->getMonitorFromString(lua_tostring(L, lua_upvalueindex(2)));
        if (m)
            mon = m;
    }

    Internal::checkResult(L, CA::dpms(action, mon));
    return 0;
}

static int dsp_event(lua_State* L) {
    Internal::checkResult(L, CA::event(lua_tostring(L, lua_upvalueindex(1))));
    return 0;
}

static int dsp_global(lua_State* L) {
    Internal::checkResult(L, CA::global(lua_tostring(L, lua_upvalueindex(1))));
    return 0;
}

static int dsp_forceRendererReload(lua_State* L) {
    Internal::checkResult(L, CA::forceRendererReload());
    return 0;
}

static int dsp_forceIdle(lua_State* L) {
    Internal::checkResult(L, CA::forceIdle((float)lua_tonumber(L, lua_upvalueindex(1))));
    return 0;
}

static int hlExecCmd(lua_State* L) {
    lua_pushstring(L, luaL_checkstring(L, 1));
    lua_pushcclosure(L, dsp_execCmd, 1);
    return 1;
}

static int hlExecRaw(lua_State* L) {
    lua_pushstring(L, luaL_checkstring(L, 1));
    lua_pushcclosure(L, dsp_execRaw, 1);
    return 1;
}

static int hlExit(lua_State* L) {
    lua_pushcclosure(L, dsp_exit, 0);
    return 1;
}

static int hlSubmap(lua_State* L) {
    lua_pushstring(L, luaL_checkstring(L, 1));
    lua_pushcclosure(L, dsp_submap, 1);
    return 1;
}

static int hlPass(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.pass: expected a table { window }");

    const auto w = Internal::requireTableFieldWindowSelector(L, 1, "window", "hl.pass");
    lua_pushstring(L, w.c_str());
    lua_pushcclosure(L, dsp_pass, 1);
    return 1;
}

static int hlLayout(lua_State* L) {
    lua_pushstring(L, luaL_checkstring(L, 1));
    lua_pushcclosure(L, dsp_layoutMsg, 1);
    return 1;
}

static int hlDpms(lua_State* L) {
    CA::eTogglableAction       action = Internal::tableToggleAction(L, 1);
    std::optional<std::string> monStr;

    if (lua_istable(L, 1))
        monStr = Internal::tableOptMonitorSelector(L, 1, "monitor", "hl.dpms");

    lua_pushnumber(L, (int)action);
    if (monStr)
        lua_pushstring(L, monStr->c_str());
    else
        lua_pushnil(L);
    lua_pushcclosure(L, dsp_dpms, 2);
    return 1;
}

static int hlEvent(lua_State* L) {
    lua_pushstring(L, luaL_checkstring(L, 1));
    lua_pushcclosure(L, dsp_event, 1);
    return 1;
}

static int hlGlobal(lua_State* L) {
    lua_pushstring(L, luaL_checkstring(L, 1));
    lua_pushcclosure(L, dsp_global, 1);
    return 1;
}

static int hlForceRendererReload(lua_State* L) {
    lua_pushcclosure(L, dsp_forceRendererReload, 0);
    return 1;
}

static int hlForceIdle(lua_State* L) {
    lua_pushnumber(L, luaL_checknumber(L, 1));
    lua_pushcclosure(L, dsp_forceIdle, 1);
    return 1;
}

static std::expected<uint32_t, std::string> resolveKeycode(const std::string& key) {
    if (isNumber(key) && std::stoi(key) > 9)
        return (uint32_t)std::stoi(key);

    if (key.starts_with("code:") && isNumber(key.substr(5)))
        return (uint32_t)std::stoi(key.substr(5));

    if (key.starts_with("mouse:") && isNumber(key.substr(6))) {
        uint32_t code = std::stoi(key.substr(6));
        if (code < 272)
            return std::unexpected("invalid mouse button");
        return code;
    }

    const auto KEYSYM = xkb_keysym_from_name(key.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);

    const auto KB = g_pSeatManager->m_keyboard;
    if (!KB)
        return std::unexpected("no keyboard");

    const auto KEYPAIRSTRING = std::format("{}{}", rc<uintptr_t>(KB.get()), key);

    if (g_pKeybindManager->m_keyToCodeCache.contains(KEYPAIRSTRING))
        return g_pKeybindManager->m_keyToCodeCache[KEYPAIRSTRING];

    xkb_keymap*   km          = KB->m_xkbKeymap;
    xkb_state*    ks          = KB->m_xkbState;
    xkb_keycode_t keycode_min = xkb_keymap_min_keycode(km);
    xkb_keycode_t keycode_max = xkb_keymap_max_keycode(km);
    uint32_t      keycode     = 0;

    for (xkb_keycode_t kc = keycode_min; kc <= keycode_max; ++kc) {
        xkb_keysym_t sym = xkb_state_key_get_one_sym(ks, kc);
        if (sym == KEYSYM) {
            keycode                                            = kc;
            g_pKeybindManager->m_keyToCodeCache[KEYPAIRSTRING] = keycode;
        }
    }

    if (!keycode)
        return std::unexpected("key not found");

    return keycode;
}

static int dsp_sendShortcut(lua_State* L) {
    const uint32_t    modMask = g_pKeybindManager->stringToModMask(lua_tostring(L, lua_upvalueindex(1)));
    const std::string key     = lua_tostring(L, lua_upvalueindex(2));

    auto              keycodeResult = resolveKeycode(key);
    if (!keycodeResult)
        return luaL_error(L, "hl.send_shortcut: %s", keycodeResult.error().c_str());

    PHLWINDOW window = nullptr;
    if (!lua_isnil(L, lua_upvalueindex(3))) {
        window = g_pCompositor->getWindowByRegex(lua_tostring(L, lua_upvalueindex(3)));
        if (!window)
            return luaL_error(L, "hl.send_shortcut: window not found");
    }

    Internal::checkResult(L, CA::pass(modMask, *keycodeResult, window));
    return 0;
}

static int dsp_sendKeyState(lua_State* L) {
    const uint32_t    modMask  = g_pKeybindManager->stringToModMask(lua_tostring(L, lua_upvalueindex(1)));
    const std::string key      = lua_tostring(L, lua_upvalueindex(2));
    const uint32_t    keyState = (uint32_t)lua_tonumber(L, lua_upvalueindex(3));

    auto              keycodeResult = resolveKeycode(key);
    if (!keycodeResult)
        return luaL_error(L, "hl.send_key_state: %s", keycodeResult.error().c_str());

    PHLWINDOW window = nullptr;
    if (!lua_isnil(L, lua_upvalueindex(4))) {
        window = g_pCompositor->getWindowByRegex(lua_tostring(L, lua_upvalueindex(4)));
        if (!window)
            return luaL_error(L, "hl.send_key_state: window not found");
    }

    Internal::checkResult(L, CA::sendKeyState(modMask, *keycodeResult, keyState, window));
    return 0;
}

static int hlSendShortcut(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.send_shortcut: expected a table { mods, key, window? }");

    const auto mods = Internal::requireTableFieldStr(L, 1, "mods", "hl.send_shortcut");
    const auto key  = Internal::requireTableFieldStr(L, 1, "key", "hl.send_shortcut");

    lua_pushstring(L, mods.c_str());
    lua_pushstring(L, key.c_str());
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_sendShortcut, 3);
    return 1;
}

static int hlSendKeyState(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.send_key_state: expected a table { mods, key, state, window? }");

    const auto mods     = Internal::requireTableFieldStr(L, 1, "mods", "hl.send_key_state");
    const auto key      = Internal::requireTableFieldStr(L, 1, "key", "hl.send_key_state");
    const auto stateStr = Internal::requireTableFieldStr(L, 1, "state", "hl.send_key_state");

    uint32_t   keyState = 0;
    if (stateStr == "down")
        keyState = 1;
    else if (stateStr == "repeat")
        keyState = 2;
    else if (stateStr != "up")
        return luaL_error(L, "hl.send_key_state: 'state' must be \"down\", \"up\", or \"repeat\"");

    lua_pushstring(L, mods.c_str());
    lua_pushstring(L, key.c_str());
    lua_pushnumber(L, keyState);
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_sendKeyState, 4);
    return 1;
}

static int hlDispatch(lua_State* L) {
    if (!lua_isfunction(L, 1))
        return luaL_error(L, "hl.dispatch: expected a dispatcher function (e.g. hl.window.close())");

    lua_pushvalue(L, 1);
    int status = LUA_OK;
    if (auto* mgr = CConfigManager::fromLuaState(L); mgr)
        status = mgr->guardedPCall(0, 0, 0, CConfigManager::LUA_TIMEOUT_DISPATCH_MS, "hl.dispatch");
    else
        status = lua_pcall(L, 0, 0, 0);

    if (status != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        lua_pop(L, 1);
        return luaL_error(L, "hl.dispatch: %s", err);
    }

    return 0;
}

static int hlOn(lua_State* L) {
    auto*       mgr       = static_cast<CConfigManager*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* eventName = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushvalue(L, 2);
    int        ref = luaL_ref(L, LUA_REGISTRYINDEX);

    const auto handle = mgr->m_eventHandler->registerEvent(eventName, ref);
    if (!handle.has_value()) {
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
        const auto& known = CLuaEventHandler::knownEvents();
        std::string list;
        for (const auto& e : known)
            list += "\n  " + e;
        return luaL_error(L, "%s", (std::string("hl.on: unknown event \"") + eventName + "\". Known events:" + list).c_str());
    }

    Objects::CLuaEventSubscription::push(L, mgr->m_eventHandler.get(), *handle);
    return 1;
}

static int hlExecOnce(lua_State* L) {
    auto* mgr = static_cast<CConfigManager*>(lua_touserdata(L, lua_upvalueindex(1)));

    if (mgr->isFirstLaunch())
        Config::Supplementary::executor()->addExecOnce({Internal::argStr(L, 1), true});

    return 0;
}

static int hlExecShutdown(lua_State* L) {
    if (g_pCompositor->m_finalRequests) {
        Config::Supplementary::executor()->spawn(Internal::argStr(L, 1));
        return 0;
    }

    Config::Supplementary::executor()->addExecShutdown({Internal::argStr(L, 1), true});
    return 0;
}

static int hlUnbind(lua_State* L) {
    if (lua_isstring(L, 1) && std::string_view(lua_tostring(L, 1)) == "all" && lua_gettop(L) == 1) {
        g_pKeybindManager->clearKeybinds();
        return 0;
    }

    const char* mods   = luaL_checkstring(L, 1);
    const char* keyStr = luaL_checkstring(L, 2);

    uint32_t    mod = g_pKeybindManager->stringToModMask(mods);

    SParsedKey  key;
    std::string k = keyStr;
    if (Hyprutils::String::isNumber(k) && std::stoi(k) > 9)
        key = {.keycode = (uint32_t)std::stoi(k)};
    else if (k.starts_with("code:") && Hyprutils::String::isNumber(k.substr(5)))
        key = {.keycode = (uint32_t)std::stoi(k.substr(5))};
    else if (k == "catchall")
        key = {.catchAll = true};
    else
        key = {.key = k};

    g_pKeybindManager->removeKeybind(mod, key);
    return 0;
}

static int hlTimer(lua_State* L) {
    auto* mgr = static_cast<CConfigManager*>(lua_touserdata(L, lua_upvalueindex(1)));

    luaL_checktype(L, 1, LUA_TFUNCTION);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getfield(L, 2, "timeout");
    if (!lua_isnumber(L, -1))
        return luaL_error(L, "hl.timer: opts.timeout must be a number (ms)");
    int timeoutMs = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    if (timeoutMs <= 0)
        return luaL_error(L, "hl.timer: opts.timeout must be > 0");

    lua_getfield(L, 2, "type");
    if (!lua_isstring(L, -1))
        return luaL_error(L, "hl.timer: opts.type must be \"repeat\" or \"oneshot\"");
    std::string type = lua_tostring(L, -1);
    lua_pop(L, 1);

    bool repeat = false;
    if (type == "repeat")
        repeat = true;
    else if (type != "oneshot")
        return luaL_error(L, "hl.timer: opts.type must be \"repeat\" or \"oneshot\"");

    lua_pushvalue(L, 1);
    int  ref = luaL_ref(L, LUA_REGISTRYINDEX);

    auto timer = makeShared<CEventLoopTimer>(
        std::chrono::milliseconds(timeoutMs),
        [L, ref, repeat, timeoutMs, mgr](SP<CEventLoopTimer> self, void* data) {
            // update repeat already so that if we call set_timeout inside
            // our timer it doesn't get overwritten
            if (repeat)
                self->updateTimeout(std::chrono::milliseconds(timeoutMs));

            lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
            int status = LUA_OK;
            if (mgr)
                status = mgr->guardedPCall(0, 0, 0, CConfigManager::LUA_TIMEOUT_TIMER_CALLBACK_MS, "hl.timer callback");
            else
                status = lua_pcall(L, 0, 0, 0);

            if (status != LUA_OK) {
                Log::logger->log(Log::ERR, "[Lua] error in timer callback: {}", lua_tostring(L, -1));
                lua_pop(L, 1);
            }

            if (!repeat) {
                luaL_unref(L, LUA_REGISTRYINDEX, ref);
                std::erase_if(mgr->m_luaTimers, [&self](const auto& lt) { return lt.timer == self; });
            }
        },
        nullptr);

    mgr->m_luaTimers.emplace_back(CConfigManager::SLuaTimer{timer, ref});
    if (g_pEventLoopManager)
        g_pEventLoopManager->addTimer(timer);

    Objects::CLuaTimer::push(L, timer, timeoutMs);
    return 1;
}

void Internal::registerToplevelBindings(lua_State* L, CConfigManager* mgr) {
    Internal::setMgrFn(L, mgr, "on", hlOn);
    Internal::setMgrFn(L, mgr, "bind", hlBind);
    Internal::setMgrFn(L, mgr, "define_submap", hlDefineSubmap);
    Internal::setMgrFn(L, mgr, "timer", hlTimer);

    Internal::setFn(L, "dispatch", hlDispatch);
    Internal::setFn(L, "exec_cmd", hlExecCmd);
    Internal::setFn(L, "exec_raw", hlExecRaw);
    Internal::setFn(L, "exit", hlExit);
    Internal::setFn(L, "submap", hlSubmap);
    Internal::setFn(L, "pass", hlPass);
    Internal::setFn(L, "send_shortcut", hlSendShortcut);
    Internal::setFn(L, "send_key_state", hlSendKeyState);
    Internal::setFn(L, "layout", hlLayout);
    Internal::setFn(L, "dpms", hlDpms);
    Internal::setFn(L, "event", hlEvent);
    Internal::setFn(L, "global", hlGlobal);
    Internal::setFn(L, "force_renderer_reload", hlForceRendererReload);
    Internal::setFn(L, "force_idle", hlForceIdle);

    Internal::setMgrFn(L, mgr, "exec_once", hlExecOnce);
    Internal::setFn(L, "exec_shutdown", hlExecShutdown);
    Internal::setFn(L, "unbind", hlUnbind);
}
