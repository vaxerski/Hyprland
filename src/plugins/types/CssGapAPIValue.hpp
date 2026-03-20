#include "APIValue.hpp"

#include "../../config/ConfigValue.hpp"
#include "../../config/shared/complex/ComplexDataTypes.hpp"

#include <optional>

FORWARD_CONFIG_MANAGERS;

namespace Plugin {
    class CCssGapAPIValue : public IAPIValue {
      public:
        CCssGapAPIValue(const char* name, Config::INTEGER def, std::optional<int64_t> min = std::nullopt, std::optional<int64_t> max = std::nullopt);

        virtual ~CCssGapAPIValue() = default;

        virtual const std::type_info*       underlying() const override;
        virtual void                        commence() override;

        const Config::CCssGapData&          value() const;

      private:
        CConfigValue<Config::IComplexConfigValue> m_val;
        std::optional<int64_t>                    m_min, m_max;
        Config::CCssGapData                       m_default;

        CONFIG_FRIEND
    };
}