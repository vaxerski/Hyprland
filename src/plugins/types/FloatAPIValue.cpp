#include "FloatAPIValue.hpp"

using namespace Plugin;

CFloatAPIValue::CFloatAPIValue(const char* name, Config::FLOAT def, std::optional<Config::FLOAT> min, std::optional<Config::FLOAT> max) : m_min(min), m_max(max), m_default(def) {
    m_name = name;
}

const std::type_info* CFloatAPIValue::underlying() const {
    return &typeid(decltype(m_default));
}

void CFloatAPIValue::commence() {
    m_val = CConfigValue<Config::FLOAT>(m_name);
}

Config::FLOAT         CFloatAPIValue::value() const {
    if (!m_val.good())
        return false;

    return *m_val;
}