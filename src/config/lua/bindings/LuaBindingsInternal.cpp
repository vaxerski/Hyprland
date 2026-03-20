#include "LuaBindingsInternal.hpp"

using namespace Config;
using namespace Config::Lua;
using namespace Config::Lua::Bindings;

namespace CA = Config::Actions;

namespace {
    constexpr const char* LUA_WINDOW_MT    = "HL.Window";
    constexpr const char* LUA_WORKSPACE_MT = "HL.Workspace";
    constexpr const char* LUA_MONITOR_MT   = "HL.Monitor";
}

std::string Internal::argStr(lua_State* L, int idx) {
    if (lua_type(L, idx) == LUA_TNUMBER)
        return std::to_string((long long)lua_tonumber(L, idx));

    size_t      n = 0;
    const char* s = luaL_checklstring(L, idx, &n);
    return {s, n};
}

std::optional<std::string> Internal::tableOptStr(lua_State* L, int idx, const char* field) {
    lua_getfield(L, idx, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return std::nullopt;
    }

    const char* s = lua_tostring(L, -1);
    lua_pop(L, 1);
    return s ? std::optional(std::string(s)) : std::nullopt;
}

std::optional<double> Internal::tableOptNum(lua_State* L, int idx, const char* field) {
    lua_getfield(L, idx, field);
    if (lua_isnil(L, -1) || !lua_isnumber(L, -1)) {
        lua_pop(L, 1);
        return std::nullopt;
    }

    const double v = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return v;
}

std::optional<bool> Internal::tableOptBool(lua_State* L, int idx, const char* field) {
    lua_getfield(L, idx, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return std::nullopt;
    }

    const bool v = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return v;
}

PHLMONITOR Internal::monitorFromLuaSelectorOrObject(lua_State* L, int idx, const char* fnName) {
    idx = lua_absindex(L, idx);

    if (lua_isnil(L, idx))
        return nullptr;

    if (auto* ref = static_cast<PHLMONITORREF*>(luaL_testudata(L, idx, LUA_MONITOR_MT)); ref)
        return ref->lock();

    if (lua_isstring(L, idx) || lua_isnumber(L, idx))
        return g_pCompositor->getMonitorFromString(argStr(L, idx));

    luaL_error(L, "%s: expected a monitor object or selector", fnName);
    return nullptr;
}

PHLWORKSPACE Internal::workspaceFromLuaSelectorOrObject(lua_State* L, int idx, const char* fnName) {
    idx = lua_absindex(L, idx);

    if (lua_isnil(L, idx))
        return nullptr;

    if (auto* ref = static_cast<PHLWORKSPACEREF*>(luaL_testudata(L, idx, LUA_WORKSPACE_MT)); ref) {
        auto ws = ref->lock();
        if (!ws || ws->inert())
            return nullptr;

        return ws;
    }

    if (lua_isstring(L, idx) || lua_isnumber(L, idx))
        return g_pCompositor->getWorkspaceByString(argStr(L, idx));

    luaL_error(L, "%s: expected a workspace object or selector", fnName);
    return nullptr;
}

PHLWINDOW Internal::windowFromLuaSelectorOrObject(lua_State* L, int idx, const char* fnName) {
    idx = lua_absindex(L, idx);

    if (lua_isnil(L, idx))
        return nullptr;

    if (auto* ref = static_cast<PHLWINDOWREF*>(luaL_testudata(L, idx, LUA_WINDOW_MT)); ref)
        return ref->lock();

    if (lua_isstring(L, idx) || lua_isnumber(L, idx))
        return g_pCompositor->getWindowByRegex(argStr(L, idx));

    luaL_error(L, "%s: expected a window object or selector", fnName);
    return nullptr;
}

std::optional<PHLMONITOR> Internal::tableOptMonitor(lua_State* L, int idx, const char* field, const char* fnName) {
    idx = lua_absindex(L, idx);
    lua_getfield(L, idx, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return std::nullopt;
    }

    auto mon = monitorFromLuaSelectorOrObject(L, -1, fnName);
    lua_pop(L, 1);
    return mon;
}

std::optional<PHLWORKSPACE> Internal::tableOptWorkspace(lua_State* L, int idx, const char* field, const char* fnName) {
    idx = lua_absindex(L, idx);
    lua_getfield(L, idx, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return std::nullopt;
    }

    auto ws = workspaceFromLuaSelectorOrObject(L, -1, fnName);
    lua_pop(L, 1);
    return ws;
}

