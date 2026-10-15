#pragma once
#include <cstdint>

namespace lfx::bus {

    // 1. ESEMPIO EVENTO (Transitorio -> va nel Channel / Coda Circolare)
    struct TestEventPayload {
        uint32_t eventId = 0;
        float value      = 0.0f;
    };

    // 2. ESEMPIO STATO FINITO (Continuo -> va nel TripleBuffer)
    struct LfxRenderable {
        uint32_t graphicId   = 0;
        uint32_t meshAssetId = 0;
        uint32_t materialId  = 0;

        float alpha       = 1.0f;
        float uvOffset[2] = {0.0f, 0.0f};
        float uvScale[2]  = {1.0f, 1.0f};
    };

} // namespace lfx::bus