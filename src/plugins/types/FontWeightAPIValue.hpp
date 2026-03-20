#include "APIValue.hpp"

#include "../../config/ConfigValue.hpp"
#include "../../config/shared/complex/ComplexDataTypes.hpp"

FORWARD_CONFIG_MANAGERS;

namespace Plugin {
    class CFontWeightAPIValue : public IAPIValue {
      public:
        CFontWeightAPIValue(const char* name, Config::INTEGER def = 400);

        virtual ~CFontWeightAPIValue() = default;

        virtual const std::type_info*                 underlying() const override;
        virtual void                                  commence() override;

        const Config::CFontWeightConfigValueData&     value() const;

      private:
        CConfigValue<Config::IComplexConfigValue> m_val;
        Config::CFontWeightConfigValueData        m_default;

        CONFIG_FRIEND
    };
}