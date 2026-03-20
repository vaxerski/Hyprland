#pragma once

#include "LuaConfigValue.hpp"
#include "../../../helpers/memory/Memory.hpp"
#include "../../../plugins/types/APIValue.hpp"

namespace Config::Lua {
    UP<ILuaConfigValue> fromAPI(SP<Plugin::IAPIValue> v);
};