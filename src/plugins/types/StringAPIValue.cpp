#include "StringAPIValue.hpp"

using namespace Plugin;

CStringAPIValue::CStringAPIValue(const char* name, Config::STRING def, std::optional<std::function<std::expected<void, std::string>(const std::string&)>>&& validator) :
    m_validator(std::move(validator)), m_default(def) {
    m_name = name;
}

const std::type_info* CStringAPIValue::underlying() const {
    return &typeid(decltype(m_default));
}

void CStringAPIValue::commence() {
    m_val = CConfigValue<Config::STRING>(m_name);
}

Config::STRING        CStringAPIValue::value() const {
    if (!m_val.good())
        return m_default;

    return *m_val;
}