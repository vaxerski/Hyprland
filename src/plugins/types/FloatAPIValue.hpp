#include "APIValue.hpp"

#include "../../config/ConfigValue.hpp"

FORWARD_CONFIG_MANAGERS;

namespace Plugin {
    class CFloatAPIValue : public IAPIValue {
      public:
        CFloatAPIValue(const char* name, Config::FLOAT def, std::optional<Config::FLOAT> min = std::nullopt, std::optional<Config::FLOAT> max = std::nullopt);

        virtual ~CFloatAPIValue() = default;

        virtual const std::type_info* underlying() const override;
        virtual void                  commence() override;

        Config::FLOAT                 value() const;

      private:
        CConfigValue<Config::FLOAT>  m_val;
        std::optional<Config::FLOAT> m_min, m_max;
        Config::FLOAT                m_default = 0.F;

        CONFIG_FRIEND
    };
}