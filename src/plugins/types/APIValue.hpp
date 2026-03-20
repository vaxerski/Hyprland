#pragma once

#include <type_traits>

#define FORWARD_CONFIG_MANAGERS                                                                                                                                                    \
    namespace Config {                                                                                                                                                             \
        namespace Legacy {                                                                                                                                                         \
            class CConfigManager;                                                                                                                                                  \
        }                                                                                                                                                                          \
        namespace Lua {                                                                                                                                                            \
            class CConfigManager;                                                                                                                                                  \
        }                                                                                                                                                                          \
    };

#define CONFIG_FRIEND                                                                                                                                                              \
    friend class Config::Legacy::CConfigManager;                                                                                                                                   \
    friend class Config::Lua::CConfigManager;

namespace Plugin {
    class IAPIValue {
      protected:
        IAPIValue() = default;

        const char* m_name = nullptr;

      public:
        virtual ~IAPIValue() = default;

        virtual const std::type_info* underlying() const = 0;
        virtual const char*           name() const;
        virtual void                  commence() = 0;
    };
};