std::optional<PHLWINDOW> Internal::tableOptWindow(lua_State* L, int idx, const char* field, const char* fnName) {
    idx = lua_absindex(L, idx);
    lua_getfield(L, idx, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return std::nullopt;
    }

    auto window = windowFromLuaSelectorOrObject(L, -1, fnName);
    lua_pop(L, 1);
    return window;
}

std::optional<std::string> Internal::monitorSelectorFromLuaSelectorOrObject(lua_State* L, int idx, const char* fnName) {
    idx = lua_absindex(L, idx);

    if (lua_isnil(L, idx))
        return std::nullopt;

    if (auto* ref = static_cast<PHLMONITORREF*>(luaL_testudata(L, idx, LUA_MONITOR_MT)); ref) {
        const auto mon = ref->lock();
        if (!mon)
            luaL_error(L, "%s: monitor object is expired", fnName);

        return mon->m_name;
    }

    if (lua_isstring(L, idx) || lua_isnumber(L, idx))
        return argStr(L, idx);

    luaL_error(L, "%s: expected a monitor object or selector", fnName);
    return std::nullopt;
}

std::optional<std::string> Internal::workspaceSelectorFromLuaSelectorOrObject(lua_State* L, int idx, const char* fnName) {
    idx = lua_absindex(L, idx);

    if (lua_isnil(L, idx))
        return std::nullopt;

    if (auto* ref = static_cast<PHLWORKSPACEREF*>(luaL_testudata(L, idx, LUA_WORKSPACE_MT)); ref) {
        const auto ws = ref->lock();
        if (!ws || ws->inert())
            luaL_error(L, "%s: workspace object is expired", fnName);

        return std::to_string(ws->m_id);
    }

    if (lua_isstring(L, idx) || lua_isnumber(L, idx))
        return argStr(L, idx);

    luaL_error(L, "%s: expected a workspace object or selector", fnName);
    return std::nullopt;
}

std::optional<std::string> Internal::windowSelectorFromLuaSelectorOrObject(lua_State* L, int idx, const char* fnName) {
    idx = lua_absindex(L, idx);

    if (lua_isnil(L, idx))
        return std::nullopt;

    if (auto* ref = static_cast<PHLWINDOWREF*>(luaL_testudata(L, idx, LUA_WINDOW_MT)); ref) {
        const auto w = ref->lock();
        if (!w)
            luaL_error(L, "%s: window object is expired", fnName);

        return std::format("0x{:x}", reinterpret_cast<uintptr_t>(w.get()));
    }

    if (lua_isstring(L, idx) || lua_isnumber(L, idx))
        return argStr(L, idx);

    luaL_error(L, "%s: expected a window object or selector", fnName);
    return std::nullopt;
}

std::optional<std::string> Internal::tableOptMonitorSelector(lua_State* L, int idx, const char* field, const char* fnName) {
    idx = lua_absindex(L, idx);
    lua_getfield(L, idx, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return std::nullopt;
    }

    auto selector = monitorSelectorFromLuaSelectorOrObject(L, -1, fnName);
    lua_pop(L, 1);
    return selector;
}

std::optional<std::string> Internal::tableOptWorkspaceSelector(lua_State* L, int idx, const char* field, const char* fnName) {
    idx = lua_absindex(L, idx);
    lua_getfield(L, idx, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return std::nullopt;
    }

    auto selector = workspaceSelectorFromLuaSelectorOrObject(L, -1, fnName);
    lua_pop(L, 1);
    return selector;
}

std::optional<std::string> Internal::tableOptWindowSelector(lua_State* L, int idx, const char* field, const char* fnName) {
    idx = lua_absindex(L, idx);
    lua_getfield(L, idx, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return std::nullopt;
    }

    auto selector = windowSelectorFromLuaSelectorOrObject(L, -1, fnName);
    lua_pop(L, 1);
    return selector;
}

std::string Internal::requireTableFieldMonitorSelector(lua_State* L, int idx, const char* field, const char* fnName) {
    auto selector = tableOptMonitorSelector(L, idx, field, fnName);
    if (!selector)
        luaL_error(L, "%s: '%s' is required", fnName, field);

    return *selector;
}

std::string Internal::requireTableFieldWorkspaceSelector(lua_State* L, int idx, const char* field, const char* fnName) {
    auto selector = tableOptWorkspaceSelector(L, idx, field, fnName);
    if (!selector)
        luaL_error(L, "%s: '%s' is required", fnName, field);

    return *selector;
}

