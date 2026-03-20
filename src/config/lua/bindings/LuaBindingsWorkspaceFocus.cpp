#include "LuaBindingsInternal.hpp"

using namespace Config;
using namespace Config::Lua;
using namespace Config::Lua::Bindings;

namespace CA = Config::Actions;

static int dsp_moveFocus(lua_State* L) {
    Internal::checkResult(L, CA::moveFocus(sc<Math::eDirection>((int)lua_tonumber(L, lua_upvalueindex(1)))));
    return 0;
}

static int dsp_focusMonitor(lua_State* L) {
    const auto PMONITOR = g_pCompositor->getMonitorFromString(lua_tostring(L, lua_upvalueindex(1)));
    if (!PMONITOR)
        return luaL_error(L, "hl.focus.monitor: monitor not found");
    Internal::checkResult(L, CA::focusMonitor(PMONITOR));
    return 0;
}

static int dsp_focusWindowBySelector(lua_State* L) {
    const auto PWINDOW = g_pCompositor->getWindowByRegex(lua_tostring(L, lua_upvalueindex(1)));
    if (!PWINDOW)
        return luaL_error(L, "hl.focus: window not found");
    Internal::checkResult(L, CA::focus(PWINDOW));
    return 0;
}

static int dsp_focusUrgentOrLast(lua_State* L) {
    Internal::checkResult(L, CA::focusUrgentOrLast());
    return 0;
}

static int dsp_focusCurrentOrLast(lua_State* L) {
    Internal::checkResult(L, CA::focusCurrentOrLast());
    return 0;
}

static int hlFocus(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.focus: expected a table, e.g. { direction = \"left\" }");

    auto dirStr = Internal::tableOptStr(L, 1, "direction");
    if (dirStr) {
        auto dir = Internal::parseDirectionStr(*dirStr);
        if (dir == Math::DIRECTION_DEFAULT)
            return luaL_error(L, "hl.focus: invalid direction \"%s\" (expected left/right/up/down)", dirStr->c_str());
        lua_pushnumber(L, (int)dir);
        lua_pushcclosure(L, dsp_moveFocus, 1);
        return 1;
    }

    auto monStr = Internal::tableOptMonitorSelector(L, 1, "monitor", "hl.focus");
    if (monStr) {
        lua_pushstring(L, monStr->c_str());
        lua_pushcclosure(L, dsp_focusMonitor, 1);
        return 1;
    }

    auto winStr = Internal::tableOptWindowSelector(L, 1, "window", "hl.focus");
    if (winStr) {
        lua_pushstring(L, winStr->c_str());
        lua_pushcclosure(L, dsp_focusWindowBySelector, 1);
        return 1;
    }

    auto urgent = Internal::tableOptBool(L, 1, "urgent_or_last");
    if (urgent && *urgent) {
        lua_pushcclosure(L, dsp_focusUrgentOrLast, 0);
        return 1;
    }

    auto last = Internal::tableOptBool(L, 1, "last");
    if (last && *last) {
        lua_pushcclosure(L, dsp_focusCurrentOrLast, 0);
        return 1;
    }

    return luaL_error(L, "hl.focus: unrecognized arguments. Expected one of: direction, monitor, window, urgent_or_last, last");
}

static int dsp_changeWorkspace(lua_State* L) {
    Internal::checkResult(L, CA::changeWorkspace(std::string(lua_tostring(L, lua_upvalueindex(1)))));
    return 0;
}

static int dsp_toggleSpecial(lua_State* L) {
    std::string name                                   = lua_isnil(L, lua_upvalueindex(1)) ? "" : lua_tostring(L, lua_upvalueindex(1));
    const auto& [workspaceID, workspaceName, isAutoID] = getWorkspaceIDNameFromString("special:" + name);
    if (workspaceID == WORKSPACE_INVALID || !g_pCompositor->isWorkspaceSpecial(workspaceID))
        return luaL_error(L, "Invalid special workspace");

    auto ws = g_pCompositor->getWorkspaceByID(workspaceID);
    if (!ws) {
        const auto PMONITOR = Desktop::focusState()->monitor();
        if (PMONITOR)
            ws = g_pCompositor->createNewWorkspace(workspaceID, PMONITOR->m_id, workspaceName);
    }
    if (!ws)
        return luaL_error(L, "Could not resolve special workspace");

    Internal::checkResult(L, CA::toggleSpecial(ws));
    return 0;
}

static int dsp_renameWorkspace(lua_State* L) {
    const auto PWS = g_pCompositor->getWorkspaceByString(lua_tostring(L, lua_upvalueindex(1)));
    if (!PWS)
        return luaL_error(L, "hl.workspace.rename: no such workspace");
    std::string name = lua_isnil(L, lua_upvalueindex(2)) ? "" : lua_tostring(L, lua_upvalueindex(2));
    Internal::checkResult(L, CA::renameWorkspace(PWS, name));
    return 0;
}

static int dsp_moveWorkspaceToMonitor(lua_State* L) {
    const auto WORKSPACEID = getWorkspaceIDNameFromString(lua_tostring(L, lua_upvalueindex(1))).id;
    if (WORKSPACEID == WORKSPACE_INVALID)
        return luaL_error(L, "Invalid workspace");
    const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(WORKSPACEID);
    if (!PWORKSPACE)
        return luaL_error(L, "Workspace not found");
    const auto PMONITOR = g_pCompositor->getMonitorFromString(lua_tostring(L, lua_upvalueindex(2)));
    if (!PMONITOR)
        return luaL_error(L, "Monitor not found");
    Internal::checkResult(L, CA::moveToMonitor(PWORKSPACE, PMONITOR));
    return 0;
}

