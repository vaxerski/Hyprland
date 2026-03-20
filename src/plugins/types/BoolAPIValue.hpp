#include "APIValue.hpp"

#include "../../config/ConfigValue.hpp"

FORWARD_CONFIG_MANAGERS;

namespace Plugin {
    class CBoolAPIValue : public IAPIValue {
      public:
        CBoolAPIValue(const char* name, Config::BOOL def);

        virtual ~CBoolAPIValue() = default;

        virtual const std::type_info* underlying() const override;
        virtual void                  commence() override;

        Config::BOOL                  value() const;

      private:
        CConfigValue<Config::BOOL> m_val;
        bool                       m_default = false;

        CONFIG_FRIEND
    };
}