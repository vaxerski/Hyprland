#include "Vec2APIValue.hpp"

using namespace Plugin;

CVec2APIValue::CVec2APIValue(const char* name, Config::VEC2 def, std::optional<std::function<std::expected<void, std::string>(const Config::VEC2&)>>&& validator) :
    m_validator(std::move(validator)), m_default(def) {
    m_name = name;
}

const std::type_info* CVec2APIValue::underlying() const {
    return &typeid(decltype(m_default));
}

void CVec2APIValue::commence() {
    m_val = CConfigValue<Config::VEC2>(m_name);
}

Config::VEC2          CVec2APIValue::value() const {
    if (!m_val.good())
        return m_default;

    return *m_val;
}