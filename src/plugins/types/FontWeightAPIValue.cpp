#include "FontWeightAPIValue.hpp"

using namespace Plugin;

CFontWeightAPIValue::CFontWeightAPIValue(const char* name, Config::INTEGER def) : m_default(def) {
    m_name = name;
}

const std::type_info* CFontWeightAPIValue::underlying() const {
    return &typeid(Config::IComplexConfigValue*);
}

void CFontWeightAPIValue::commence() {
    m_val = CConfigValue<Config::IComplexConfigValue>(m_name);
}

const Config::CFontWeightConfigValueData& CFontWeightAPIValue::value() const {
    if (!m_val.good())
        return m_default;

    return *dc<Config::CFontWeightConfigValueData*>(m_val.ptr());
}