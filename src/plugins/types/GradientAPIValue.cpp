#include "GradientAPIValue.hpp"

using namespace Plugin;

CGradientAPIValue::CGradientAPIValue(const char* name, CHyprColor def) : m_default(def) {
    m_name = name;
}

const std::type_info* CGradientAPIValue::underlying() const {
    return &typeid(decltype(m_default));
}

void CGradientAPIValue::commence() {
    m_val = CConfigValue<Config::IComplexConfigValue>(m_name);
}

const Config::CGradientValueData& CGradientAPIValue::value() const {
    if (!m_val.good())
        return m_default;

    return *dc<Config::CGradientValueData*>(m_val.ptr());
}