std::string Internal::requireTableFieldWindowSelector(lua_State* L, int idx, const char* field, const char* fnName) {
    auto selector = tableOptWindowSelector(L, idx, field, fnName);
    if (!selector)
        luaL_error(L, "%s: '%s' is required", fnName, field);

    return *selector;
}

Math::eDirection Internal::parseDirectionStr(const std::string& str) {
    if (str == "left" || str == "l")
        return Math::DIRECTION_LEFT;
    if (str == "right" || str == "r")
        return Math::DIRECTION_RIGHT;
    if (str == "up" || str == "u" || str == "t")
        return Math::DIRECTION_UP;
    if (str == "down" || str == "d" || str == "b")
        return Math::DIRECTION_DOWN;
    return Math::DIRECTION_DEFAULT;
}

CA::eTogglableAction Internal::parseToggleStr(const std::string& str) {
    if (str.empty() || str == "toggle")
        return CA::TOGGLE_ACTION_TOGGLE;
    if (str == "enable" || str == "on")
        return CA::TOGGLE_ACTION_ENABLE;
    if (str == "disable" || str == "off")
        return CA::TOGGLE_ACTION_DISABLE;
    return CA::TOGGLE_ACTION_TOGGLE;
}

std::optional<PHLWINDOW> Internal::windowFromUpval(lua_State* L, int idx) {
    if (lua_isnil(L, lua_upvalueindex(idx)))
        return std::nullopt;

    return g_pCompositor->getWindowByRegex(lua_tostring(L, lua_upvalueindex(idx)));
}

void Internal::pushWindowUpval(lua_State* L, int tableIdx) {
    if (lua_istable(L, tableIdx)) {
        auto selector = tableOptWindowSelector(L, tableIdx, "window", "window selector");
        if (!selector)
            lua_pushnil(L);
        else
            lua_pushstring(L, selector->c_str());
    } else
        lua_pushnil(L);
}

void Internal::checkResult(lua_State* L, const CA::ActionResult& r) {
    if (!r)
        luaL_error(L, "%s", r.error().c_str());
}

PHLWORKSPACE Internal::resolveWorkspaceStr(const std::string& args) {
    const auto& [id, name, isAutoID] = getWorkspaceIDNameFromString(args);
    if (id == WORKSPACE_INVALID)
        return nullptr;

    auto ws = g_pCompositor->getWorkspaceByID(id);
    if (!ws) {
        const auto PMONITOR = Desktop::focusState()->monitor();
        if (PMONITOR)
            ws = g_pCompositor->createNewWorkspace(id, PMONITOR->m_id, name, false);
    }

    return ws;
}

std::string Internal::getSourceInfo(lua_State* L, int stackLevel) {
    lua_Debug   ar         = {};
    std::string sourceInfo = "?:?";

    if (lua_getstack(L, stackLevel, &ar) && lua_getinfo(L, "Sl", &ar)) {
        const char* src = ar.source;
        if (src && src[0] == '@')
            src++;

        sourceInfo = std::format("{}:{}", src ? src : "?", ar.currentline);
    }

    return sourceInfo;
}

std::string Internal::requireTableFieldStr(lua_State* L, int idx, const char* field, const char* fnName) {
    auto value = tableOptStr(L, idx, field);
    if (!value)
        luaL_error(L, "%s: '%s' is required", fnName, field);

    return *value;
}

double Internal::requireTableFieldNum(lua_State* L, int idx, const char* field, const char* fnName) {
    auto value = tableOptNum(L, idx, field);
    if (!value)
        luaL_error(L, "%s: '%s' is required", fnName, field);

    return *value;
}

CA::eTogglableAction Internal::tableToggleAction(lua_State* L, int idx, const char* field) {
    if (!lua_istable(L, idx))
        return CA::TOGGLE_ACTION_TOGGLE;

    if (const auto action = tableOptStr(L, idx, field); action.has_value())
        return parseToggleStr(*action);

    return CA::TOGGLE_ACTION_TOGGLE;
}

void Internal::setFn(lua_State* L, const char* name, lua_CFunction fn) {
    lua_pushcfunction(L, fn);
    lua_setfield(L, -2, name);
}

void Internal::setMgrFn(lua_State* L, CConfigManager* mgr, const char* name, lua_CFunction fn) {
    lua_pushlightuserdata(L, mgr);
    lua_pushcclosure(L, fn, 1);
    lua_setfield(L, -2, name);
}
