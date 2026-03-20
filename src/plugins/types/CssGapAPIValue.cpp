#include "CssGapAPIValue.hpp"

using namespace Plugin;

CCssGapAPIValue::CCssGapAPIValue(const char* name, Config::INTEGER def, std::optional<int64_t> min, std::optional<int64_t> max) : m_min(min), m_max(max), m_default(def) {
    m_name = name;
}

const std::type_info* CCssGapAPIValue::underlying() const {
    return &typeid(decltype(m_default));
}

void CCssGapAPIValue::commence() {
    m_val = CConfigValue<Config::IComplexConfigValue>(m_name);
}

const Config::CCssGapData& CCssGapAPIValue::value() const {
    if (!m_val.good())
        return m_default;

    return *dc<Config::CCssGapData*>(m_val.ptr());
}