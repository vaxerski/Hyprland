#include "APIValue.hpp"

#include "../../config/ConfigValue.hpp"

#include <optional>
#include <functional>
#include <expected>

FORWARD_CONFIG_MANAGERS;

namespace Plugin {
    class CVec2APIValue : public IAPIValue {
      public:
        CVec2APIValue(const char* name, Config::VEC2 def, std::optional<std::function<std::expected<void, std::string>(const Config::VEC2&)>>&& validator = std::nullopt);

        virtual ~CVec2APIValue() = default;

        virtual const std::type_info* underlying() const override;
        virtual void                  commence() override;

        Config::VEC2                  value() const;

      private:
        CConfigValue<Config::VEC2>                                                          m_val;
        std::optional<std::function<std::expected<void, std::string>(const Config::VEC2&)>> m_validator;
        Config::VEC2                                                                        m_default = {};

        CONFIG_FRIEND
    };
}