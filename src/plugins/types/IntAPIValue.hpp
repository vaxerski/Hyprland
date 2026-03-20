#include "APIValue.hpp"

#include "../../config/ConfigValue.hpp"

#include <optional>
#include <unordered_map>
#include <string>

FORWARD_CONFIG_MANAGERS;

namespace Plugin {
    class CIntAPIValue : public IAPIValue {
      public:
        CIntAPIValue(const char* name, Config::INTEGER def, std::optional<Config::INTEGER> min = std::nullopt, std::optional<Config::INTEGER> max = std::nullopt,
                     std::optional<std::unordered_map<std::string, Config::INTEGER>> map = std::nullopt);

        virtual ~CIntAPIValue() = default;

        virtual const std::type_info* underlying() const override;
        virtual void                  commence() override;

        Config::INTEGER               value() const;

      private:
        CConfigValue<Config::INTEGER>                                       m_val;
        std::optional<Config::INTEGER>                                      m_min, m_max;
        std::optional<std::unordered_map<std::string, Config::INTEGER>>     m_map;
        Config::INTEGER                                                     m_default = 0;

        CONFIG_FRIEND
    };
}