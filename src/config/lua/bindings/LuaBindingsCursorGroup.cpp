#include "LuaBindingsInternal.hpp"

using namespace Config;
using namespace Config::Lua;
using namespace Config::Lua::Bindings;

namespace CA = Config::Actions;

static int dsp_moveCursorToCorner(lua_State* L) {
    Internal::checkResult(L, CA::moveCursorToCorner((int)lua_tonumber(L, lua_upvalueindex(1)), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_moveCursor(lua_State* L) {
    Internal::checkResult(L, CA::moveCursor(Vector2D{lua_tonumber(L, lua_upvalueindex(1)), lua_tonumber(L, lua_upvalueindex(2))}));
    return 0;
}

static int dsp_toggleGroup(lua_State* L) {
    Internal::checkResult(L, CA::toggleGroup(Internal::windowFromUpval(L, 1)));
    return 0;
}

static int dsp_changeGroupActive(lua_State* L) {
    Internal::checkResult(L, CA::changeGroupActive(lua_toboolean(L, lua_upvalueindex(1)), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_setGroupActive(lua_State* L) {
    Internal::checkResult(L, CA::setGroupActive((int)lua_tonumber(L, lua_upvalueindex(1)), Internal::windowFromUpval(L, 2)));
    return 0;
}

static int dsp_moveGroupWindow(lua_State* L) {
    Internal::checkResult(L, CA::moveGroupWindow(lua_toboolean(L, lua_upvalueindex(1))));
    return 0;
}

static int dsp_lockGroups(lua_State* L) {
    Internal::checkResult(L, CA::lockGroups(sc<CA::eTogglableAction>((int)lua_tonumber(L, lua_upvalueindex(1)))));
    return 0;
}

static int dsp_lockActiveGroup(lua_State* L) {
    Internal::checkResult(L, CA::lockActiveGroup(sc<CA::eTogglableAction>((int)lua_tonumber(L, lua_upvalueindex(1)))));
    return 0;
}

static int hlCursorMoveToCorner(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.cursor.move_to_corner: expected a table { corner, window? }");

    lua_pushnumber(L, Internal::requireTableFieldNum(L, 1, "corner", "hl.cursor.move_to_corner"));
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_moveCursorToCorner, 2);
    return 1;
}

static int hlCursorMove(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.cursor.move: expected a table { x, y }");

    lua_pushnumber(L, Internal::requireTableFieldNum(L, 1, "x", "hl.cursor.move"));
    lua_pushnumber(L, Internal::requireTableFieldNum(L, 1, "y", "hl.cursor.move"));
    lua_pushcclosure(L, dsp_moveCursor, 2);
    return 1;
}

static int hlGroupToggle(lua_State* L) {
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_toggleGroup, 1);
    return 1;
}

static int hlGroupNext(lua_State* L) {
    lua_pushboolean(L, true);
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_changeGroupActive, 2);
    return 1;
}

static int hlGroupPrev(lua_State* L) {
    lua_pushboolean(L, false);
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_changeGroupActive, 2);
    return 1;
}

static int hlGroupMoveWindow(lua_State* L) {
    bool forward = true;
    if (lua_istable(L, 1)) {
        auto f = Internal::tableOptBool(L, 1, "forward");
        if (f)
            forward = *f;
    }
    lua_pushboolean(L, forward);
    lua_pushcclosure(L, dsp_moveGroupWindow, 1);
    return 1;
}

static int hlGroupActive(lua_State* L) {
    if (!lua_istable(L, 1))
        return luaL_error(L, "hl.group.active: expected a table { index, window? }");

    lua_pushnumber(L, Internal::requireTableFieldNum(L, 1, "index", "hl.group.active"));
    Internal::pushWindowUpval(L, 1);
    lua_pushcclosure(L, dsp_setGroupActive, 2);
    return 1;
}

static int hlGroupLock(lua_State* L) {
    const auto action = Internal::tableToggleAction(L, 1);

    lua_pushnumber(L, (int)action);
    lua_pushcclosure(L, dsp_lockGroups, 1);
    return 1;
}

static int hlGroupLockActive(lua_State* L) {
    const auto action = Internal::tableToggleAction(L, 1);

    lua_pushnumber(L, (int)action);
    lua_pushcclosure(L, dsp_lockActiveGroup, 1);
    return 1;
}

void Internal::registerCursorGroupBindings(lua_State* L) {
    lua_newtable(L);
    Internal::setFn(L, "move_to_corner", hlCursorMoveToCorner);
    Internal::setFn(L, "move", hlCursorMove);
    lua_setfield(L, -2, "cursor");

    lua_newtable(L);
    Internal::setFn(L, "toggle", hlGroupToggle);
    Internal::setFn(L, "next", hlGroupNext);
    Internal::setFn(L, "prev", hlGroupPrev);
    Internal::setFn(L, "active", hlGroupActive);
    Internal::setFn(L, "move_window", hlGroupMoveWindow);
    Internal::setFn(L, "lock", hlGroupLock);
    Internal::setFn(L, "lock_active", hlGroupLockActive);
    lua_setfield(L, -2, "group");
}
