#include "BoolAPIValue.hpp"

using namespace Plugin;

CBoolAPIValue::CBoolAPIValue(const char* name, bool def) : m_default(def) {
    m_name = name;
}

const std::type_info* CBoolAPIValue::underlying() const {
    return &typeid(decltype(m_default));
}

bool CBoolAPIValue::value() const {
    if (!m_val.good())
        return false;

    return *m_val;
}

void CBoolAPIValue::commence() {
    m_val = CConfigValue<Config::BOOL>(m_name);
}
