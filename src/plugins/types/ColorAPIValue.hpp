#include "APIValue.hpp"

#include "../../config/ConfigValue.hpp"

FORWARD_CONFIG_MANAGERS;

namespace Plugin {
    class CColorAPIValue : public IAPIValue {
      public:
        CColorAPIValue(const char* name, Config::INTEGER def);

        virtual ~CColorAPIValue() = default;

        virtual const std::type_info* underlying() const override;
        virtual void                  commence() override;

        Config::INTEGER               value() const;

      private:
        CConfigValue<Config::INTEGER> m_val;
        Config::INTEGER               m_default = 0;

        CONFIG_FRIEND
    };
}