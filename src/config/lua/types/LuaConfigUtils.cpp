#include "LuaConfigUtils.hpp"
#include "LuaConfigInt.hpp"
#include "LuaConfigFloat.hpp"
#include "LuaConfigBool.hpp"
#include "LuaConfigString.hpp"
#include "LuaConfigColor.hpp"
#include "LuaConfigVec2.hpp"
#include "LuaConfigCssGap.hpp"
#include "LuaConfigFontWeight.hpp"
#include "LuaConfigGradient.hpp"
#include "../../../plugins/types/IntAPIValue.hpp"
#include "../../../plugins/types/FloatAPIValue.hpp"
#include "../../../plugins/types/BoolAPIValue.hpp"
#include "../../../plugins/types/StringAPIValue.hpp"
#include "../../../plugins/types/ColorAPIValue.hpp"
#include "../../../plugins/types/Vec2APIValue.hpp"
#include "../../../plugins/types/CssGapAPIValue.hpp"
#include "../../../plugins/types/FontWeightAPIValue.hpp"
#include "../../../plugins/types/GradientAPIValue.hpp"

using namespace Config;
using namespace Config::Lua;

UP<ILuaConfigValue> Lua::fromAPI(SP<Plugin::IAPIValue> v) {
    if (auto p = dc<Plugin::CIntAPIValue*>(v.get()))
        return makeUnique<CLuaConfigInt>(p->value());
    if (auto p = dc<Plugin::CFloatAPIValue*>(v.get()))
        return makeUnique<CLuaConfigFloat>(p->value());
    if (auto p = dc<Plugin::CBoolAPIValue*>(v.get()))
        return makeUnique<CLuaConfigBool>(p->value());
    if (auto p = dc<Plugin::CStringAPIValue*>(v.get()))
        return makeUnique<CLuaConfigString>(p->value());
    if (auto p = dc<Plugin::CColorAPIValue*>(v.get()))
        return makeUnique<CLuaConfigColor>(p->value());
    if (auto p = dc<Plugin::CVec2APIValue*>(v.get()))
        return makeUnique<CLuaConfigVec2>(p->value());
    if (auto p = dc<Plugin::CCssGapAPIValue*>(v.get()))
        return makeUnique<CLuaConfigCssGap>(p->value().m_top);
    if (auto p = dc<Plugin::CFontWeightAPIValue*>(v.get()))
        return makeUnique<CLuaConfigFontWeight>(p->value().m_value);
    if (auto p = dc<Plugin::CGradientAPIValue*>(v.get()))
        return makeUnique<CLuaConfigGradient>(p->value().m_colors.empty() ? CHyprColor{} : p->value().m_colors.front());

    return nullptr;
}
