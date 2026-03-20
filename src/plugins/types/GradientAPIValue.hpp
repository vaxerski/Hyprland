#include "APIValue.hpp"

#include "../../config/ConfigValue.hpp"
#include "../../config/shared/complex/ComplexDataTypes.hpp"

FORWARD_CONFIG_MANAGERS;

namespace Plugin {
    class CGradientAPIValue : public IAPIValue {
      public:
        CGradientAPIValue(const char* name, CHyprColor def);

        virtual ~CGradientAPIValue() = default;

        virtual const std::type_info*         underlying() const override;
        virtual void                          commence() override;

        const Config::CGradientValueData&     value() const;

      private:
        CConfigValue<Config::IComplexConfigValue> m_val;
        Config::CGradientValueData                m_default;

        CONFIG_FRIEND
    };
}