static int dsp_moveCurrentWorkspaceToMonitor(lua_State* L) {
    const auto PMONITOR = g_pCompositor->getMonitorFromString(lua_tostring(L, lua_upvalueindex(1)));
    if (!PMONITOR)
        return luaL_error(L, "Monitor not found");
    const auto PCURRENTWORKSPACE = Desktop::focusState()->monitor()->m_activeWorkspace;
    if (!PCURRENTWORKSPACE)
        return luaL_error(L, "Invalid workspace");
    Internal::checkResult(L, CA::moveToMonitor(PCURRENTWORKSPACE, PMONITOR));
    return 0;
}

static int dsp_focusWorkspaceOnCurrentMonitor(lua_State* L) {
    auto ws = Internal::resolveWorkspaceStr(lua_tostring(L, lua_upvalueindex(1)));
    if (!ws)
        return luaL_error(L, "Invalid workspace");
    Internal::checkResult(L, CA::changeWorkspaceOnCurrentMonitor(ws));
    return 0;
}

static int dsp_swapActiveWorkspaces(lua_State* L) {
    const auto PMON1 = g_pCompositor->getMonitorFromString(lua_tostring(L, lua_upvalueindex(1)));
    const auto PMON2 = g_pCompositor->getMonitorFromString(lua_tostring(L, lua_upvalueindex(2)));
    if (!PMON1 || !PMON2)
        return luaL_error(L, "Monitor not found");
    Internal::checkResult(L, CA::swapActiveWorkspaces(PMON1, PMON2));
    return 0;
}

static int hlWorkspace(lua_State* L) {
    if (!lua_istable(L, 1)) {
        auto ws = Internal::workspaceSelectorFromLuaSelectorOrObject(L, 1, "hl.workspace");
        if (!ws)
            return luaL_error(L, "hl.workspace: expected a workspace selector/object or table");

        lua_pushstring(L, ws->c_str());
        lua_pushcclosure(L, dsp_changeWorkspace, 1);
        return 1;
    }

    auto special = Internal::tableOptStr(L, 1, "special");
    if (special) {
        lua_pushstring(L, special->c_str());
        lua_pushcclosure(L, dsp_toggleSpecial, 1);
        return 1;
    }

    auto onCurrentMonitor = Internal::tableOptBool(L, 1, "on_current_monitor");
    auto id               = Internal::tableOptWorkspaceSelector(L, 1, "id", "hl.workspace");
    if (id) {
        if (onCurrentMonitor && *onCurrentMonitor) {
            lua_pushstring(L, id->c_str());
            lua_pushcclosure(L, dsp_focusWorkspaceOnCurrentMonitor, 1);
            return 1;
        }
        lua_pushstring(L, id->c_str());
        lua_pushcclosure(L, dsp_changeWorkspace, 1);
        return 1;
    }

    return luaL_error(L, "hl.workspace: unrecognized arguments. Expected one of: number/string shorthand, { special }, { id }");
}

static int hlWorkspaceRename(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.workspace.rename: expected a table { id, name? }");

    const auto id   = Internal::requireTableFieldWorkspaceSelector(L, 1, "id", "hl.workspace.rename");
    auto       name = Internal::tableOptStr(L, 1, "name");

    lua_pushstring(L, id.c_str());
    if (name)
        lua_pushstring(L, name->c_str());
    else
        lua_pushnil(L);
    lua_pushcclosure(L, dsp_renameWorkspace, 2);
    return 1;
}

static int hlWorkspaceMove(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.workspace.move: expected a table, e.g. { monitor = \"DP-1\" }");

    const auto mon = Internal::requireTableFieldMonitorSelector(L, 1, "monitor", "hl.workspace.move");

    auto       id = Internal::tableOptWorkspaceSelector(L, 1, "id", "hl.workspace.move");
    if (id) {
        lua_pushstring(L, id->c_str());
        lua_pushstring(L, mon.c_str());
        lua_pushcclosure(L, dsp_moveWorkspaceToMonitor, 2);
        return 1;
    }

    lua_pushstring(L, mon.c_str());
    lua_pushcclosure(L, dsp_moveCurrentWorkspaceToMonitor, 1);
    return 1;
}

static int hlWorkspaceSwapMonitors(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.workspace.swap_monitors: expected a table { monitor1, monitor2 }");

    const auto m1 = Internal::requireTableFieldMonitorSelector(L, 1, "monitor1", "hl.workspace.swap_monitors");
    const auto m2 = Internal::requireTableFieldMonitorSelector(L, 1, "monitor2", "hl.workspace.swap_monitors");
    lua_pushstring(L, m1.c_str());
    lua_pushstring(L, m2.c_str());
    lua_pushcclosure(L, dsp_swapActiveWorkspaces, 2);
    return 1;
}

static int hlWorkspaceCall(lua_State* L) {
    lua_remove(L, 1);
    return hlWorkspace(L);
}

void Internal::registerWorkspaceFocusBindings(lua_State* L) {
    Internal::setFn(L, "focus", hlFocus);

    lua_newtable(L);
    Internal::setFn(L, "rename", hlWorkspaceRename);
    Internal::setFn(L, "move", hlWorkspaceMove);
    Internal::setFn(L, "swap_monitors", hlWorkspaceSwapMonitors);

    lua_newtable(L);
    lua_pushcfunction(L, hlWorkspaceCall);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, "workspace");
}
