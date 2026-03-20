#include "ColorAPIValue.hpp"

using namespace Plugin;

CColorAPIValue::CColorAPIValue(const char* name, Config::INTEGER def) : m_default(def) {
    m_name = name;
}

const std::type_info* CColorAPIValue::underlying() const {
    return &typeid(decltype(m_default));
}

void CColorAPIValue::commence() {
    m_val = CConfigValue<Config::INTEGER>(m_name);
}

Config::INTEGER       CColorAPIValue::value() const {
    if (!m_val.good())
        return m_default;

    return *m_val;
}