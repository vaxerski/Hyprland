#include "APIValue.hpp"

#include "../../config/ConfigValue.hpp"

#include <optional>
#include <functional>
#include <expected>

FORWARD_CONFIG_MANAGERS;

namespace Plugin {
    class CStringAPIValue : public IAPIValue {
      public:
        CStringAPIValue(const char* name, Config::STRING def, std::optional<std::function<std::expected<void, std::string>(const Config::STRING&)>>&& validator = std::nullopt);

        virtual ~CStringAPIValue() = default;

        virtual const std::type_info* underlying() const override;
        virtual void                  commence() override;

        Config::STRING                value() const;

      private:
        CConfigValue<Config::STRING>                                                       m_val;
        std::optional<std::function<std::expected<void, std::string>(const std::string&)>> m_validator;
        Config::STRING                                                                     m_default = "[[EMPTY]]";

        CONFIG_FRIEND
    };
}