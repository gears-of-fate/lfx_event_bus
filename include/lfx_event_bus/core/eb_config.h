#pragma once
#include <cstddef>

namespace lfx::bus {

    struct BusConfig {
        bool enableEventChannel = true;
        bool enableStateBuffer  = true;

        size_t eventQueueCapacity = 1024; // Worst-case per Moodycamel
    };

} // namespace lfx::bus