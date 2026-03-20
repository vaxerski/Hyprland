#include "IntAPIValue.hpp"

using namespace Plugin;

CIntAPIValue::CIntAPIValue(const char* name, Config::INTEGER def, std::optional<Config::INTEGER> min, std::optional<Config::INTEGER> max,
                           std::optional<std::unordered_map<std::string, Config::INTEGER>> map) : m_min(min), m_max(max), m_map(std::move(map)), m_default(def) {
    m_name = name;
}

const std::type_info* CIntAPIValue::underlying() const {
    return &typeid(decltype(m_default));
}

void CIntAPIValue::commence() {
    m_val = CConfigValue<Config::INTEGER>(m_name);
}

Config::INTEGER       CIntAPIValue::value() const {
    if (!m_val.good())
        return m_default;

    return *m_val;
}