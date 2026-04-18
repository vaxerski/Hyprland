#include "LuaBindingsInternal.hpp"

using namespace Config;
using namespace Config::Lua;
using namespace Config::Lua::Bindings;

namespace CA = Config::Actions;

static int dsp_moveToWorkspace(lua_State* L) {
    auto ws = Internal::resolveWorkspaceStr(lua_tostring(L, lua_upvalueindex(1)));
    if (!ws)
        return luaL_error(L, "Invalid workspace");

    bool silent = lua_toboolean(L, lua_upvalueindex(2));
    Internal::checkResult(L, CA::moveToWorkspace(ws, silent, Internal::windowFromUpval(L, 3)));
    return 0;
}

static int dsp_closeWindow(lua_State* L) {
    Internal::checkResult(L, CA::closeWindow(Internal::windowFromUpval(L, 1)));
    return 0;
}

static int dsp_killWindow(lua_State* L) {
    Internal::checkResult(L, CA::killWindow(Internal::windowFromUpval(L, 1)));
    return 0;
}

static int dsp_signalWindow(lua_State* L) {
    Internal::checkResult(L, CA::signalWindow((int)lua_tonumber(L, lua_upvalueindex(1)), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_floatWindow(lua_State* L) {
    Internal::checkResult(L, CA::floatWindow(sc<CA::eTogglableAction>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_fullscreenWindow(lua_State* L) {
    Internal::checkResult(L, CA::fullscreenWindow(sc<eFullscreenMode>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_fullscreenWindowWithAction(lua_State* L) {
    const auto mode      = sc<eFullscreenMode>((int)lua_tonumber(L, lua_upvalueindex(1)));
    const int  actionRaw = (int)lua_tonumber(L, lua_upvalueindex(2));
    auto       maybeW    = Internal::windowFromUpval(L, 3);

    if (actionRaw == 0) {
        Internal::checkResult(L, CA::fullscreenWindow(mode, maybeW));
        return 0;
    }

    const auto target = maybeW.value_or(Desktop::focusState()->window());
    if (!target)
        return luaL_error(L, "No target found.");

    const bool currentlyMode = target->isEffectiveInternalFSMode(mode);

    if (actionRaw == 1) {
        if (!currentlyMode)
            Internal::checkResult(L, CA::fullscreenWindow(mode, maybeW));
        return 0;
    }

    if (actionRaw == 2) {
        if (currentlyMode)
            Internal::checkResult(L, CA::fullscreenWindow(mode, maybeW));
        return 0;
    }

    return luaL_error(L, "hl.window.fullscreen: invalid action");
}

static int dsp_fullscreenState(lua_State* L) {
    const auto desiredInternal = sc<eFullscreenMode>((int)lua_tonumber(L, lua_upvalueindex(1)));
    const auto desiredClient   = sc<eFullscreenMode>((int)lua_tonumber(L, lua_upvalueindex(2)));
    const int  actionRaw       = (int)lua_tonumber(L, lua_upvalueindex(3)); // 0: toggle, 1: set, 2: unset
    auto       maybeW          = Internal::windowFromUpval(L, 4);

    const auto target = maybeW.value_or(Desktop::focusState()->window());
    if (!target)
        return luaL_error(L, "No target found.");

    const auto CURRENT        = target->m_fullscreenState;
    const bool atDesiredState = CURRENT.internal == desiredInternal && CURRENT.client == desiredClient;

    if (actionRaw == 0) {
        Internal::checkResult(L, CA::fullscreenWindow(desiredInternal, desiredClient, maybeW));
        return 0;
    }

    if (actionRaw == 1) {
        if (!atDesiredState)
            Internal::checkResult(L, CA::fullscreenWindow(desiredInternal, desiredClient, maybeW));
        return 0;
    }

    if (actionRaw == 2) {
        if (atDesiredState)
            Internal::checkResult(L, CA::fullscreenWindow(desiredInternal, desiredClient, maybeW));
        return 0;
    }

    return luaL_error(L, "hl.window.fullscreen_state: invalid action");
}

static int dsp_pseudoWindow(lua_State* L) {
    Internal::checkResult(L, CA::pseudoWindow(sc<CA::eTogglableAction>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_moveInDirection(lua_State* L) {
    Internal::checkResult(L, CA::moveInDirection(sc<Math::eDirection>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_swapInDirection(lua_State* L) {
    Internal::checkResult(L, CA::swapInDirection(sc<Math::eDirection>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_center(lua_State* L) {
    Internal::checkResult(L, CA::center(Internal::windowFromUpval(L, 1)));
    return 0;
}

static int dsp_cycleNext(lua_State* L) {
    bool                next        = lua_toboolean(L, lua_upvalueindex(1));
    int                 tiledRaw    = (int)lua_tonumber(L, lua_upvalueindex(2));
    int                 floatingRaw = (int)lua_tonumber(L, lua_upvalueindex(3));
    std::optional<bool> tiled       = tiledRaw < 0 ? std::nullopt : std::optional(tiledRaw > 0);
    std::optional<bool> floating    = floatingRaw < 0 ? std::nullopt : std::optional(floatingRaw > 0);
    Internal::checkResult(L, CA::cycleNext(next, tiled, floating, Internal::windowFromUpval(L, 4)));
    return 0;
}

static int dsp_swapNext(lua_State* L) {
    Internal::checkResult(L, CA::swapNext(lua_toboolean(L, lua_upvalueindex(1)), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_swapWithWindow(lua_State* L) {
    auto       source = Internal::windowFromUpval(L, 1);

    const auto targetSelector = lua_tostring(L, lua_upvalueindex(2));
    const auto target         = g_pCompositor->getWindowByRegex(targetSelector);
    if (!target)
        return luaL_error(L, "hl.window.swap: target window not found");

    Internal::checkResult(L, CA::swapWith(target, source));
    return 0;
}

static int dsp_tagWindow(lua_State* L) {
    Internal::checkResult(L, CA::tag(lua_tostring(L, lua_upvalueindex(1)), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_toggleSwallow(lua_State* L) {
    Internal::checkResult(L, CA::toggleSwallow());
    return 0;
}

static int dsp_resize(lua_State* L) {
    Vector2D value{lua_tonumber(L, lua_upvalueindex(1)), lua_tonumber(L, lua_upvalueindex(2))};
    Internal::checkResult(L, CA::resize(value, lua_toboolean(L, lua_upvalueindex(3)), Internal::windowFromUpval(L, 4)));
    return 0;
}

static int dsp_move(lua_State* L) {
    Vector2D value{lua_tonumber(L, lua_upvalueindex(1)), lua_tonumber(L, lua_upvalueindex(2))};
    Internal::checkResult(L, CA::move(value, lua_toboolean(L, lua_upvalueindex(3)), Internal::windowFromUpval(L, 4)));
    return 0;
}

static int dsp_pinWindow(lua_State* L) {
    Internal::checkResult(L, CA::pinWindow(sc<CA::eTogglableAction>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_bringToTop(lua_State* L) {
    Internal::checkResult(L, CA::alterZOrder("top"));
    return 0;
}

static int dsp_alterZOrder(lua_State* L) {
    Internal::checkResult(L, CA::alterZOrder(lua_tostring(L, lua_upvalueindex(1)), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_setProp(lua_State* L) {
    Internal::checkResult(L, CA::setProp(lua_tostring(L, lua_upvalueindex(1)), lua_tostring(L, lua_upvalueindex(2)), Internal::windowFromUpval(L, 3)));
    return 0;
}

static int dsp_moveIntoGroup(lua_State* L) {
    Internal::checkResult(L, CA::moveIntoGroup(sc<Math::eDirection>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_moveOutOfGroup(lua_State* L) {
    Internal::checkResult(L, CA::moveOutOfGroup(sc<Math::eDirection>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_moveWindowOrGroup(lua_State* L) {
    Internal::checkResult(L, CA::moveWindowOrGroup(sc<Math::eDirection>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_moveIntoOrCreateGroup(lua_State* L) {
    Internal::checkResult(L, CA::moveIntoOrCreateGroup(sc<Math::eDirection>((int)lua_tonumber(L, lua_upvalueindex(1))), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_denyFromGroup(lua_State* L) {
    Internal::checkResult(L, CA::denyWindowFromGroup(sc<CA::eTogglableAction>((int)lua_tonumber(L, lua_upvalueindex(1)))));
    return 0;
}

static int dsp_mouseDrag(lua_State* L) {
    Internal::checkResult(L, CA::mouse("movewindow"));
    return 0;
}

static int dsp_mouseResize(lua_State* L) {
    Internal::checkResult(L, CA::mouse("resizewindow"));
    return 0;
}

static int hlWindowClose(lua_State* L) {
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_closeWindow, 1);
    return 1;
}

static int hlWindowKill(lua_State* L) {
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_killWindow, 1);
    return 1;
}

static int hlWindowSignal(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.window.signal: expected a table { signal, window? }");

    lua_pushnumber(L, Internal::requireTableFieldNum(L, 1, "signal", "hl.window.signal"));
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_signalWindow, 2);
    return 1;
}

static int hlWindowFloat(lua_State* L) {
    const auto action = Internal::tableToggleAction(L, 1);

    lua_pushnumber(L, (int)action);
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_floatWindow, 2);
    return 1;
}

static int hlWindowFullscreen(lua_State* L) {
    eFullscreenMode mode   = FSMODE_FULLSCREEN;
    int             action = 0; // 0: toggle, 1: set, 2: unset
    if (lua_istable(L, 1)) {
        auto m = Internal::tableOptStr(L, 1, "mode");
        if (m) {
            if (*m == "maximized" || *m == "1")
                mode = FSMODE_MAXIMIZED;
            else if (*m == "fullscreen" || *m == "0")
                mode = FSMODE_FULLSCREEN;
            else
                return luaL_error(L, "hl.window.fullscreen: invalid mode \"%s\" (expected fullscreen/maximized)", m->c_str());
        }

        auto a = Internal::tableOptStr(L, 1, "action");
        if (a) {
            if (*a == "toggle")
                action = 0;
            else if (*a == "set")
                action = 1;
            else if (*a == "unset")
                action = 2;
            else
                return luaL_error(L, "hl.window.fullscreen: invalid action \"%s\" (expected toggle/set/unset)", a->c_str());
        }
    }
    lua_pushnumber(L, (int)mode);
    if (action == 0) {
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_fullscreenWindow, 2);
    } else {
        lua_pushnumber(L, action);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_fullscreenWindowWithAction, 3);
    }
    return 1;
}

static int hlWindowFullscreenState(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.window.fullscreen_state: expected a table { internal, client, action?, window? }");

    int action = 1; // default to set semantics
    if (auto a = Internal::tableOptStr(L, 1, "action"); a) {
        if (*a == "toggle")
            action = 0;
        else if (*a == "set")
            action = 1;
        else if (*a == "unset")
            action = 2;
        else
            return luaL_error(L, "hl.window.fullscreen_state: invalid action \"%s\" (expected toggle/set/unset)", a->c_str());
    }

    auto im = Internal::tableOptNum(L, 1, "internal");
    auto cm = Internal::tableOptNum(L, 1, "client");
    if (!im || !cm)
        return luaL_error(L, "hl.window.fullscreen_state: 'internal' and 'client' are required");

    lua_pushnumber(L, (int)*im);
    lua_pushnumber(L, (int)*cm);
    lua_pushnumber(L, action);
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_fullscreenState, 4);
    return 1;
}

static int hlWindowPseudo(lua_State* L) {
    const auto action = Internal::tableToggleAction(L, 1);

    lua_pushnumber(L, (int)action);
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_pseudoWindow, 2);
    return 1;
}

static int hlWindowMove(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.window.move: expected a table, e.g. { direction = \"left\" }");

    auto dirStr = Internal::tableOptStr(L, 1, "direction");
    if (dirStr) {
        auto dir = Internal::parseDirectionStr(*dirStr);
        if (dir == Math::DIRECTION_DEFAULT)
            return luaL_error(L, "hl.window.move: invalid direction \"%s\" (expected left/right/up/down)", dirStr->c_str());

        auto groupAware = Internal::tableOptBool(L, 1, "group_aware");
        if (groupAware && *groupAware) {
            lua_pushnumber(L, (int)dir);
            Internal::pushWindowUpval(L, 1);
            lua_pushcclosure(L, dsp_moveWindowOrGroup, 2);
            return 1;
        }

        lua_pushnumber(L, (int)dir);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_moveInDirection, 2);
        return 1;
    }

    auto x = Internal::tableOptNum(L, 1, "x");
    auto y = Internal::tableOptNum(L, 1, "y");
    if (x && y) {
        bool relative = Internal::tableOptBool(L, 1, "relative").value_or(false);
        lua_pushnumber(L, *x);
        lua_pushnumber(L, *y);
        lua_pushboolean(L, relative);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_move, 4);
        return 1;
    }

    auto ws = Internal::tableOptWorkspaceSelector(L, 1, "workspace", "hl.window.move");
    if (ws) {
        auto follow = Internal::tableOptBool(L, 1, "follow");
        bool silent = follow.has_value() && !*follow;
        lua_pushstring(L, ws->c_str());
        lua_pushboolean(L, silent);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_moveToWorkspace, 3);
        return 1;
    }

    auto intoGroup = Internal::tableOptStr(L, 1, "into_group");
    if (intoGroup) {
        auto dir = Internal::parseDirectionStr(*intoGroup);
        if (dir == Math::DIRECTION_DEFAULT)
            return luaL_error(L, "hl.window.move: invalid into_group direction \"%s\"", intoGroup->c_str());
        lua_pushnumber(L, (int)dir);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_moveIntoGroup, 2);
        return 1;
    }

    auto intoOrCreateGroup = Internal::tableOptStr(L, 1, "into_or_create_group");
    if (intoOrCreateGroup) {
        auto dir = Internal::parseDirectionStr(*intoOrCreateGroup);
        if (dir == Math::DIRECTION_DEFAULT)
            return luaL_error(L, "hl.window.move: invalid into_or_create_group direction \"%s\"", intoOrCreateGroup->c_str());
        lua_pushnumber(L, (int)dir);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_moveIntoOrCreateGroup, 2);
        return 1;
    }

    auto outOfGroup = Internal::tableOptStr(L, 1, "out_of_group");
    if (outOfGroup) {
        auto dir = Internal::parseDirectionStr(*outOfGroup);
        lua_pushnumber(L, (int)dir);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_moveOutOfGroup, 2);
        return 1;
    }

    auto outOfGroupBool = Internal::tableOptBool(L, 1, "out_of_group");
    if (outOfGroupBool && *outOfGroupBool) {
        lua_pushnumber(L, (int)Math::DIRECTION_DEFAULT);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_moveOutOfGroup, 2);
        return 1;
    }

    return luaL_error(L, "hl.window.move: unrecognized arguments. Expected one of: direction, x+y(+relative), workspace, into_group, out_of_group");
}

static int hlWindowSwap(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.window.swap: expected a table, e.g. { direction = \"left\" }");

    auto dirStr = Internal::tableOptStr(L, 1, "direction");
    if (dirStr) {
        auto dir = Internal::parseDirectionStr(*dirStr);
        if (dir == Math::DIRECTION_DEFAULT)
            return luaL_error(L, "hl.window.swap: invalid direction \"%s\" (expected left/right/up/down)", dirStr->c_str());
        lua_pushnumber(L, (int)dir);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_swapInDirection, 2);
        return 1;
    }

    auto target = Internal::tableOptWindowSelector(L, 1, "target", "hl.window.swap");
    if (!target)
        target = Internal::tableOptWindowSelector(L, 1, "with", "hl.window.swap");
    if (!target)
        target = Internal::tableOptWindowSelector(L, 1, "other", "hl.window.swap");

    if (target) {
        Internal::pushWindowUpval(L, 1);
        lua_pushstring(L, target->c_str());
        lua_pushcclosure(L, dsp_swapWithWindow, 2);
        return 1;
    }

    auto next = Internal::tableOptBool(L, 1, "next");
    if (next && *next) {
        lua_pushboolean(L, true);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_swapNext, 2);
        return 1;
    }

    auto prev = Internal::tableOptBool(L, 1, "prev");
    if (prev && *prev) {
        lua_pushboolean(L, false);
        Internal::pushWindowUpval(L, 1);
        lua_pushcclosure(L, dsp_swapNext, 2);
        return 1;
    }

    return luaL_error(L, "hl.window.swap: unrecognized arguments. Expected one of: direction, target/with/other, next, prev");
}

static int hlWindowCenter(lua_State* L) {
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_center, 1);
    return 1;
}

static int hlWindowCycleNext(lua_State* L) {
    bool next     = true;
    int  tiled    = -1;
    int  floating = -1;
    if (lua_istable(L, 1)) {
        auto n = Internal::tableOptBool(L, 1, "next");
        if (n)
            next = *n;
        auto t = Internal::tableOptBool(L, 1, "tiled");
        if (t)
            tiled = *t ? 1 : 0;
        auto f = Internal::tableOptBool(L, 1, "floating");
        if (f)
            floating = *f ? 1 : 0;
    }
    lua_pushboolean(L, next);
    lua_pushnumber(L, tiled);
    lua_pushnumber(L, floating);
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_cycleNext, 4);
    return 1;
}

static int hlWindowTag(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.window.tag: expected a table { tag, window? }");

    const auto tag = Internal::requireTableFieldStr(L, 1, "tag", "hl.window.tag");
    lua_pushstring(L, tag.c_str());
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_tagWindow, 2);
    return 1;
}

static int hlWindowToggleSwallow(lua_State* L) {
    lua_pushcclosure(L, dsp_toggleSwallow, 0);
    return 1;
}

static int hlWindowResizeExact(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.window.resize: expected a table { x, y, relative?, window? }");
    auto x        = Internal::tableOptNum(L, 1, "x");
    auto y        = Internal::tableOptNum(L, 1, "y");
    bool relative = Internal::tableOptBool(L, 1, "relative").value_or(false);
    if (!x || !y)
        return luaL_error(L, "hl.window.resize: 'x' and 'y' are required");
    lua_pushnumber(L, *x);
    lua_pushnumber(L, *y);
    lua_pushboolean(L, relative);
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_resize, 4);
    return 1;
}

static int hlWindowPin(lua_State* L) {
    const auto action = Internal::tableToggleAction(L, 1);

    lua_pushnumber(L, (int)action);
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_pinWindow, 2);
    return 1;
}

static int hlWindowBringToTop(lua_State* L) {
    lua_pushcclosure(L, dsp_bringToTop, 0);
    return 1;
}

static int hlWindowAlterZOrder(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.window.alter_zorder: expected a table { mode, window? }");

    const auto mode = Internal::requireTableFieldStr(L, 1, "mode", "hl.window.alter_zorder");
    lua_pushstring(L, mode.c_str());
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_alterZOrder, 2);
    return 1;
}

static int hlWindowSetProp(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.window.set_prop: expected a table { prop, value, window? }");

    const auto prop  = Internal::requireTableFieldStr(L, 1, "prop", "hl.window.set_prop");
    const auto value = Internal::requireTableFieldStr(L, 1, "value", "hl.window.set_prop");
    lua_pushstring(L, prop.c_str());
    lua_pushstring(L, value.c_str());
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_setProp, 3);
    return 1;
}

static int hlWindowDenyFromGroup(lua_State* L) {
    const auto action = Internal::tableToggleAction(L, 1);

    lua_pushnumber(L, (int)action);
    lua_pushcclosure(L, dsp_denyFromGroup, 1);
    return 1;
}

static int hlWindowDrag(lua_State* L) {
    lua_pushcclosure(L, dsp_mouseDrag, 0);
    return 1;
}

static int hlWindowResize(lua_State* L) {
    if (lua_gettop(L) == 0 || lua_isnil(L, 1)) {
        lua_pushcclosure(L, dsp_mouseResize, 0);
        return 1;
    }

    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.window.resize: expected no args, or a table { x, y, relative?, window? }");

    return hlWindowResizeExact(L);
}
void Internal::registerWindowBindings(lua_State* L) {
    lua_newtable(L);
    Internal::setFn(L, "close", hlWindowClose);
    Internal::setFn(L, "kill", hlWindowKill);
    Internal::setFn(L, "signal", hlWindowSignal);
    Internal::setFn(L, "float", hlWindowFloat);
    Internal::setFn(L, "fullscreen", hlWindowFullscreen);
    Internal::setFn(L, "fullscreen_state", hlWindowFullscreenState);
    Internal::setFn(L, "pseudo", hlWindowPseudo);
    Internal::setFn(L, "move", hlWindowMove);
    Internal::setFn(L, "swap", hlWindowSwap);
    Internal::setFn(L, "center", hlWindowCenter);
    Internal::setFn(L, "cycle_next", hlWindowCycleNext);
    Internal::setFn(L, "tag", hlWindowTag);
    Internal::setFn(L, "toggle_swallow", hlWindowToggleSwallow);
    Internal::setFn(L, "pin", hlWindowPin);
    Internal::setFn(L, "bring_to_top", hlWindowBringToTop);
    Internal::setFn(L, "alter_zorder", hlWindowAlterZOrder);
    Internal::setFn(L, "set_prop", hlWindowSetProp);
    Internal::setFn(L, "deny_from_group", hlWindowDenyFromGroup);
    Internal::setFn(L, "drag", hlWindowDrag);
    Internal::setFn(L, "resize", hlWindowResize);
    lua_setfield(L, -2, "window");
}
